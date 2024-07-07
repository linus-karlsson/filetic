#pragma once
#include "define.h"
#include "rendering.h"
#include "hash_table.h"
#include "platform/platform.h"
#include "font.h"

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

typedef enum NodeType
{
    NODE_ROOT,
    NODE_PARENT,
    NODE_LEAF
} NodeType;

typedef enum SplitAxis
{
    SPLIT_NONE,
    SPLIT_HORIZONTAL,
    SPLIT_VERTICAL
} SplitAxis;

typedef struct DockNode
{
    NodeType type;
    SplitAxis split_axis;
    i32 window;
    struct DockNode* children[2];
    AABB aabb;
    f32 size_ratio;
}DockNode;

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

    u32 rendering_index_offset;
    u32 rendering_index_count;

    u32 related_window;

    V2 position_before_animation;
    V2 position_after_animation;
    f32 position_animation_precent; 

    V2 size_before_animation;
    V2 size_after_animation;
    f32 size_animation_precent; 

    f32 total_height;
    f32 end_scroll_offset;
    f32 current_scroll_offset;
    ScrollBar scroll_bar;

    f32 total_width;
    f32 end_scroll_offset_width;
    f32 current_scroll_offset_width;
    ScrollBar scroll_bar_width;

    DockNode* dock_node;

    b8 docked;

    b8 area_hit;
    b8 top_bar;
    b8 top_bar_pressed;
    b8 top_bar_hold;
    b8 hide;
    u8 resize_dragging;
    u8 resizeable;

    b8 any_holding;
    b8 size_animation_on;
    b8 position_animation_on;

} UiWindow;

#define DOCK_SIDE_RIGHT 0
#define DOCK_SIDE_LEFT 1

#define DOCK_SIDE_BOTTOM 0
#define DOCK_SIDE_TOP 1


typedef struct InputBuffer
{
    CharArray buffer;
    SelectionCharacterArray chars;
    SelectionCharacterArray chars_selected;

    i32 start_selection_index;
    i32 end_selection_index;

    f32 selected_pivot_point;
    f64 time;
    i32 input_index;
    b8 selected;
    b8 active;
} InputBuffer;

typedef struct DropDownMenu
{
    f32 x;
    i32 tab_index;
    CharPtrArray options;
    b8 (*menu_options_selection)(u32 index, b8 hit, b8 should_close,
                                 b8 item_clicked, V4* text_color, void* data);
} DropDownMenu;

InputBuffer ui_input_buffer_create();
void ui_input_buffer_clear_selection(InputBuffer* input);
char* ui_input_buffer_get_selection_as_string(InputBuffer* input);
void ui_input_buffer_copy_selection_to_clipboard(InputBuffer* input);
void ui_input_buffer_erase_from_selection(InputBuffer* input);

void ui_context_create();
void ui_context_begin(const V2 dimensions, const AABB* dock_space, const f64 delta_time, const b8 check_collisions);
void ui_context_end();
void ui_context_destroy();

void ui_dock_space_begin(V2 position, V2 dimensions, u32* windows, u32 window_count);
void ui_dock_space_end();

u32 ui_window_create();
UiWindow* ui_window_get(u32 window_id);
u32 ui_window_in_focus();
b8 ui_window_begin(u32 window_id, b8 top_bar);
b8 ui_window_end(const char* title, b8 closable);

void ui_window_row_begin(const f32 padding);
f32 ui_window_row_end();

b8 ui_window_add_icon_button(V2 position, const V2 size, const V4 texture_coordinates, const f32 texture_index, const b8 disable);
b8 ui_window_add_button(V2 position, V2* dimensions, const V4* color, const char* text);
void ui_window_add_directory_list(V2 position);
b8 ui_window_add_folder_list(V2 position, const f32 item_height, const DirectoryItemArray* items, SelectedItemValues* selected_item_values, i32* item_selected);
b8 ui_window_add_file_list(V2 position, const f32 item_height, const DirectoryItemArray* items, SelectedItemValues* selected_item_values, i32* item_selected);
b8 ui_window_add_input_field(V2 position, const V2 size, InputBuffer* input);
b8 ui_window_add_drop_down_menu(V2 position, DropDownMenu* drop_down_menu, void* option_data);
void ui_window_add_text(V2 position, const char* text, b8 scrolling);
b8 ui_window_set_overlay();
void ui_window_add_image(V2 position, V2 image_dimensions, u32 image);
i32 ui_window_add_menu_bar(CharPtrArray* values, V2* position_of_clicked_item);
void ui_window_add_icon(V2 position, const V2 size, const V4 texture_coordinates, const f32 texture_index);

DockNode* dock_node_create(NodeType type, SplitAxis split_axis, i32 window);
void dock_node_dock_window(DockNode* root, DockNode* window,
                           SplitAxis split_axis, u8 where);
void dock_node_resize_from_root(DockNode* root, const AABB* aabb);
void dock_node_remove_node(DockNode* root, DockNode* node_to_remove);
