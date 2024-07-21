#pragma once
#include "define.h"
#include "rendering.h"
#include "hash_table.h"
#include "platform/platform.h"
#include "font.h"

#define UI_DEFAULT_TEXTURE 0.0f
#define UI_FONT_TEXTURE 1.0f

#define UI_ARROW_ICON_TEXTURE 2.0f
#define UI_LIST_ICON_TEXTURE 3.0f
#define UI_GRID_ICON_TEXTURE 4.0f

#define UI_CIRCLE_TEXTURE 5.0f

#define UI_FOLDER_ICON_TEXTURE 6.0f
#define UI_FILE_ICON_TEXTURE 7.0f
#define UI_FILE_PNG_ICON_TEXTURE 8.0f
#define UI_FILE_JPG_ICON_TEXTURE 9.0f
#define UI_FILE_PDF_ICON_TEXTURE 10.0f
#define UI_FILE_JAVA_ICON_TEXTURE 11.0f
#define UI_FILE_CPP_ICON_TEXTURE 12.0f
#define UI_FILE_C_ICON_TEXTURE 13.0f

#define UI_FOLDER_ICON_BIG_TEXTURE 14.0f
#define UI_FILE_ICON_BIG_TEXTURE 15.0f
#define UI_FILE_PNG_ICON_BIG_TEXTURE 16.0f
#define UI_FILE_JPG_ICON_BIG_TEXTURE 17.0f
#define UI_FILE_PDF_ICON_BIG_TEXTURE 18.0f
#define UI_FILE_JAVA_ICON_BIG_TEXTURE 19.0f
#define UI_FILE_CPP_ICON_BIG_TEXTURE 20.0f
#define UI_FILE_C_ICON_BIG_TEXTURE 21.0f

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
    u32 window_in_focus;
    U32Array windows;
    struct DockNode* children[2];
    AABB aabb;
    f32 size_ratio;
} DockNode;

#define RESIZE_NONE 0
#define RESIZE_RIGHT BIT_1
#define RESIZE_LEFT BIT_2
#define RESIZE_BOTTOM BIT_3
#define RESIZE_TOP BIT_4

typedef struct UiWindow
{
    const char* title;
    u32 id;
    V2 size;
    V2 position;
    V2 first_item_position;
    V2 top_bar_offset;

    V2 resize_pointer_offset;
    V2 resize_size_offset;
    V4 top_color;
    V4 bottom_color;

    u32 rendering_index_offset;
    u32 rendering_index_count;

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

    f32 alpha;

    b8 docked;
    b8 hide;

    b8 area_hit;
    b8 top_bar;
    b8 top_bar_pressed;
    b8 release_from_dock_space;
    b8 top_bar_hold;

    u8 resize_dragging;
    b8 resizeable;

    b8 any_holding;
    b8 size_animation_on;
    b8 position_animation_on;
    b8 closing;
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

typedef struct II32
{
    i32 first;
    i32 second;
}II32;

typedef struct List
{
    InputBuffer input;
    SelectedItemValues selected_item_values;
    DirectoryItem* item_to_change;
    AABB input_field;
    f64 input_pressed;
    b8 reload;
    b8 item_selected;
} List;

typedef struct UU32
{
    u32 first;
    u32 second;
} UU32;

InputBuffer ui_input_buffer_create();
void ui_input_buffer_delete(InputBuffer* input);
void ui_input_buffer_clear_selection(InputBuffer* input);
char* ui_input_buffer_get_selection_as_string(InputBuffer* input);
void ui_input_buffer_copy_selection_to_clipboard(InputBuffer* input);
void ui_input_buffer_erase_from_selection(InputBuffer* input);

void ui_context_create();
void ui_context_begin(const V2 dimensions, const AABB* dock_space, const f64 delta_time, const b8 check_collisions);
void ui_context_end();
void ui_context_destroy();

const FontTTF* ui_context_get_font();
f32 ui_context_get_font_pixel_height();
void ui_context_change_font_pixel_height(const f32 pixel_height);
const char* ui_context_get_font_path();
void ui_context_set_font_path(const char* new_path);

void ui_context_set_animation(b8 on);
void ui_context_set_highlight_focused_window(b8 on);

u32 ui_window_create();
UiWindow* ui_window_get(const u32 window_id);
u32 ui_window_in_focus();
b8 ui_window_begin(u32 window_id, const char* title, b8 top_bar, b8 resizeable);
b8 ui_window_end();

void ui_window_set_end_scroll_offset(const u32 window_id, const f32 offest);
void ui_window_set_current_scroll_offset(const u32 window_id, const f32 offset);

void ui_window_close(u32 window_id);

void ui_window_row_begin(const f32 padding);
f32 ui_window_row_end();
void ui_window_column_begin(const f32 padding);
f32 ui_window_column_end();

void ui_window_start_size_animation(UiWindow* window, V2 start_size, V2 end_size);
void ui_window_start_position_animation(UiWindow* window, V2 start_position, V2 end_position);

b8 ui_window_add_icon_button(V2 position, const V2 size, const V4 hover_color, const V4 texture_coordinates, const f32 texture_index, const b8 disable);
b8 ui_window_add_button(V2 position, V2* dimensions, const V4* color, const char* text);
b8 ui_window_add_folder_list(V2 position, const f32 item_height, DirectoryItemArray* items, List* list, i32* double_clicked_index);
b8 ui_window_add_file_list(V2 position, const f32 item_height, DirectoryItemArray* items, List* list, i32* double_clicked_index);
II32 ui_window_add_directory_item_grid(V2 position, const DirectoryItemArray* folders, const DirectoryItemArray* files, const f32 item_height, List* list);
b8 ui_window_add_input_field(V2 position, const V2 size, InputBuffer* input);
b8 ui_window_add_drop_down_menu(V2 position, DropDownMenu* drop_down_menu, void* option_data);
void ui_window_add_text(V2 position, const char* text, b8 scrolling);
void ui_window_add_text_colored(V2 position, const ColoredCharacterArray* text, b8 scrolling);
b8 ui_window_set_overlay();
void ui_window_add_image(V2 position, V2 image_dimensions, u32 image);
i32 ui_window_add_menu_bar(CharPtrArray* values, V2* position_of_clicked_item);
void ui_window_add_icon(V2 position, const V2 size, const V4 texture_coordinates, const f32 texture_index);
void ui_window_add_switch(V2 position, b8* selected, f32* x);
b8 ui_window_add_drop_down(V2 position, b8* open);
