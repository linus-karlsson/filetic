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


void ui_context_create();
void ui_context_begin(V2 dimensions, f64 delta_time, b8 check_collisions);
void ui_context_end();

u32 ui_window_create();
void ui_window_begin(u32 window_id, b8 top_bar);
void ui_window_end(const f64 delta_time);
void ui_window_add_directory_list(V2 position);
void ui_window_add_folder_list(V2 position);
void ui_window_add_file_list(V2 position);
void ui_window_add_input_field(V2 position);
