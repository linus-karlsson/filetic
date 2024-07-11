#pragma once
#include "define.h"
#include "util.h"

typedef void Platform;
typedef void (*OnKeyPressedCallback)(u16 key, b8 ctrl_pressed, b8 alt_pressed);
typedef void (*OnKeyReleasedCallback)(u16 key, b8 ctrl_pressed, b8 alt_pressed);
typedef void (*OnButtonPressedCallback)(u8 key, b8 double_clicked);
typedef void (*OnButtonReleasedCallback)(u8 key);
typedef void (*OnMouseMovedCallback_)(i16 x, i16 y);
typedef void (*OnMouseWheelCallback_)(i16 z_delta);
typedef void (*OnWindowFocusedCallback)(b8 focused);
typedef void (*OnWindowResizeCallback)(u16 width, u16 height);
typedef void (*OnWindowEnterLeaveCallback)(b8 enter_leave);
typedef void (*OnKeyStrokeCallback_)(char key);

typedef struct ClientRect
{
    int left;
    int top;
    int right;
    int bottom;
} ClientRect;

typedef struct DirectoryItem
{
    u64 size;
    char* name;
    char* path;

    V2 position_before_animation;
    V2 position_after_animation;
    f32 position_animation_precent; 

    b8 position_animation_on;
    b8 rename;
} DirectoryItem;

typedef struct DirectoryItemArray
{
    u32 size;
    u32 capacity;
    DirectoryItem* data;
} DirectoryItemArray;

typedef struct Directory
{
    char* parent;
    DirectoryItemArray files;
    DirectoryItemArray sub_directories;
} Directory;

typedef struct MenuItem MenuItem;

typedef struct MenuItemArray
{
    u32 size;
    u32 capacity;
    MenuItem* data;
} MenuItemArray;

struct MenuItem
{
    i32 id;
    char* text;
    MenuItemArray submenu_items;
};

typedef struct ContextMenu
{
    void* psf_parent;
    void* pcm;
    void* pidl;
    MenuItemArray items;
} ContextMenu;


void platform_init(const char* title, u16 width, u16 height, Platform** platform);
void platform_shut_down(Platform* platform);
b8 platform_is_running(Platform* platform);
void platform_event_fire(Platform* platform);

void platform_init_drag_drop();
void platform_uninit_drag_drop();

void platform_start_drag_drop(const CharPtrArray* paths);

char* platform_get_last_error();
void platform_print_string(const char* string);
void platform_local_free(void* memory);

void platform_opengl_init(Platform* platform);
void platform_opengl_clean(Platform* platform);
void platform_opengl_swap_buffers(Platform* platform);

ClientRect platform_get_client_rect(Platform* platform);

void platform_event_fire(Platform* platform);
void platform_event_set_on_key_pressed(Platform* platform, OnKeyPressedCallback callback);
void platform_event_set_on_key_released(Platform* platform, OnKeyReleasedCallback callback);
void platform_event_set_on_button_pressed(Platform* platform, OnButtonPressedCallback callback);
void platform_event_set_on_button_released(Platform* platform, OnButtonReleasedCallback callback);
void platform_event_set_on_mouse_move(Platform* platform, OnMouseMovedCallback_ callback);
void platform_event_set_on_mouse_wheel(Platform* platform, OnMouseWheelCallback_ callback);
void platform_event_set_on_window_focused(Platform* platform, OnWindowFocusedCallback callback);
void platform_event_set_on_window_resize(Platform* platform, OnWindowResizeCallback callback);
void platform_event_set_on_window_enter_leave(Platform* platform, OnWindowEnterLeaveCallback callback);
void platform_event_set_on_key_stroke(Platform* platform, OnKeyStrokeCallback_ callback);

void platform_change_cursor(Platform* platform, u32 cursor_id);

b8 platform_directory_exists(const char* directory_path);
Directory platform_get_directory(const char* directory_path, const u32 directory_len);
void platform_reset_directory(Directory* directory);

FTicMutex platform_mutex_create(void);
void platform_mutex_lock(FTicMutex* mutex);
void platform_mutex_unlock(FTicMutex* mutex);
void platform_mutex_destroy(FTicMutex* mutex);

FTicSemaphore platform_semaphore_create(i32 initial_count, i32 max_count);
void platform_semaphore_increment(FTicSemaphore* sem, long* previous_count);
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
f64 platform_get_time(void);
void platform_sleep(u64 milliseconds);

void platform_open_file(const char* file_path);
void platform_copy_to_clipboard(const CharPtrArray* paths);
b8 platform_clipboard_is_empty();
void platform_paste_from_clipboard(CharPtrArray* paths);
void platform_paste_to_directory(const CharPtrArray* paths, const char* directory_path);
void platform_delete_files(const CharPtrArray* paths);
void platform_rename_file(const char* path, char* new_name, const u32 name_length);
void platform_show_properties(i32 x, i32 y, const char* file_path);

void platform_listen_to_directory_change(void* data);

void platform_get_executable_directory(CharArray* buffer);

void platform_context_menu_create(ContextMenu* menu, const char* path);
void platform_context_menu_destroy(ContextMenu* menu);

void platform_open_context(void* window, const char* path);
