#pragma once
#include "define.h"
#include "ftic_window.h"
#include "font.h"
#include "platform/platform.h"
#include "hash_table.h"
#include "event.h"
#include "shader.h"

typedef struct SelectedItemValues
{
    CharPtrArray paths;
    HashTableCharU32 selected_items;
} SelectedItemValues;

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

DirectoryPage* current_directory(DirectoryHistory* history);

typedef struct DirectoryTab
{
    DirectoryHistory directory_history;
    SelectedItemValues selected_item_values;
} DirectoryTab;

typedef struct DirectoryTabArray
{
    u32 size;
    u32 capacity;
    DirectoryTab* data;
} DirectoryTabArray;

typedef struct ApplicationContext
{
    FTicWindow* window;
    FontTTF font;

    u32 tab_index;
    DirectoryTabArray tabs;

    V2 dimensions;
    V2 mouse_position;

    f64 last_time;
    f64 delta_time;
    MVP mvp;

    f64 last_moved_time;
} ApplicationContext;

u8* application_initialize(ApplicationContext* application);
void application_uninitialize(ApplicationContext* application);
void application_begin_frame(ApplicationContext* application);
void application_end_frame(ApplicationContext* application);
f64 application_get_last_mouse_move_time(const ApplicationContext* appliction);
DirectoryTab tab_add();
void reset_selected_items(SelectedItemValues* selected_item_values);
