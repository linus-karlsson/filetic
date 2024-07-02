#pragma once
#include "define.h"
#include "rendering.h"
#include "hash_table.h"
#include "platform/platform.h"

typedef struct ScrollBar
{
    f32 mouse_pointer_offset;
    b8 dragging;
} ScrollBar;

typedef struct HoverClickedIndex
{
    i32 index;
    b8 hover;
    b8 pressed;
    b8 clicked;
    b8 double_clicked;
} HoverClickedIndex;

typedef struct DockNode DockNode;

#define RESIZE_NONE 0
#define RESIZE_RIGHT BIT_1

typedef struct UiWindow
{
    u32 id;
    V2 size;
    V2 position;
    V2 first_item_position;
    V2 top_bar_offset;
    V2 resize_offset;
    V2 last_resize_offset;
    V2 resize_pointer_offset;
    V4 top_color;
    V4 bottom_color;
    u32 index;

    u32 rendering_index_offset;
    u32 rendering_index_count;

    f32 total_height;
    f32 end_scroll_offset;
    f32 current_scroll_offset;
    ScrollBar scroll_bar;

    DockNode* dock_node;

    b8 docked;

    b8 area_hit;
    b8 top_bar;
    b8 top_bar_pressed;
    b8 top_bar_hold;
    b8 hide;
    u8 resize_dragging;
    u8 resizeable;

} UiWindow;

#define DOCK_SIDE_RIGHT 0
#define DOCK_SIDE_LEFT 1

#define DOCK_SIDE_BOTTOM 0
#define DOCK_SIDE_TOP 1

typedef enum
{
    NODE_ROOT,
    NODE_PARENT,
    NODE_LEAF
} NodeType;

typedef enum
{
    SPLIT_NONE,
    SPLIT_HORIZONTAL,
    SPLIT_VERTICAL
} SplitAxis;

struct DockNode
{
    NodeType type;
    SplitAxis split_axis;
    UiWindow* window;
    DockNode* children[2];
    V2 size;
    V2 position;
};

typedef struct InputBuffer
{
    CharArray buffer;
    f64 time;
    i32 input_index;
    b8 active;
} InputBuffer;

InputBuffer ui_input_buffer_create();

void ui_context_create();
void ui_context_begin(V2 dimensions, f64 delta_time, b8 check_collisions);
void ui_context_end();

void ui_dock_space_begin(V2 position, V2 dimensions, u32* windows, u32 window_count);
void ui_dock_space_end();

u32 ui_window_create();
UiWindow* ui_window_get(u32 window_id);
b8 ui_window_begin(u32 window_id, b8 top_bar);
void ui_window_end();

void ui_window_row_begin(const f32 padding);
f32 ui_window_row_end();

b8 ui_window_add_icon_button(V2 position, const V2 size, const V4 texture_coordinates, const f32 texture_index, const b8 disable);
void ui_window_add_directory_list(V2 position);
b8 ui_window_add_folder_list(V2 position, const f32 item_height, const DirectoryItemArray* items, SelectedItemValues* selected_item_values, i32* item_selected);
b8 ui_window_add_file_list(V2 position, const f32 item_height, const DirectoryItemArray* items, SelectedItemValues* selected_item_values, i32* item_selected);
b8 ui_window_add_input_field(V2 position, const V2 size, InputBuffer* input);

DockNode* dock_node_create(NodeType type, SplitAxis split_axis, UiWindow* window);
void dock_node_dock_window(DockNode* root, DockNode* window, SplitAxis split_axis, u8 where);
void dock_node_resize_from_root(DockNode* root, const V2 size);
