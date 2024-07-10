#pragma once
#include "platform/platform.h"
#include "ui.h"

typedef enum SortBy
{
    SORT_NONE = 0,
    SORT_NAME = 1,
    SORT_SIZE = 2,
} SortBy;

typedef struct DirectoryPage
{
    SortBy sort_by;
    u32 sort_count;
    f32 offset;
    f32 scroll_offset;
    f64 pulse_x;
    Directory directory;
} DirectoryPage;

typedef struct DirectoryArray
{
    u32 size;
    u32 capacity;
    DirectoryPage* data;
} DirectoryArray;

typedef struct DirectoryHistory
{
    u32 current_index;
    DirectoryArray history;
} DirectoryHistory;

typedef struct DirectoryTab
{
    u32 window_id;
    DirectoryHistory directory_history;
    List directory_list;
    b8 list_view;
} DirectoryTab;

typedef struct DirectoryTabArray
{
    u32 size;
    u32 capacity;
    DirectoryTab* data;
} DirectoryTabArray;

DirectoryPage* directory_current(DirectoryHistory* history);
void directory_paste_in_directory(DirectoryPage* current_directory);
void directory_reload(DirectoryPage* directory_page);
void directory_sort(DirectoryPage* directory_page);
void directory_sort_by_name(DirectoryItemArray* array);
void directory_sort_by_size(DirectoryItemArray* array);
void directory_flip_array(DirectoryItemArray* array);
b8   directory_go_to(char* path, u32 length, DirectoryHistory* directory_history);
void directory_open_folder(char* folder_path, DirectoryHistory* directory_history);
void directory_move_in_history(const i32 index_add, SelectedItemValues* selected_item_values, DirectoryHistory* directory_history);
b8   directory_can_go_up(char* parent);
void directory_go_up(DirectoryHistory* directory_history);

DirectoryTab directory_tab_add(const char* dir);
void directory_tab_clear(DirectoryTab* tab);
void directory_clear_selected_items(SelectedItemValues* selected_item_values);
void directory_remove_selected_item(SelectedItemValues* selected_item_values, const char* path);

