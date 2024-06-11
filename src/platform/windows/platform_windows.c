#pragma once
#include "platform/platform.h"
#include "logging.h"

#define WIN32_LEAN_AND_MEAN
#define WIN_32_EXTRA_LEAN
#include <Windows.h>
#include <stdlib.h>

typedef struct Callbacks
{
    OnKeyPressedCallback on_key_pressed;
    OnKeyReleasedCallback on_key_released;
    OnButtonPressedCallback on_button_pressed;
    OnButtonReleasedCallback on_button_released;
    OnMouseMovedCallback on_mouse_moved;
    OnMouseWheelCallback on_mouse_wheel;
    OnWindowFocusedCallback on_window_focused;
    OnWindowResizeCallback on_window_resize;
    OnWindowEnterLeaveCallback on_window_enter_leave;
    OnKeyStrokeCallback on_key_stroke;
} Callbacks;

typedef struct OpenGLProperties
{
    HDC hdc;
    HGLRC context;
} OpenGLProperties;

typedef struct WindowsPlatformInternal
{
    HWND window;
    i32 width;
    i32 height;
    Callbacks callbacks;
    OpenGLProperties opengl_properties;
    b8 running;
} WindowsPlatformInternal;

LRESULT msg_handler(HWND window, UINT msg, WPARAM w_param, LPARAM l_param)
{
    WindowsPlatformInternal* platform =
        (WindowsPlatformInternal*)GetWindowLongPtrA(window, GWLP_USERDATA);
    LRESULT result = 0;
    switch (msg)
    {
        case WM_CHAR:
        {
            char char_code = (char)w_param;
            platform->callbacks.on_key_stroke(char_code);
            break;
        }
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            u16 key = (u16)w_param;
            platform->callbacks.on_key_pressed(key);
            break;
        }
        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            u16 key = (u16)w_param;
            platform->callbacks.on_key_released(key);
            break;
        }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        {
            u8 button = (u8)w_param;
            platform->callbacks.on_button_pressed(button);
            break;
        }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        {
            u8 button = (u8)w_param;
            platform->callbacks.on_button_released(button);
            break;
        }
        case WM_MOUSEMOVE:
        {
            i16 position_x = LOWORD(l_param);
            i16 position_y = HIWORD(l_param);
            platform->callbacks.on_mouse_moved(position_x, position_y);
            break;
        }
        case WM_MOUSEWHEEL:
        {
            i16 z_delta = GET_WHEEL_DELTA_WPARAM(w_param);
            platform->callbacks.on_mouse_wheel(z_delta);
            break;
        }
        case WM_SIZE:
        {
            platform->width = LOWORD(l_param);
            platform->height = HIWORD(l_param);
            platform->callbacks.on_window_resize(platform->width,
                                                 platform->height);
            break;
        }
        // TODO: mouse leave and enter and focus;
        case WM_MOVE:
        {
            break;
        }
        case WM_DESTROY:
        case WM_QUIT:
        {
            platform->running = false;
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

void platform_init(const char* title, u16 width, u16 height,
                   Platform** platform)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)calloc(1, sizeof(WindowsPlatformInternal));

    WNDCLASS window_class = {
        .style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = msg_handler,
        .hInstance = GetModuleHandle(0),
        .lpszClassName = "filetic",
    };

    ATOM res = RegisterClass(&window_class);
    if (res)
    {
        platform_internal->window =
            CreateWindowEx(0, window_class.lpszClassName, "FileTic",
                           WS_OVERLAPPEDWINDOW | WS_VISIBLE, 10, 10, width,
                           height, 0, 0, window_class.hInstance, 0);

        if (platform_internal->window)
        {
            platform_internal->width = width;
            platform_internal->height = height;
            platform_internal->running = true;
        }
        else
        {
            log_last_error();
            ftic_assert(false);
        }
    }
    else
    {
        log_last_error();
        ftic_assert(false);
    }
    SetWindowLongPtrA(platform_internal->window, GWLP_USERDATA,
                      (LONG_PTR)platform_internal);
    *platform = (Platform*)platform_internal;
}

b8 platform_is_running(Platform* platform)
{
    return ((WindowsPlatformInternal*)platform)->running;
}

void platform_event_fire(Platform* platform)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)platform;

    MSG msg;
    while (PeekMessage(&msg, platform_internal->window, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

char* platform_get_last_error()
{
    DWORD error = GetLastError();
    if (!error) return "";

    char* message = "";

    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char*)&message, 0,
        NULL);

    return message;
}

void platform_print_string(const char* string)
{
    OutputDebugString(string);
}

void platform_local_free(void* memory)
{
    LocalFree(memory);
}

void platform_opengl_init(Platform* platform)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)platform;
    HDC hdc = GetDC(platform_internal->window);

    PIXELFORMATDESCRIPTOR pixel_format_desc = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 32,
        .cAlphaBits = 8,
        .iLayerType = PFD_MAIN_PLANE,
    };

    int pixel_format_index = ChoosePixelFormat(hdc, &pixel_format_desc);
    if (!pixel_format_index)
    {
        log_last_error();
        ftic_assert(false);
    }

    PIXELFORMATDESCRIPTOR suggested_pixel_format = { 0 };
    DescribePixelFormat(hdc, pixel_format_index, sizeof(suggested_pixel_format),
                        &suggested_pixel_format);
    if (!SetPixelFormat(hdc, pixel_format_index, &suggested_pixel_format))
    {
        log_last_error();
        ftic_assert(false);
    }

    HGLRC context = wglCreateContext(hdc);
    if (!context)
    {
        log_last_error();
        ftic_assert(false);
    }

    if (!wglMakeCurrent(hdc, context))
    {
        log_last_error();
        ftic_assert(false);
    }

    platform_internal->opengl_properties =
        (OpenGLProperties){ .hdc = hdc, .context = context };
}

void platform_opengl_clean(Platform* platform)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)platform;
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(platform_internal->opengl_properties.context);
    ReleaseDC(platform_internal->window,
              platform_internal->opengl_properties.hdc);
}

void platform_opengl_swap_buffers(Platform* platform)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)platform;
    SwapBuffers(platform_internal->opengl_properties.hdc);
}

ClientRect platform_get_client_rect(Platform* platform)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)platform;

    RECT client_rect;
    GetClientRect(platform_internal->window, &client_rect);
    return (ClientRect){
        .left = client_rect.left,
        .top = client_rect.top,
        .right = client_rect.right,
        .bottom = client_rect.bottom,
    };
}

void platform_event_set_on_key_pressed(Platform* platform,
                                       OnKeyPressedCallback callback)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)platform;
    platform_internal->callbacks.on_key_pressed = callback;
}

void platform_event_set_on_key_released(Platform* platform,
                                        OnKeyReleasedCallback callback)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)platform;
    platform_internal->callbacks.on_key_released = callback;
}

void platform_event_set_on_button_pressed(Platform* platform,
                                          OnButtonPressedCallback callback)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)platform;
    platform_internal->callbacks.on_button_pressed = callback;
}

void platform_event_set_on_button_released(Platform* platform,
                                           OnButtonReleasedCallback callback)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)platform;
    platform_internal->callbacks.on_button_released = callback;
}

void platform_event_set_on_mouse_move(Platform* platform,
                                      OnMouseMovedCallback callback)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)platform;
    platform_internal->callbacks.on_mouse_moved = callback;
}

void platform_event_set_on_mouse_wheel(Platform* platform,
                                       OnMouseWheelCallback callback)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)platform;
    platform_internal->callbacks.on_mouse_wheel = callback;
}

void platform_event_set_on_window_focused(Platform* platform,
                                          OnWindowFocusedCallback callback)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)platform;
    platform_internal->callbacks.on_window_focused = callback;
}

void platform_event_set_on_window_resize(Platform* platform,
                                         OnWindowResizeCallback callback)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)platform;
    platform_internal->callbacks.on_window_resize = callback;
}

void platform_event_set_on_window_enter_leave(
    Platform* platform, OnWindowEnterLeaveCallback callback)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)platform;
    platform_internal->callbacks.on_window_enter_leave = callback;
}

void platform_event_set_on_key_stroke(Platform* platform,
                                      OnKeyStrokeCallback callback)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)platform;
    platform_internal->callbacks.on_key_stroke = callback;
}
