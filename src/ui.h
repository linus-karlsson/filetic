#pragma once
#include "define.h"
#include "rendering.h"
#include "hash_table.h"

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

typedef struct UiWindow
{
    u32 id;
    i32 docking_id;
    V2 dimensions;
    V2 position;
    V2 first_item_position;
    V2 top_bar_offset;
    V4 top_color;
    V4 bottom_color;
    u32 index;

    u32 rendering_index_offset;
    u32 rendering_index_count;

    f32 total_height;
    f32 end_scroll_offset;
    f32 current_scroll_offset;

    b8 area_hit;
    b8 top_bar;
    b8 top_bar_pressed;
    b8 top_bar_hold;

    ScrollBar scroll_bar;
} UiWindow;

typedef struct InputBuffer
{
    CharArray buffer;
    f64 time;
    i32 input_index;
    b8 active;
} InputBuffer;

void ui_context_create();
void ui_context_begin(V2 dimensions, f64 delta_time, b8 check_collisions);
void ui_context_end();

u32 ui_window_create();
UiWindow* ui_window_get(u32 window_id);

void ui_window_begin(u32 window_id, b8 top_bar);
void ui_window_end(const f64 delta_time);
b8 ui_window_add_icon_button(V2 position, const V2 size, const V4 texture_coordinates, const f32 texture_index, const b8 disable);
void ui_window_add_directory_list(V2 position);
void ui_window_add_folder_list(V2 position);
void ui_window_add_file_list(V2 position);
void ui_window_add_input_field(V2 position, const V2 size, const f64 delta_time, InputBuffer* input);
void ui_window_add_input_field_width_suggestions(V2 position, CharArray* input);
