#pragma once
#include "define.h"
#include "util.h"
#include "ftic_guid.h"

typedef enum DirectoryItemType
{
    FOLDER_DEFAULT = 0,
    FILE_DEFAULT,
    FILE_PNG,
    FILE_JPG,
    FILE_PDF,
    FILE_CPP,
    FILE_C,
    FILE_JAVA,
    FILE_OBJ,
} DirectoryItemType;

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

typedef struct PlatformTime
{
    u16 year;
    u16 month;
    u16 dayOfWeek;
    u16 day;
    u16 hour;
    u16 minute;
    u16 second;
    u16 milliseconds;
}PlatformTime;

typedef struct DirectoryItem
{
    FticGUID id;                                   
    u64 size;
    u64 last_write_time;
    char* path;

    V2 animation_offset;

    DirectoryItemType type;
    u32 texture_id;
    u16 texture_width;
    u16 texture_height;

    u16 name_offset;
    b8 reload_thumbnail;
    b8 rename;
} DirectoryItem;

char* item_name(DirectoryItem* item);
const char* item_namec(const DirectoryItem* item);

typedef struct DirectoryItemArray
{
    u32 size;
    u32 capacity;
    DirectoryItem* data;
} DirectoryItemArray;

typedef struct Directory
{
    FticGUID parent_id;
    char* parent;
    DirectoryItemArray items;
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
    u32 texture_id;
    char* text;
    MenuItemArray submenu_items;
    b8 submenu_open;
};

typedef struct ContextMenu
{
    f32 x;
    void* psf_parent;
    void* pcm;
    void* pidl;
    MenuItemArray items;
} ContextMenu;

typedef struct DirectoryChangeData
{
    void* handle;
    char* path;
    b8 changed;
}DirectoryChangeData;

i32 platform_time_compare(const PlatformTime* first, const PlatformTime* second);

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
Directory platform_get_directory(const char* directory_path, const u32 directory_len, b8 files);
void platform_reset_directory(Directory* directory, b8 delete_textures);

FTicMutex platform_mutex_create(void);
void platform_mutex_lock(FTicMutex* mutex);
void platform_mutex_unlock(FTicMutex* mutex);
void platform_mutex_destroy(FTicMutex* mutex);

FTicSemaphore platform_semaphore_create(i32 initial_count, i32 max_count);
void platform_semaphore_increment(FTicSemaphore* sem, long* previous_count);
void platform_semaphore_wait_and_decrement(FTicSemaphore* sem);
void platform_semaphore_destroy(FTicSemaphore* sem);

FTicThreadHandle platform_thread_create(void* data, thread_return_value (*thread_function)(void* data), unsigned long creation_flag, unsigned long* thread_id);
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
void platform_move_to_directory(const CharPtrArray* paths, const char* directory_path);
void platform_delete_files(const CharPtrArray* paths);
void platform_rename_file(const char* path, char* new_name, const u32 name_length);
void platform_show_properties(i32 x, i32 y, const char* file_path);

void platform_listen_to_directory_change(void* data);

void platform_set_executable_directory();
const char* platform_get_executable_directory();
u32 platform_get_executable_directory_length();

void platform_context_menu_create(ContextMenu* menu, const char* path);
void platform_context_menu_destroy(ContextMenu* menu);
void platform_context_menu_invoke_command(ContextMenu* menu, void* window, i32 command);

void platform_open_context(void* window, const char* path);
void platform_open_background_context(void* window, const char* path);

void* directory_listen_to_directory_changes(const char* path);
void directory_unlisten_to_directory_changes(void* handle);
b8 directory_look_for_directory_change(void* handle);
void platform_show_hidden_files(b8 show);

PlatformTime platform_time_from_u64(u64 time);
void platform_open_terminal(const char* path);

void platform_initialize_filter();
void platform_insert_filter_value(char* value, b8 selected);
void platform_set_filter_on(const char* value, b8 selected);
void platform_set_filter(b8 on);
void platform_set_folder_filter(b8 on);

char* platform_get_path_from_id(FticGUID id);
b8 platform_get_id_from_path(const char* path, FticGUID* id);

void platform_get_quick_access_items(CharPtrArray* paths);
