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

#define RESIZE_NONE 0
#define RESIZE_RIGHT BIT_1

typedef struct UiWindow
{
    u32 id;
    i32 docking_id;
    V2 dimensions;
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

    b8 area_hit;
    b8 top_bar;
    b8 top_bar_pressed;
    b8 top_bar_hold;
    b8 hide;
    u8 resize_dragging;
    u8 resizeable;

} UiWindow;

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
void ui_window_end(const f64 delta_time);
b8 ui_window_add_icon_button(V2 position, const V2 size, const V4 texture_coordinates, const f32 texture_index, const b8 disable);
void ui_window_add_directory_list(V2 position);
b8 ui_window_add_folder_list(V2 position, const f32 item_height, const DirectoryItemArray* items, SelectedItemValues* selected_item_values, i32* item_selected);
b8 ui_window_add_file_list(V2 position, const f32 item_height, const DirectoryItemArray* items, SelectedItemValues* selected_item_values, i32* item_selected);
b8 ui_window_add_input_field(V2 position, const V2 size, const f64 delta_time, InputBuffer* input);
