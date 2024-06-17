#pragma once
#include "platform/platform.h"
#include "logging.h"

#include <stdio.h>
#include <Windows.h>
#include <stdlib.h>
#include <time.h>

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
    HINSTANCE instance;
    HWND window;
    u16 width;
    u16 height;
    Callbacks callbacks;
    OpenGLProperties opengl_properties;

    u32 current_cursor;
    HCURSOR cursors[TOTAL_CURSORS];

    b8 running;
} WindowsPlatformInternal;

internal LRESULT msg_handler(HWND window, UINT msg, WPARAM w_param,
                             LPARAM l_param)
{
    WindowsPlatformInternal* platform =
        (WindowsPlatformInternal*)GetWindowLongPtrA(window, GWLP_USERDATA);
    b8 double_clicked = false;
    LRESULT result = 0;
    switch (msg)
    {
        case WM_CHAR:
        {
            if (platform && platform->callbacks.on_key_stroke)
            {
                char char_code = (char)w_param;
                platform->callbacks.on_key_stroke(char_code);
            }
            break;
        }
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            if (platform && platform->callbacks.on_key_pressed)
            {
                u16 key = (u16)w_param;
                platform->callbacks.on_key_pressed(key);
            }
            break;
        }
        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            if (platform && platform->callbacks.on_key_released)
            {
                u16 key = (u16)w_param;
                platform->callbacks.on_key_released(key);
            }
            break;
        }
        case WM_XBUTTONDOWN:
        {
            WORD xButton = GET_XBUTTON_WPARAM(w_param);
            break;
        }
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        {
            double_clicked = true;
        }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        {
            if (platform && platform->callbacks.on_button_pressed)
            {
                u8 button = (u8)w_param;
                platform->callbacks.on_button_pressed(button, double_clicked);
            }
            break;
        }
        case WM_LBUTTONUP:
        {
            if (platform && platform->callbacks.on_button_released)
            {
                platform->callbacks.on_button_released(1);
            }
            break;
        }
        case WM_RBUTTONUP:
        {
            if (platform && platform->callbacks.on_button_released)
            {
                platform->callbacks.on_button_released(2);
            }
            break;
        }
        case WM_MOUSEMOVE:
        {
            if (platform && platform->callbacks.on_mouse_moved)
            {
                i16 position_x = LOWORD(l_param);
                i16 position_y = HIWORD(l_param);
                platform->callbacks.on_mouse_moved(position_x, position_y);
            }
            break;
        }
        case WM_MOUSEWHEEL:
        {
            if (platform && platform->callbacks.on_mouse_wheel)
            {
                i16 z_delta = GET_WHEEL_DELTA_WPARAM(w_param);
                platform->callbacks.on_mouse_wheel(z_delta);
            }
            break;
        }
        case WM_SIZE:
        {
            if (platform && platform->callbacks.on_window_resize)
            {
                platform->width = LOWORD(l_param);
                platform->height = HIWORD(l_param);
                platform->callbacks.on_window_resize(platform->width,
                                                     platform->height);
            }
            break;
        }
        case WM_SETCURSOR:
        {
            if (platform)
            {
                SetCursor(platform->cursors[platform->current_cursor]);
            }
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
            if (platform)
            {
                platform->running = false;
            }
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

    platform_internal->cursors[FTIC_NORMAL_CURSOR] = LoadCursor(0, IDC_ARROW);
    platform_internal->cursors[FTIC_HAND_CURSOR] = LoadCursor(0, IDC_HAND);
    platform_internal->cursors[FTIC_RESIZE_H_CURSOR] =
        LoadCursor(0, IDC_SIZEWE);
    platform_internal->cursors[FTIC_RESIZE_V_CURSOR] =
        LoadCursor(0, IDC_SIZENS);
    platform_internal->cursors[FTIC_RESIZE_NW_CURSOR] =
        LoadCursor(0, IDC_SIZENWSE);
    platform_internal->cursors[FTIC_MOVE_CURSOR] = LoadCursor(0, IDC_SIZEALL);
    platform_internal->cursors[FTIC_HIDDEN_CURSOR] = NULL;

    WNDCLASS window_class = {
        .style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
        .lpfnWndProc = msg_handler,
        .hInstance = GetModuleHandle(0),
        .lpszClassName = "filetic",
        .hCursor = platform_internal->cursors[FTIC_NORMAL_CURSOR],
    };

    platform_internal->instance = window_class.hInstance;
    platform_internal->current_cursor = FTIC_NORMAL_CURSOR;

    ATOM res = RegisterClass(&window_class);
    if (res)
    {
        platform_internal->window = CreateWindowEx(
            0, window_class.lpszClassName, "FileTic",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, window_class.hInstance, 0);

        if (platform_internal->window)
        {
            RECT window_rect = { 0 };
            GetWindowRect(platform_internal->window, &window_rect);
            platform_internal->width =
                (u16)(window_rect.right - window_rect.left);
            platform_internal->height =
                (u16)(window_rect.bottom - window_rect.top);
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

void platform_shut_down(Platform* platform)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)platform;
    DestroyWindow(platform_internal->window);
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

void platform_change_cursor(Platform* platform, u32 cursor_id)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)platform;

    if (platform_internal->current_cursor != cursor_id)
    {
        if (cursor_id < TOTAL_CURSORS)
        {
            platform_internal->current_cursor = cursor_id;
            SetCursor(
                platform_internal->cursors[platform_internal->current_cursor]);
        }
    }
}

internal char* copy_string(const char* string, const u32 string_length)
{
    char* result = (char*)calloc(string_length + 3, sizeof(char));
    memcpy(result, string, string_length);
    return result;
}

global volatile LONG64 id = 0;

Directory platform_get_directory(const char* directory_path,
                                 const u32 directory_len)
{
    Directory directory = { 0 };
    array_create(&directory.files, 10);
    array_create(&directory.sub_directories, 10);

    WIN32_FIND_DATA ffd = { 0 };
    HANDLE file_handle = FindFirstFile(directory_path, &ffd);
    if (file_handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            char* path =
                concatinate(directory_path, directory_len - 1, ffd.cFileName,
                            (u32)strlen(ffd.cFileName), 0, 2, NULL);
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (!strcmp(ffd.cFileName, ".") || !strcmp(ffd.cFileName, ".."))
                {
                    free(path);
                    continue;
                }
                // TODO: Wasting 8 bytes
                DirectoryItem directory_item = {
                    //.id = (u64)InterlockedAdd64(&id, 1),
                    .path = path,
                    .name = path + directory_len - 1,
                };
                array_push(&directory.sub_directories, directory_item);
            }
            else
            {
                DirectoryItem directory_item = {
                    //.id = (u64)InterlockedAdd64(&id, 1),
                    .size =
                        (ffd.nFileSizeHigh * (MAXDWORD + 1)) + ffd.nFileSizeLow,
                    .path = path,
                    .name = path + directory_len - 1,
                };
                array_push(&directory.files, directory_item);
            }

        } while (FindNextFile(file_handle, &ffd));
        FindClose(file_handle);
    }
    directory.parent = (char*)calloc(directory_len + 1, sizeof(char));
    memcpy(directory.parent, directory_path, directory_len - 2);
    return directory;
}

void platform_reset_directory(Directory* directory)
{
    for (u32 i = 0; i < directory->files.size; ++i)
    {
        free(directory->files.data[i].path);
    }
    for (u32 i = 0; i < directory->sub_directories.size; ++i)
    {
        free(directory->sub_directories.data[i].path);
    }
    free(directory->files.data);
    free(directory->sub_directories.data);
    free(directory->parent);
    directory->files = (DirectoryItemArray){ 0 };
    directory->sub_directories = (DirectoryItemArray){ 0 };
}

FTicMutex platform_mutex_create(void)
{
    return CreateMutex(NULL, false, NULL);
}

void platform_mutex_lock(FTicMutex* mutex)
{
    WaitForSingleObject(*mutex, INFINITE);
}

void platform_mutex_unlock(FTicMutex* mutex)
{
    ReleaseMutex(*mutex);
}

void platform_mutex_destroy(FTicMutex* mutex)
{
    CloseHandle(*mutex);
}

FTicSemaphore platform_semaphore_create(i32 initial_count, i32 max_count)
{
    return CreateSemaphore(NULL, initial_count, max_count, NULL);
}

void platform_semaphore_increment(FTicSemaphore* sem, long* previous_count)
{
    ReleaseSemaphore(*sem, 1, previous_count);
}

void platform_semaphore_wait_and_decrement(FTicSemaphore* sem)
{
    WaitForSingleObject(*sem, INFINITE);
}

void platform_semaphore_destroy(FTicSemaphore* sem)
{
    CloseHandle(*sem);
}

FTicThreadHandle
platform_thread_create(void* data,
                       thread_return_value (*thread_function)(void* data),
                       unsigned long creation_flag, unsigned long* thread_id)
{
    return CreateThread(0, 0, thread_function, data, creation_flag, thread_id);
}

void platform_thread_join(FTicThreadHandle handle)
{
    WaitForSingleObject(handle, INFINITE);
}

void platform_thread_close(FTicThreadHandle handle)
{
    CloseHandle(handle);
}

void platform_thread_terminate(FTicThreadHandle handle)
{
    TerminateThread(handle, 0);
}

void platform_interlock_exchange(volatile long* target, long value)
{
    InterlockedExchange(target, value);
}

long platform_interlock_compare_exchange(volatile long* dest, long value,
                                         long compare)
{
    return InterlockedCompareExchange(dest, value, compare);
}

u32 platform_get_core_count(void)
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
}

f64 platform_get_time(void)
{
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    return now.tv_sec + (now.tv_nsec * 0.000000001);
}

void platform_sleep(u64 milli)
{
    Sleep((DWORD)milli);
}

void platform_open_file(const char* file_path)
{
    HINSTANCE result = ShellExecute(NULL, "open",
                                    file_path, NULL, NULL, SW_SHOWNORMAL);

    if ((int)result <= 32)
    {
        SHELLEXECUTEINFO sei = {
            .cbSize = sizeof(SHELLEXECUTEINFO),
            .fMask = SEE_MASK_INVOKEIDLIST,
            .lpVerb = "openas",
            .lpFile = file_path,
            .lpParameters = NULL,
            .lpDirectory = NULL,
            .nShow = SW_SHOWNORMAL,
            .hInstApp = NULL,
        };

        ShellExecuteEx(&sei);
    }
}

