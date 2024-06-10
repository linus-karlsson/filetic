#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <glad/glad.h>

#include "define.h"

typedef struct OpenGLProperties
{
    HDC hdc;
    HGLRC context;
} OpenGLProperties;

typedef struct File_Attrib
{
    u8* buffer;
    u32 size;
} File_Attrib;

global b8 running = false;

LRESULT msg_handler(HWND window, UINT msg, WPARAM w_param, LPARAM l_param)
{
    LRESULT result = 0;
    switch (msg)
    {
        case WM_DESTROY:
        case WM_QUIT:
        {
            running = false;
            break;
        }
        default:
        {
            result = DefWindowProc(window, msg, w_param, l_param);
            break;
        }
    }
    return result;
}

char* get_last_error()
{
    DWORD error = GetLastError();
    if (!error) return NULL;

    char* message = NULL;

    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char*)&message, 0,
        NULL);

    return message;
}

char* concatinate(const char* first, const size_t first_length,
                  const char* second, const size_t second_length,
                  const char delim_between, size_t* result_length)
{
    const size_t extra = delim_between != '\0';
    const size_t length = first_length + second_length + extra;
    char* result = (char*)calloc(length + 1, sizeof(char));

    memcpy(result, first, first_length);
    if (delim_between) result[first_length] = delim_between;
    memcpy(result + first_length + extra, second, second_length);
    if (result_length)
    {
        *result_length = length;
    }
    return result;
}

void _log_message(const char* prefix, const size_t prefix_len,
                  const char* message, const size_t message_len)
{
    char* final =
        concatinate(prefix, prefix_len, message, message_len, '\0', NULL);
    OutputDebugString(final);
    free(final);
}

void log_message(const char* message, const size_t message_len)
{
    const char* log = "[LOG]: ";
    const size_t log_len = strlen(log);
    _log_message(log, log_len, message, message_len);
}

void log_error_message(const char* message, const size_t message_len)
{
    const char* error = "[ERROR]: ";
    const size_t error_len = strlen(error);
    _log_message(error, error_len, message, message_len);
}

void log_last_error()
{
    char* message = get_last_error();
    log_error_message(message, strlen(message));
    LocalFree(message);
}

OpenGLProperties opengl_init(HWND window)
{
    HDC hdc = GetDC(window);

    PIXELFORMATDESCRIPTOR pixel_format_desc = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 32,
        .cDepthBits = 24,
        .cAlphaBits = 8,
        .cStencilBits = 8,
        .iLayerType = PFD_MAIN_PLANE,
    };

    int pixel_format_index = ChoosePixelFormat(hdc, &pixel_format_desc);
    if (!pixel_format_index)
    {
        log_last_error();
        assert(false);
    }

    PIXELFORMATDESCRIPTOR suggested_pixel_format = { 0 };
    DescribePixelFormat(hdc, pixel_format_index, sizeof(suggested_pixel_format),
                        &suggested_pixel_format);
    if (!SetPixelFormat(hdc, pixel_format_index, &suggested_pixel_format))
    {
        log_last_error();
        assert(false);
    }

    HGLRC context = wglCreateContext(hdc);
    if (!context)
    {
        log_last_error();
        assert(false);
    }

    if (!wglMakeCurrent(hdc, context))
    {
        log_last_error();
        assert(false);
    }

    if (!gladLoadGL())
    {
        const char* message = "Could not load glad!";
        log_error_message(message, strlen(message));
        assert(false);
    }

    return (OpenGLProperties){ .hdc = hdc, .context = context };
}

void clean_opengl(HWND window, OpenGLProperties* opengl_properties)
{
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(opengl_properties->context);
    ReleaseDC(window, opengl_properties->hdc);
}

File_Attrib read_file(const char* file_path, const char* operation)
{
    FILE* file = fopen(file_path, operation);

    if (!file)
    {
        log_last_error();
        assert(false);
    }

    File_Attrib file_attrib = { 0 };
    fseek(file, 0, SEEK_END);
    file_attrib.size = ftell(file);
    rewind(file);

    file_attrib.buffer = (u8*)calloc(file_attrib.size, sizeof(u8));

    if (fread(file_attrib.buffer, sizeof(u8), file_attrib.size, file) !=
        file_attrib.size)
    {
        log_last_error();
        assert(false);
    }
    return file_attrib;
}

u32 compile_shader(u32 type, const u8* source)
{
    u32 id = glCreateShader(type);
    glShaderSource(id, sizeof(u8), (const char**)&source, NULL);
    glCompileShader(id);

    int result = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        const char* prefix = "Failed to compile";
        const char* what_file = type == GL_VERTEX_SHADER ? " vertex shader: "
                                                         : " fragment shader: ";
        size_t prefix_message_length = 0;
        char* prefix_message =
            concatinate(prefix, strlen(prefix), what_file, strlen(what_file), 0,
                        &prefix_message_length);

        int length = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)calloc(length, sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);

        char* error_message = concatinate(
            prefix_message, prefix_message_length, message, length, 0, NULL);

        log_error_message(error_message, strlen(error_message));

        free(prefix_message);
        free(message);
        free(error_message);
        glDeleteShader(id);
        return 0;
    }

    return id;
}

u32 create_shader(const char* vertex_file_path, const char* fragment_file_path)
{
    return 0;
}

int main(int argc, char** argv)
{
    HWND window;
    const int width = 600;
    const int height = 300;

    WNDCLASS window_class = {
        .style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = msg_handler,
        .hInstance = GetModuleHandle(0),
        .lpszClassName = "filetic",
    };

    ATOM res = RegisterClass(&window_class);
    if (res)
    {
        window = CreateWindowEx(0, window_class.lpszClassName, "FileTic",
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE, 10, 10, width,
                                height, 0, 0, window_class.hInstance, 0);
        if (window)
        {
            OpenGLProperties opengl_properties = opengl_init(window);

            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            running = true;
            while (running)
            {
                MSG msg;
                while (PeekMessage(&msg, window, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                RECT client_rect;
                GetClientRect(window, &client_rect);
                GLint viewport_width = client_rect.right - client_rect.left;
                GLint viewport_height = client_rect.bottom - client_rect.top;
                glViewport(0, 0, viewport_width, viewport_height);
                glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                SwapBuffers(opengl_properties.hdc);
            }
            clean_opengl(window, &opengl_properties);
        }
        else
        {
            log_last_error();
            assert(false);
        }
    }
    else
    {
        log_last_error();
        assert(false);
    }
}
