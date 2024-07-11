#pragma once
#include "platform/platform.h"
#include "logging.h"

#include <stdio.h>
#include <Windows.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_NATIVE_INCLUDE_NONE
#include <GLFW/glfw3native.h>
#include <shlwapi.h>
#include <ShlObj.h>
#include <shobjidl.h>
#include <stdlib.h>
#include <time.h>
#include <ole2.h>

#define FTIC_NORMAL_CURSOR 0
#define FTIC_HAND_CURSOR 1
#define FTIC_RESIZE_H_CURSOR 2
#define FTIC_RESIZE_V_CURSOR 3
#define FTIC_RESIZE_NW_CURSOR 4
#define FTIC_MOVE_CURSOR 5
#define FTIC_HIDDEN_CURSOR 6

#define TOTAL_CURSORS 7

typedef struct Callbacks
{
    OnKeyPressedCallback on_key_pressed;
    OnKeyReleasedCallback on_key_released;
    OnButtonPressedCallback on_button_pressed;
    OnButtonReleasedCallback on_button_released;
    OnMouseMovedCallback_ on_mouse_moved;
    OnMouseWheelCallback_ on_mouse_wheel;
    OnWindowFocusedCallback on_window_focused;
    OnWindowResizeCallback on_window_resize;
    OnWindowEnterLeaveCallback on_window_enter_leave;
    OnKeyStrokeCallback_ on_key_stroke;
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

internal wchar_t* char_to_wchar(const char* text, const size_t original_size)
{
    size_t converted_chars = 0;
    wchar_t* wText = (wchar_t*)calloc(original_size, sizeof(wchar_t));
    mbstowcs_s(&converted_chars, wText, original_size, text, _TRUNCATE);
    return wText;
}

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
                b8 ctrl_pressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                b8 alt_pressed = (GetKeyState(VK_MENU) & 0x8000) != 0;
                platform->callbacks.on_key_pressed(key, ctrl_pressed,
                                                   alt_pressed);
            }
            break;
        }
        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            if (platform && platform->callbacks.on_key_released)
            {
                u16 key = (u16)w_param;
                b8 ctrl_pressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                b8 alt_pressed = (GetKeyState(VK_MENU) & 0x8000) != 0;
                platform->callbacks.on_key_released(key, ctrl_pressed,
                                                    alt_pressed);
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

void platform_init_drag_drop()
{
    OleInitialize(NULL);
}

void platform_uninit_drag_drop()
{
    OleUninitialize();
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
                                      OnMouseMovedCallback_ callback)
{
    WindowsPlatformInternal* platform_internal =
        (WindowsPlatformInternal*)platform;
    platform_internal->callbacks.on_mouse_moved = callback;
}

void platform_event_set_on_mouse_wheel(Platform* platform,
                                       OnMouseWheelCallback_ callback)
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
                                      OnKeyStrokeCallback_ callback)
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

b8 platform_directory_exists(const char* directory_path)
{
    DWORD file_attributes = GetFileAttributes(directory_path);
    return (file_attributes != INVALID_FILE_ATTRIBUTES &&
            (file_attributes & FILE_ATTRIBUTE_DIRECTORY));
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
    HINSTANCE result =
        ShellExecute(NULL, "open", file_path, NULL, NULL, SW_SHOWNORMAL);

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

internal HGLOBAL create_hglobal_from_paths(const CharPtrArray* paths)
{
    u32* file_paths_length = (u32*)calloc(paths->size, sizeof(u32));
    // NOTE(Linus): +1 for the double null terminator
    size_t total_size = sizeof(DROPFILES) + 1;
    for (u32 i = 0; i < paths->size; ++i)
    {
        const u32 length = (u32)strlen(paths->data[i]) + 1;
        file_paths_length[i] = length;
        total_size += length * sizeof(wchar_t);
    }

    HGLOBAL hglobal = GlobalAlloc(GHND | GMEM_SHARE, total_size);
    if (!hglobal) return hglobal;

    GlobalLock(hglobal);

    DROPFILES* drop_files = (DROPFILES*)GlobalLock(hglobal);
    drop_files->pFiles = sizeof(DROPFILES);
    drop_files->pt.x = 0;
    drop_files->pt.y = 0;
    drop_files->fNC = TRUE;
    drop_files->fWide = TRUE; // ANSI or Unicode

    wchar_t* ptr = (wchar_t*)((char*)drop_files + sizeof(DROPFILES));
    u32 j = 0;
    for (u32 i = 0; i < paths->size; ++i)
    {
        const size_t size_left = total_size - sizeof(DROPFILES) - j;
        ftic_assert(size_left >= file_paths_length[i]);
        memcpy(ptr + j, char_to_wchar(paths->data[i], file_paths_length[i]),
               file_paths_length[i] * sizeof(wchar_t));
        j += file_paths_length[i];
    }
    ptr[j] = L'\0'; // Double null terminator
    free(file_paths_length);

    GlobalUnlock(hglobal);
    return hglobal;
}

void platform_copy_to_clipboard(const CharPtrArray* paths)
{
    HGLOBAL hglobal = create_hglobal_from_paths(paths);
    if (!hglobal) return;

    if (OpenClipboard(NULL))
    {
        EmptyClipboard();
        SetClipboardData(CF_HDROP, hglobal);
        CloseClipboard();
    }
    else
    {
        log_last_error();
        GlobalFree(hglobal);
    }
}

void platform_paste_from_clipboard(CharPtrArray* paths)
{
    if (OpenClipboard(NULL))
    {
        HGLOBAL hGlobal = GetClipboardData(CF_HDROP);
        if (hGlobal)
        {
            DROPFILES* df = (DROPFILES*)GlobalLock(hGlobal);
            if (df)
            {
                if (df->fWide)
                {
                    // TODO(Linus): handle Unicode
                    char* ptr = (char*)df + df->pFiles;
                    u32 current = 0;

                    while (ptr[current])
                    {
                        const u32 start_index = current;
                        u32 size = 0;
                        while (ptr[current] || ptr[current + 1])
                        {
                            size++;
                            current += 2;
                        }
                        current += 2;

                        char* buffer = (char*)calloc(size + 2, sizeof(char));
                        for (u32 i = 0, j = start_index; i < size; ++i, j += 2)
                        {
                            buffer[i] = ptr[j];
                        }
                        array_push(paths, buffer);
                    }
                }
                else
                {
                    char* ptr = (char*)df + df->pFiles;
                    while (*ptr)
                    {
                        const size_t length = strlen(ptr);
                        char* buffer = (char*)calloc(length + 2, sizeof(char));
                        memcpy(buffer, ptr, length);
                        array_push(paths, buffer);
                        ptr += length + 1;
                    }
                }
                GlobalUnlock(hGlobal);
            }
        }
        CloseClipboard();
    }
}

b8 platform_clipboard_is_empty()
{
    b8 result = true;
    if (OpenClipboard(NULL))
    {
        HGLOBAL hGlobal = GetClipboardData(CF_HDROP);
        if (hGlobal)
        {
            result = false;
            GlobalUnlock(hGlobal);
        }
        CloseClipboard();
    }
    return result;
}

void platform_paste_to_directory(const CharPtrArray* paths,
                                 const char* directory_path)
{

    const size_t directory_path_length = strlen(directory_path);
    for (u32 i = 0; i < paths->size; ++i)
    {
        char* source_path = paths->data[i];
        const size_t source_path_length = strlen(paths->data[i]);

        size_t name_length = 0;
        for (i32 j = (i32)source_path_length - 1; j >= 0; --j, ++name_length)
        {
            if (source_path[j] == '\\' || source_path[j] == '/') break;
        }

        const size_t source_parent_length = source_path_length - name_length;
        char saved_char = source_path[source_parent_length - 1];
        source_path[source_parent_length - 1] = '\0';
        if (!string_compare_case_insensitive(directory_path, source_path))
        {
            source_path[source_parent_length - 1] = saved_char;
            continue;
        }
        source_path[source_parent_length - 1] = saved_char;

        const size_t destination_path_length =
            directory_path_length + name_length + 1;
        char* destination_path =
            (char*)calloc(destination_path_length + 2, sizeof(char));
        memcpy(destination_path, directory_path, directory_path_length);
        destination_path[directory_path_length] = '\\';
        memcpy(destination_path + directory_path_length + 1,
               source_path + source_parent_length, name_length);

        SHFILEOPSTRUCT file_op = {
            .wFunc = FO_COPY,
            .pFrom = source_path,
            .pTo = destination_path,
            .fFlags = FOF_NOCONFIRMMKDIR, // TODO(Linus): FOF_ALLOWUNDO
        };

        int result = SHFileOperation(&file_op);

        free(destination_path);
    }
}

void platform_delete_files(const CharPtrArray* paths)
{
    for (u32 i = 0; i < paths->size; ++i)
    {
        const char* path = paths->data[i];

        SHFILEOPSTRUCT file_op = {
            .wFunc = FO_DELETE,
            .pFrom = path,
            .fFlags = FOF_ALLOWUNDO,
        };

        int result = SHFileOperation(&file_op);
    }
}

void platform_rename_file(const char* path, char* new_name,
                          const u32 name_length)
{
    u32 path_length = get_path_length(path, (u32)strlen(path));

    char* to = string_copy(path, path_length, name_length + 2);
    memcpy(to + path_length, new_name, name_length);

    SHFILEOPSTRUCT file_op = {
        .wFunc = FO_RENAME,
        .pFrom = path,
        .pTo = to,
        .fFlags = FOF_ALLOWUNDO,
    };

    int result = SHFileOperation(&file_op);

    free(to);
}

void platform_show_properties(i32 x, i32 y, const char* file_path)
{
    wchar_t* w_file_path = char_to_wchar(file_path, strlen(file_path) + 1);

    HRESULT hr = SHObjectProperties(NULL, SHOP_FILEPATH, w_file_path, NULL);
}

/*
void platform_listen_to_directory_change(void* data)
{
    LPCWSTR directoryPath = (LPCWSTR)data;
    HANDLE hDir = CreateFile(
        directoryPath,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );


}
*/

void platform_get_executable_directory(CharArray* buffer)
{
    buffer->capacity = MAX_PATH;
    buffer->data = (char*)calloc(buffer->capacity, sizeof(char));
    buffer->size =
        GetModuleFileNameA(NULL, buffer->data, (DWORD)buffer->capacity);
}

typedef struct DataObject
{
    IDataObjectVtbl* vtbl;
    LONG reference_count;
    const CharPtrArray* file_paths;
} DataObject;

ULONG STDMETHODCALLTYPE data_object_add_ref(IDataObject* data_object)
{
    DataObject* object = (DataObject*)data_object;
    return InterlockedIncrement(&object->reference_count);
}

HRESULT STDMETHODCALLTYPE data_object_query_interface(IDataObject* data_object,
                                                      REFIID riid, void** ppv)
{
    DataObject* object = (DataObject*)data_object;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDataObject))
    {
        *ppv = data_object;
        data_object_add_ref(data_object);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE data_object_release(IDataObject* data_object)
{
    DataObject* object = (DataObject*)data_object;
    ULONG count = InterlockedDecrement(&object->reference_count);
    if (count == 0)
    {
        free(object);
        return 0;
    }
    return count;
}

HRESULT STDMETHODCALLTYPE data_object_get_data(IDataObject* data_object,
                                               FORMATETC* format_etc,
                                               STGMEDIUM* medium)
{
    CLIPFORMAT cfShellIDList =
        (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLIST);

    DataObject* object = (DataObject*)data_object;
    if ((format_etc->cfFormat == CF_HDROP ||
         format_etc->cfFormat == cfShellIDList) &&
        (format_etc->tymed & TYMED_HGLOBAL))
    {
        medium->tymed = TYMED_HGLOBAL;
        medium->pUnkForRelease = NULL;

        HGLOBAL hglobal = create_hglobal_from_paths(object->file_paths);

        if (hglobal == NULL)
        {
            return E_OUTOFMEMORY;
        }
        medium->hGlobal = hglobal;
        return S_OK;
    }
    return DV_E_FORMATETC;
}

HRESULT STDMETHODCALLTYPE data_object_get_data_here(IDataObject* data_object,
                                                    FORMATETC* format_etc,
                                                    STGMEDIUM* medium)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE data_object_query_get_data(IDataObject* data_object,
                                                     FORMATETC* format_etc)
{
    CLIPFORMAT cfShellIDList =
        (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLIST);

    if ((format_etc->cfFormat == CF_HDROP ||
         format_etc->cfFormat == cfShellIDList) &&
        (format_etc->tymed & TYMED_HGLOBAL))
    {
        return S_OK;
    }
    return DV_E_FORMATETC;
}

HRESULT STDMETHODCALLTYPE data_object_get_canonical_format_etc(
    IDataObject* data_object, FORMATETC* format_etc_in,
    FORMATETC* format_etc_out)
{
    format_etc_out->ptd = NULL;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE data_object_set_data(IDataObject* data_object,
                                               FORMATETC* format_etc,
                                               STGMEDIUM* medium, BOOL release)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE data_object_enum_format_etc(
    IDataObject* data_object, DWORD direction, IEnumFORMATETC** enum_format_etc)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE data_object_d_advise(IDataObject* data_object,
                                               FORMATETC* format_etc,
                                               DWORD advf,
                                               IAdviseSink* adv_sink,
                                               DWORD* connection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT STDMETHODCALLTYPE data_object_d_unadvise(IDataObject* data_object,
                                                 DWORD connection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT STDMETHODCALLTYPE data_object_enum_d_advise(IDataObject* data_object,
                                                    IEnumSTATDATA** enum_advise)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

IDataObjectVtbl data_object_vtbl = {
    data_object_query_interface,
    data_object_add_ref,
    data_object_release,
    data_object_get_data,
    data_object_get_data_here,
    data_object_query_get_data,
    data_object_get_canonical_format_etc,
    data_object_set_data,
    data_object_enum_format_etc,
    data_object_d_advise,
    data_object_d_unadvise,
    data_object_enum_d_advise,
};

DataObject* data_object_create(const CharPtrArray* paths)
{
    DataObject* object = (DataObject*)calloc(1, sizeof(DataObject));
    if (object)
    {
        object->vtbl = &data_object_vtbl;
        object->reference_count = 1;
        object->file_paths = paths;
    }
    return object;
}

typedef struct DropSource
{
    IDropSourceVtbl* vtbl;
    LONG reference_count;
} DropSource;

ULONG STDMETHODCALLTYPE drop_source_add_reference(IDropSource* drop_source)
{
    DropSource* object = (DropSource*)drop_source;
    return InterlockedIncrement(&object->reference_count);
}

HRESULT STDMETHODCALLTYPE drop_source_query_interface(IDropSource* drop_source,
                                                      REFIID riid, void** ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDropSource))
    {
        *ppv = drop_source;
        drop_source_add_reference(drop_source);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE drop_source_release(IDropSource* drop_source)
{
    DropSource* object = (DropSource*)drop_source;
    ULONG count = InterlockedDecrement(&object->reference_count);
    if (count == 0)
    {
        free(object);
        return 0;
    }
    return count;
}

HRESULT STDMETHODCALLTYPE drop_source_query_continue_drag(
    IDropSource* drop_source, BOOL escape_pressed, DWORD grf_key_state)
{
    if (escape_pressed)
    {
        return DRAGDROP_S_CANCEL;
    }
    if (!(grf_key_state & MK_LBUTTON))
    {
        return DRAGDROP_S_DROP;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE drop_source_give_feedback(IDropSource* drop_source,
                                                    DWORD effect)
{
    return DRAGDROP_S_USEDEFAULTCURSORS;
}

IDropSourceVtbl drop_source_vtbl = {
    drop_source_query_interface, drop_source_add_reference, drop_source_release,
    drop_source_query_continue_drag, drop_source_give_feedback
};

DropSource* drop_source_create()
{
    DropSource* object = (DropSource*)calloc(1, sizeof(DropSource));
    if (object)
    {
        object->vtbl = &drop_source_vtbl;
        object->reference_count = 1;
    }
    return object;
}

void platform_start_drag_drop(const CharPtrArray* paths)
{
    DataObject* data_object = data_object_create(paths);
    DropSource* drop_source = drop_source_create();

    DWORD effect;
    HRESULT hr =
        DoDragDrop((IDataObject*)data_object, (IDropSource*)drop_source,
                   DROPEFFECT_COPY | DROPEFFECT_MOVE, &effect);

    data_object->vtbl->Release((IDataObject*)data_object);
    drop_source->vtbl->Release((IDropSource*)drop_source);
}

char* wchar_to_char(const wchar_t* w_buffer)
{
    size_t w_len = wcslen(w_buffer);
    size_t c_len = WideCharToMultiByte(CP_UTF8, 0, w_buffer, (int)w_len, NULL,
                                       0, NULL, NULL);

    char* c_buffer = (char*)calloc(c_len + 1, sizeof(char));
    WideCharToMultiByte(CP_UTF8, 0, w_buffer, (int)w_len, c_buffer, (int)c_len,
                        NULL, NULL);

    return c_buffer;
}

internal void get_menu_items(HMENU h_menu, MenuItemArray* menu_items)
{
    int count = GetMenuItemCount(h_menu);
    for (int i = 0; i < count; ++i)
    {
        wchar_t buffer[256] = { 0 };

        MENUITEMINFOW mii = {
            .cbSize = sizeof(MENUITEMINFOW),
            .fMask = MIIM_STRING | MIIM_ID | MIIM_SUBMENU,
            .dwTypeData = buffer,
            .cch = sizeof(buffer) / sizeof(wchar_t),
        };
        if (GetMenuItemInfoW(h_menu, i, TRUE, &mii))
        {
            MenuItem item = { 0 };
            item.id = mii.wID;
            item.text = wchar_to_char(buffer);
            if (mii.hSubMenu)
            {
                array_create(&item.submenu_items, 2);
                get_menu_items(mii.hSubMenu, &item.submenu_items);
            }
            array_push(menu_items, item);
        }
    }
}

void platform_context_menu_create(ContextMenu* menu, const char* path)
{
    CoInitialize(NULL);

    wchar_t* w_path = char_to_wchar(path, strlen(path) + 1);

    LPITEMIDLIST pidl = NULL;
    SFGAOF sfgao;
    HRESULT hr = SHParseDisplayName((LPWSTR)w_path, NULL, &pidl, 0, &sfgao);
    if (FAILED(hr))
    {
        log_last_error();
        CoUninitialize();
        return;
    }

    IShellFolder* psf_parent = NULL;
    PCUITEMID_CHILD pidl_child = NULL;
    hr =
        SHBindToParent(pidl, &IID_IShellFolder, (void**)&psf_parent, &pidl_child);
    if (FAILED(hr))
    {
        log_last_error();
        CoTaskMemFree((void*)pidl);
        CoUninitialize();
        return;
    }

    IContextMenu* pcm = NULL;
    psf_parent->lpVtbl->GetUIObjectOf(psf_parent, NULL, 1, &pidl_child,
                                     &IID_IContextMenu, NULL, (void**)&pcm);
    if (FAILED(hr))
    {
        log_last_error();
        psf_parent->lpVtbl->Release(psf_parent);
        CoTaskMemFree((void*)pidl);
        CoUninitialize();
        return;
    }

    HMENU h_menu = CreatePopupMenu();
    if (h_menu)
    {
        pcm->lpVtbl->QueryContextMenu(pcm, h_menu, 0, 1, 0x7FFF, CMF_NORMAL);
        array_create(&menu->items, 2);
        get_menu_items(h_menu, &menu->items);
        DestroyMenu(h_menu);
    }
    menu->pcm = pcm;
    menu->pidl = (void*)pidl;
    menu->psf_parent = psf_parent;
}

internal void free_sub_menus(MenuItemArray* submenu)
{
    for (u32 i = 0; i < submenu->size; ++i)
    {
        free_sub_menus(&submenu->data[i].submenu_items);
        free(submenu->data[i].text);
    }
    if (submenu->data)
    {
        array_free(submenu);
    }
}

void platform_context_menu_destroy(ContextMenu* menu)
{
    free_sub_menus(&menu->items);
    ((IContextMenu*)menu->pcm)->lpVtbl->Release((IContextMenu*)menu->pcm);
    ((IShellFolder*)menu->psf_parent)
        ->lpVtbl->Release((IShellFolder*)menu->psf_parent);
    CoTaskMemFree(menu->pidl);
    CoUninitialize();
}

void platform_open_context(void* window, const char* path)
{
    CoInitialize(NULL);

    wchar_t* w_path = char_to_wchar(path, strlen(path) + 1);

    LPITEMIDLIST pidl = NULL;
    SFGAOF sfgao;
    HRESULT hr = SHParseDisplayName((LPWSTR)w_path, NULL, &pidl, 0, &sfgao);
    if (FAILED(hr))
    {
        log_last_error();
        CoUninitialize();
        return;
    }

    IShellFolder* psf_parent = NULL;
    PCUITEMID_CHILD pidl_child = NULL;
    hr = SHBindToParent(pidl, &IID_IShellFolder, (void**)&psf_parent,
                        &pidl_child);
    if (FAILED(hr))
    {
        log_last_error();
        CoTaskMemFree((void*)pidl);
        CoUninitialize();
        return;
    }

    IContextMenu* pcm = NULL;
    psf_parent->lpVtbl->GetUIObjectOf(psf_parent, NULL, 1, &pidl_child,
                                      &IID_IContextMenu, NULL, (void**)&pcm);
    if (FAILED(hr))
    {
        log_last_error();
        psf_parent->lpVtbl->Release(psf_parent);
        CoTaskMemFree((void*)pidl);
        CoUninitialize();
        return;
    }

    HMENU h_menu = CreatePopupMenu();
    if (h_menu)
    {
        pcm->lpVtbl->QueryContextMenu(pcm, h_menu, 0, 1, 0x7FFF, CMF_NORMAL);
        int cmd = 0;

        HWND hwnd = glfwGetWin32Window(window);

        POINT pt;
        GetCursorPos(&pt);
        cmd = TrackPopupMenu(h_menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x,
                             pt.y, 0, hwnd, NULL);
        if (cmd)
        {
            CMINVOKECOMMANDINFOEX cmi = { 0 };
            cmi.cbSize = sizeof(CMINVOKECOMMANDINFOEX);
            cmi.fMask = 0;
            cmi.hwnd = hwnd;
            cmi.lpVerb = MAKEINTRESOURCEA(cmd - 1);
            cmi.lpParameters = NULL;
            cmi.lpDirectory = NULL;
            cmi.nShow = SW_SHOWNORMAL;
            cmi.dwHotKey = 0;
            cmi.hIcon = NULL;

            pcm->lpVtbl->InvokeCommand(pcm, (LPCMINVOKECOMMANDINFO)&cmi);
        }

        DestroyMenu(h_menu);
    }

    pcm->lpVtbl->Release(pcm);
    psf_parent->lpVtbl->Release(psf_parent);
    CoTaskMemFree((void*)pidl);

    CoUninitialize();
}

