#pragma once
#include "define.h"
#include "ftic_window.h"
#include "font.h"
#include "platform/platform.h"
#include "hash_table.h"
#include "event.h"
#include "shader.h"
#include "ui.h"
#include "thread_queue.h"
#include "directory.h"

#define COPY_OPTION_INDEX 0
#define PASTE_OPTION_INDEX 1
#define DELETE_OPTION_INDEX 2
#define ADD_TO_QUICK_OPTION_INDEX 3
#define PROPERTIES_OPTION_INDEX 4

typedef struct SafeFileArray
{
    DirectoryItemArray array;
    FTicMutex mutex;
} SafeFileArray;

typedef struct B8PtrArray
{
    u32 size;
    u32 capacity;
    b8** data;
} B8PtrArray;

typedef struct DirectoryItemList
{
    DirectoryItemArray* items;
    f64 pulse_x;
} DirectoryItemList;

typedef struct SearchPage
{
    InputBuffer input;
    SafeFileArray search_result_file_array;
    SafeFileArray search_result_folder_array;

    u32 running_id;
    u32 last_running_id;
    b8 running_callbacks[100];
} SearchPage;

typedef struct FindingCallbackAttribute
{
    ThreadTaskQueue* thread_queue;
    char* start_directory;
    const char* string_to_match;
    u32 start_directory_length;
    u32 string_to_match_length;
    u32 running_id;
    SafeFileArray* file_array;
    SafeFileArray* folder_array;
    b8* running_callbacks;
} FindingCallbackAttribute;

typedef struct MainDropDownSelectionData
{
    DirectoryPage* directory;
    DirectoryItemArray* quick_access;
    const CharPtrArray* selected_paths;
    b8 show_hidden_files;
} MainDropDownSelectionData;

typedef struct SuggestionSelectionData
{
    CharArray* parent_directory;
    DirectoryItemArray items;
    i32* cursor_index;
    i32* tab_index;
    b8 change_directory;
} SuggestionSelectionData;

typedef struct DropDownMenu2
{
    f32 x;
    i32 tab_index;
    V2 position;
    AABB aabb;
    CharPtrArray options;
    RenderingProperties* render;
    u32* index_count;
    b8 (*menu_options_selection)(u32 index, b8 hit, b8 should_close,
                                 b8 item_clicked, V4* text_color, void* data);
} DropDownMenu2;

typedef struct ApplicationContext
{
    FTicWindow* window;
    FontTTF font;
    ThreadQueue thread_queue;

    u32 tab_index;
    DirectoryTabArray tabs;

    V2 dimensions;
    V2 mouse_position;

    f64 last_time;
    f64 delta_time;
    MVP mvp;

    f64 last_moved_time;

    CharPtrArray pasted_paths;

    SearchPage search_page;
    RenderingProperties main_render;
    u32 main_index_count;
    b8 show_search_page;

    Render render_3d;
    u32 index_count_3d;

    U32Array free_window_ids;
    U32Array windows;
    U32Array tab_windows;
    u32 current_tab_window_index;

    u32 top_bar_window;
    u32 bottom_bar_window;
    u32 quick_access_window;
    u32 search_result_window;
    u32 preview_window;
    u32 menu_window;
    u32 windows_window;
    u32 font_change_window;

    Directory font_change_directory;

    b8 open_font_change_window;
    b8 open_menu_window;
    b8 open_windows_window;

    DropDownMenu2 context_menu;
    b8 context_menu_open;

    DirectoryItemArray quick_access_folders;
    b8 show_quick_access;

    InputBuffer parent_directory_input;
    DropDownMenu2 suggestions;
    SuggestionSelectionData suggestion_data;

    b8 show_hidden_files;
} ApplicationContext;

void application_run();

u8* application_initialize(ApplicationContext* application);
void application_uninitialize(ApplicationContext* application);
void application_begin_frame(ApplicationContext* application);
void application_end_frame(ApplicationContext* application);
f64 application_get_last_mouse_move_time(const ApplicationContext* appliction);

void search_page_clear_result(SearchPage* page);
b8 search_page_has_result(const SearchPage* search_page);
void search_page_search(SearchPage* page, DirectoryHistory* directory_history, ThreadTaskQueue* thread_task_queue);
