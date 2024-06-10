#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <glad/glad.h>

#include "define.h"

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

void _log_message(const char* prefix, const size_t prefix_len,
                  const char* message, const size_t message_len)
{
    char* final = (char*)calloc(message_len + prefix_len + 1, 1);
    memcpy(final, prefix, prefix_len);
    memcpy(final + prefix_len, message, message_len);
    OutputDebugString(final);
    free(final);
}

void log_message(const char* message, const size_t message_len)
{
    const char* log = "[LOG]: ";
    const size_t log_len = strlen(log);
    _log_message(log, log_len, message, message_len);
}

void error_message(const char* message, const size_t message_len)
{
    const char* error = "[ERROR]: ";
    const size_t error_len = strlen(error);
    _log_message(error, error_len, message, message_len);
}

void log_last_error()
{
    char* message = get_last_error();
    error_message(message, strlen(message));
    LocalFree(message);
}

void opengl_init(HWND window)
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

    HGLRC opengl_rc = wglCreateContext(hdc);
    if (!opengl_rc)
    {
        log_last_error();
        assert(false);
    }

    if (!wglMakeCurrent(hdc, opengl_rc))
    {
        log_last_error();
        assert(false);
    }

    if (!gladLoadGL())
    {
        const char* message = "Could not load glad!";
        error_message(message, strlen(message));
        assert(false);
    }

    // TODO: Clean all the resources
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
            opengl_init(window);

            running = true;
            while (running)
            {
                MSG msg;
                while (PeekMessage(&msg, window, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
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
