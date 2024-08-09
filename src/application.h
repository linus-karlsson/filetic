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
#include "camera.h"

#define COPY_OPTION_INDEX 0
#define PASTE_OPTION_INDEX 1
#define DELETE_OPTION_INDEX 2
#define ADD_TO_QUICK_OPTION_INDEX 3
#define OPEN_IN_TERMINAL_OPTION_INDEX 4
#define PROPERTIES_OPTION_INDEX 5
#define MORE_OPTION_INDEX 6
#define CONTEXT_ITEM_COUNT 7

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

typedef struct WindowOpenMenuItem
{
    u32 window;
    f32 switch_x;

    b8 switch_on;
    b8 show;
} WindowOpenMenuItem;

typedef struct AccessPanel
{
    DirectoryItemArray items;
    WindowOpenMenuItem menu_item;
    MovableList list;
} AccessPanel;

typedef struct MainDropDownSelectionData
{
    DirectoryPage* directory;
    DirectoryItemArray* quick_access;
    const CharPtrArray* selected_paths;
    AccessPanel* panel;
    FTicWindow* window;
    ThreadTaskQueue* task_queue;
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
    b8 (*menu_options_selection)(u32 index, b8 hit, b8 should_close, b8 item_clicked,
                                 V4* text_color, void* data);
} DropDownMenu2;

typedef struct RecentPanel
{
    AccessPanel panel;
    u32 total;
} RecentPanel;

typedef struct PreviewImage
{
    V2 dimensions;
    char* current_viewed_path;
    U32Array textures;
} PreviewImage;

typedef struct PreviewTextFile
{
    FileAttrib file;
    ColoredCharacterArray file_colored;
} PreviewTextFile;

typedef struct FilterOption
{
    char* value;
    b8 selected;
} FilterOption;

typedef struct FilterOptionArray
{
    u32 size;
    u32 capacity;
    FilterOption* data;
} FilterOptionArray;

typedef struct Filter
{
    InputBuffer buffer;
    FilterOptionArray options;
    b8 on;
} Filter;


typedef struct ApplicationContext
{
    FTicWindow* window;
    FontTTF font;
    ThreadQueue thread_queue;

    CharPtrArray menu_values;

    u32 tab_index;
    i32 tab_in_fucus;
    DirectoryTab* current_tab;
    DirectoryTabArray tabs;
    char* item_hit;

    V2 dimensions;
    V2 mouse_position;
    V2 last_mouse_position;
    f32 mouse_drag_distance;

    f64 last_time;
    f64 delta_time;
    MVP mvp;

    f64 last_moved_time;

    CharPtrArray pasted_paths;

    SearchPage search_page;
    RenderingProperties main_render;
    u32 main_index_count;
    WindowOpenMenuItem search_result_window_item;

    Render render_3d;
    u32 index_count_3d;
    int light_dir_location;

    U32Array free_window_ids;
    U32Array windows;
    U32Array tab_windows;
    u32 current_tab_window_index;

    u32 top_bar_window;
    u32 bottom_bar_window;
    u32 preview_window;
    u32 menu_window;
    u32 windows_window;
    u32 font_change_window;
    u32 context_menu_window;
    u32 color_picker_window;
    u32 style_menu_window;
    u32 filter_menu_window;
    u32 menu_bar_window;

    f32 context_menu_x;

    Directory font_change_directory;

    i32 preview_index;
    PreviewImage preview_image;
    PreviewTextFile preview_text;
    Camera preview_camera;
    AABB3D preview_mesh_aabb;

    AccessPanel* panel_right_clicked;

    i32 drop_down_tab_index;

    CharPtrArray context_menu_options;

    ThemeColorPicker picker;
    V4 secondary_color;
    V4 clear_color;
    V4 text_color;
    V4 tab_color;
    V4 bar_top_color;
    V4 bar_bottom_color;
    V4 border_color;
    V4 scroll_bar_color;
    V2 color_picker_position;
    ColorPicker* color_picker_to_use;
    V4* color_to_change;

    AccessPanel quick_access;
    RecentPanel recent;

    Filter filter;

    InputBuffer parent_directory_input;
    DropDownMenu2 suggestions;
    SuggestionSelectionData suggestion_data;

    ContextMenu context_menu;
    f32 context_menu_open_position_y;

    b8 use_shortcuts_for_tabs;
    b8 show_hidden_files;

    b8 open_font_change_window;
    b8 open_menu_window;
    b8 open_windows_window;
    b8 open_context_menu_window;
    b8 open_style_menu_window;
    b8 open_filter_menu_window;
    b8 open_color_picker_window;

    b8 check_collision_in_ui;
    b8 menu_open_this_frame;

} ApplicationContext;

void application_run();

void application_initialize(ApplicationContext* application);
void application_uninitialize(ApplicationContext* application);
void application_begin_frame(ApplicationContext* application);
void application_end_frame(ApplicationContext* application);
f64 application_get_last_mouse_move_time(const ApplicationContext* appliction);

void search_page_clear_result(SearchPage* page);
b8 search_page_has_result(const SearchPage* search_page);
void search_page_search(SearchPage* page, DirectoryHistory* directory_history, ThreadTaskQueue* thread_task_queue);
