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
    b8 clicked;
    b8 double_clicked;
} HoverClickedIndex;

typedef struct UiWindow
{
    u32 id;
    V2 dimensions;
    V2 position;
    u32 index;
    f32 total_height;
    f32 end_scroll_offset;
    f32 current_scroll_offset;
    b8 area_hit;

    ScrollBar scroll_bar;
} UiWindow;

typedef struct UiWindowArray
{
    u32 size;
    u32 capacity;
    UiWindow* data;
} UiWindowArray;

typedef struct HoverClickedIndexArray
{
    u32 size;
    u32 capacity;
    HoverClickedIndex* data;
} HoverClickedIndexArray;

typedef struct AABBArrayArray
{
    u32 size;
    u32 capacity;
    AABBArray* data;
} AABBArrayArray;

typedef struct UiContext
{
    U32Array id_to_index;
    U32Array free_indices;
    UiWindowArray windows;
    AABBArrayArray window_aabbs;
    HoverClickedIndexArray window_hover_clicked_indices;
    RenderingPropertiesArray window_rendering_properties;

    V2 dimensions;
    f64 delta_time;
} UiContext;

void ui_context_create(UiContext* context);
void ui_context_begin(UiContext* context, V2 dimensions, f64 delta_time, b8 check_collisions);
void ui_context_end();

u32 ui_window_create(UiContext* context, const RenderingProperties* render);
UiWindow* ui_window_begin(u64 window_id, UiContext* context);
void ui_window_end(UiWindow* window, const f64 delta_time);
void ui_window_add_directory_list(UiWindow* window, V2 position);
void ui_window_add_folder_list(UiWindow* window, V2 position);
void ui_window_add_file_list(UiWindow* window, V2 position);
void ui_window_add_input_field(UiWindow* window, V2 position);
