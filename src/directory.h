#pragma once
#include "platform/platform.h"
#include "ui.h"
#include "texture.h"
#include "ftic_guid.h"
#include "thread_queue.h"

typedef enum SortBy
{
    SORT_NONE = 0,
    SORT_NAME = 1,
    SORT_SIZE = 2,
    SORT_DATE = 3,
} SortBy;

typedef struct DirectoryPage
{
    SortBy sort_by;
    u32 sort_count;
    f32 offset;
    Directory directory;
    b8 grid_view;
} DirectoryPage;

typedef struct DirectoryArray
{
    u32 size;
    u32 capacity;
    DirectoryPage* data;
} DirectoryArray;

typedef struct DirectoryHistory
{
    void* change_handle;
    u32 current_index;
    DirectoryArray history;
} DirectoryHistory;

typedef struct DirectoryTab
{
    u32 window_id;
    SafeIdTexturePropertiesArray textures;
    DirectoryHistory directory_history;
    List directory_list;
} DirectoryTab;

typedef struct DirectoryTabArray
{
    u32 size;
    u32 capacity;
    DirectoryTab* data;
} DirectoryTabArray;

typedef struct LoadThumpnailData
{
    FticGUID file_id;
    char* file_path;
    SafeIdTexturePropertiesArray* array;
    i32 size;
} LoadThumpnailData;

void load_thumpnails(void* data);

DirectoryPage* directory_current(DirectoryHistory* history);
void directory_paste_in_directory(DirectoryPage* current_directory);
void directory_reload(DirectoryPage* directory_page);
void directory_sort(DirectoryPage* directory_page);
void directory_sort_by_name(DirectoryItemArray* array);
void directory_sort_by_size(DirectoryItemArray* array);
void directory_sort_by_date(DirectoryItemArray* array);
void directory_merge_sort_by_date(DirectoryItemArray* array);
void directory_flip_array(DirectoryItemArray* array);
b8 directory_go_to(char* path, u32 length, DirectoryHistory* directory_history);
void directory_open_folder(char* folder_path, DirectoryHistory* directory_history);
void directory_move_in_history(const i32 index_add, SelectedItemValues* selected_item_values, DirectoryHistory* directory_history);
b8   directory_can_go_up(char* parent);
void directory_go_up(DirectoryHistory* directory_history);

void directory_tab_add(const char* dir, ThreadTaskQueue* task_queue, DirectoryTab* tab);
void directory_tab_clear(DirectoryTab* tab);
void directory_clear_selected_items(SelectedItemValues* selected_item_values);
void directory_remove_selected_item(SelectedItemValues* selected_item_values, const FticGUID guid);


void directory_history_update_directory_change_handle(DirectoryHistory* directory_history);
