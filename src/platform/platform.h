#pragma once
#include "define.h"

typedef void Platform;
typedef void (*OnKeyPressedCallback)(u16 key);
typedef void (*OnKeyReleasedCallback)(u16 key);
typedef void (*OnButtonPressedCallback)(u8 key);
typedef void (*OnButtonReleasedCallback)(u8 key);
typedef void (*OnMouseMovedCallback)(i16 x, i16 y);
typedef void (*OnMouseWheelCallback)(i16 z_delta);
typedef void (*OnWindowFocusedCallback)(b8 focused);
typedef void (*OnWindowResizeCallback)(u16 width, u16 height);
typedef void (*OnWindowEnterLeaveCallback)(b8 enter_leave);
typedef void (*OnKeyStrokeCallback)(char key);

typedef struct ClientRect
{
    int left;
    int top;
    int right;
    int bottom;
} ClientRect;

typedef struct File
{
    u64 size;
    const char* name;
    char* path;
} File;

typedef struct DirectoryAttrib
{
    const char* name;
    char* path;
} DirectoryAttrib;

typedef struct FileArray
{
    u32 size;
    u32 capacity;
    File* data;
} FileArray;

typedef struct DirectoryAttribArray
{
    u32 size;
    u32 capacity;
    DirectoryAttrib* data;
} DirectoryAttribArray;

typedef struct Directory
{
    FileArray files;
    DirectoryAttribArray sub_directories;
} Directory;

void platform_init(const char* title, u16 width, u16 height,
                   Platform** platform);
void platform_shut_down(Platform* platform);
b8 platform_is_running(Platform* platform);
void platform_event_fire(Platform* platform);

char* platform_get_last_error();
void platform_print_string(const char* string);
void platform_local_free(void* memory);

void platform_opengl_init(Platform* platform);
void platform_opengl_clean(Platform* platform);
void platform_opengl_swap_buffers(Platform* platform);

ClientRect platform_get_client_rect(Platform* platform);

void platform_event_fire(Platform* platform);
void platform_event_set_on_key_pressed(Platform* platform,
                                       OnKeyPressedCallback callback);
void platform_event_set_on_key_released(Platform* platform,
                                        OnKeyReleasedCallback callback);
void platform_event_set_on_button_pressed(Platform* platform,
                                          OnButtonPressedCallback callback);
void platform_event_set_on_button_released(Platform* platform,
                                           OnButtonReleasedCallback callback);
void platform_event_set_on_mouse_move(Platform* platform,
                                      OnMouseMovedCallback callback);
void platform_event_set_on_mouse_wheel(Platform* platform,
                                       OnMouseWheelCallback callback);
void platform_event_set_on_window_focused(Platform* platform,
                                          OnWindowFocusedCallback callback);
void platform_event_set_on_window_resize(Platform* platform,
                                         OnWindowResizeCallback callback);
void platform_event_set_on_window_enter_leave(
    Platform* platform, OnWindowEnterLeaveCallback callback);
void platform_event_set_on_key_stroke(Platform* platform,
                                      OnKeyStrokeCallback callback);
Directory platform_get_directory(const char* directory_path,
                                 const u32 directory_len);

FTicMutex platform_mutex_create(void);
void platform_mutex_lock(FTicMutex* mutex);
void platform_mutex_unlock(FTicMutex* mutex);
void platform_mutex_destroy(FTicMutex* mutex);

FTicSemaphore platform_semaphore_create(i32 initial_count, i32 max_count);
void platform_semaphore_increment(FTicSemaphore* sem);
void platform_semaphore_wait_and_decrement(FTicSemaphore* sem);
void platform_semaphore_destroy(FTicSemaphore* sem);

FTicThreadHandle
platform_thread_create(void* data,
                       thread_return_value (*thread_function)(void* data),
                       unsigned long creation_flag, unsigned long* thread_id);
void platform_thread_join(FTicThreadHandle handle);
void platform_thread_close(FTicThreadHandle handle);
void platform_thread_terminate(FTicThreadHandle handle);
void platform_interlock_exchange(volatile long* target, long value);
long platform_interlock_compare_exchange(volatile long* dest, long value,
                                         long compare);

u32 platform_get_core_count(void);

