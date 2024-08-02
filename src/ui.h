#pragma once
#include "define.h"
#include "rendering.h"
#include "hash_table.h"
#include "platform/platform.h"
#include "font.h"
#include "thread_queue.h"
#include "texture.h"

#define UI_DEFAULT_TEXTURE 0.0f
#define UI_FONT_TEXTURE 1.0f

#define UI_ARROW_ICON_TEXTURE 2.0f
#define UI_LIST_ICON_TEXTURE 3.0f
#define UI_GRID_ICON_TEXTURE 4.0f

#define UI_CIRCLE_TEXTURE 5.0f
#define UI_COLOR_PICKER_TEXTURE 6.0f

#define UI_FOLDER_ICON_TEXTURE 7.0f
#define UI_FILE_ICON_TEXTURE 8.0f
#define UI_FILE_PNG_ICON_TEXTURE 9.0f
#define UI_FILE_JPG_ICON_TEXTURE 10.0f
#define UI_FILE_PDF_ICON_TEXTURE 11.0f
#define UI_FILE_JAVA_ICON_TEXTURE 12.0f
#define UI_FILE_CPP_ICON_TEXTURE 13.0f
#define UI_FILE_C_ICON_TEXTURE 14.0f

#define UI_FOLDER_ICON_BIG_TEXTURE 15.0f
#define UI_FILE_ICON_BIG_TEXTURE 16.0f
#define UI_FILE_PNG_ICON_BIG_TEXTURE 17.0f
#define UI_FILE_JPG_ICON_BIG_TEXTURE 18.0f
#define UI_FILE_PDF_ICON_BIG_TEXTURE 19.0f
#define UI_FILE_JAVA_ICON_BIG_TEXTURE 20.0f
#define UI_FILE_CPP_ICON_BIG_TEXTURE 21.0f
#define UI_FILE_C_ICON_BIG_TEXTURE 22.0f
#define UI_FILE_OBJ_ICON_TEXTURE 23.0f

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
    U32Array windows;
    struct DockNode* children[2];
    AABB aabb;

    u32 window_in_focus;
    f32 size_ratio;
} DockNode;

#define RESIZE_NONE 0
#define RESIZE_RIGHT BIT_1
#define RESIZE_LEFT BIT_2
#define RESIZE_BOTTOM BIT_3
#define RESIZE_TOP BIT_4

#define SCROLL_BAR_NONE 0
#define SCROLL_BAR_HEIGHT BIT_1
#define SCROLL_BAR_WIDTH BIT_2

#define UI_WINDOW_NONE 0
#define UI_WINDOW_TOP_BAR BIT_1
#define UI_WINDOW_RESIZEABLE BIT_2
#define UI_WINDOW_OVERLAY BIT_3
#define UI_WINDOW_FROSTED_GLASS BIT_4
#define UI_WINDOW_DOCKED BIT_5
#define UI_WINDOW_HIDE BIT_6
#define UI_WINDOW_AREA_HIT BIT_7
#define UI_WINDOW_CLOSING BIT_8

typedef struct UiWindow
{
    CharArray title;
    DockNode* dock_node;

    u32 id;
    V2 size;
    V2 position;

    f32 end_scroll_offset;
    f32 current_scroll_offset;

    f32 end_scroll_offset_width;
    f32 current_scroll_offset_width;

    f32 scroll_bar_mouse_pointer_offset;
    f32 alpha;

    u8 scroll_bar_dragging;
    u8 flags;
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

    f32 time;
    f32 selected_pivot_point;
    i32 input_index;
    b8 selected;
    b8 active;
} InputBuffer;

typedef struct InputBufferArray
{
    u32 size;
    u32 capacity;
    InputBuffer* data;
} InputBufferArray;

typedef struct DropDownMenu
{
    f32 x;
    i32 tab_index;
    CharPtrArray options;
    b8 (*menu_options_selection)(u32 index, b8 hit, b8 should_close, b8 item_clicked, V4* text_color, void* data);
} DropDownMenu;

typedef struct II32
{
    i32 first;
    i32 second;
} II32;

typedef struct List
{
    SelectedItemValues selected_item_values;
    InputBufferArray inputs;
    f64 input_pressed;
    i32 input_index;
    b8 item_selected;
} List;

typedef struct UU32
{
    u32 first;
    u32 second;
} UU32;

typedef struct MovableList
{
    V2 pressed_offset;
    V2 hold_position;
    i32 selected_item;
    b8 right_click;
    b8 pressed;
    b8 hold;
    b8 selected;
} MovableList;

typedef struct ColorPicker
{
    V2 at;
    f32 spectrum_at;
    b8 hold;
    b8 spectrum_hold;
} ColorPicker;

typedef struct UiLayout
{
    V2 at;
    f32 start_x;
    f32 row_height;
    f32 column_width;
    f32 padding;
} UiLayout;

UiLayout ui_layout_create(V2 at);
void ui_layout_row(UiLayout* layout);
void ui_layout_column(UiLayout* layout);
void ui_layout_reset_column(UiLayout* layout);

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

f32 ui_get_big_icon_size();
void ui_set_big_icon_size(f32 new_size);
void ui_set_big_icon_min_size(f32 new_min);
void ui_set_big_icon_max_size(f32 new_max);
V2 ui_get_big_icon_min_max();
void ui_set_list_padding(const f32 padding);
f32 ui_get_list_padding();
void ui_set_frosted_glass(b8 on);
f32 ui_get_frosted_blur_amount();
void ui_set_frosted_blur_amount(const f32 new_blur_amount);

const FontTTF* ui_context_get_font();
f32 ui_context_get_font_pixel_height();
void ui_context_change_font_pixel_height(const f32 pixel_height);
const char* ui_context_get_font_path();
void ui_context_set_font_path(const char* new_path);

void ui_context_set_animation(b8 on);
void ui_context_set_highlight_focused_window(b8 on);

u32 ui_window_create();
const UiWindow* ui_window_get(const u32 window_id);
u32 ui_window_in_focus();
b8 ui_window_begin(u32 window_id, const char* title, u8 flags);
b8 ui_window_end();

void ui_window_set_end_scroll_offset(const u32 window_id, const f32 offest);
void ui_window_set_current_scroll_offset(const u32 window_id, const f32 offset);

void ui_window_close(u32 window_id);
void ui_window_close_current();

void ui_window_set_size(u32 window_id, const V2 size);
void ui_window_set_position(u32 window_id, const V2 position);

void ui_window_row_begin(const f32 padding);
f32 ui_window_row_end();
void ui_window_column_begin(const f32 padding);
f32 ui_window_column_end();

void ui_window_start_size_animation(const u32 window_id, const V2 end_size);
void ui_window_start_position_animation(const u32 window_id, const V2 end_position);
void ui_window_dock_space_size(const u32 window_id, const V2 end_size);
void ui_window_dock_space_min(const u32 window_id, const V2 end_position);
void ui_window_set_animation_x(const u32 window_id, const f32 x);

void ui_window_set_alpha(const u32 window_id, const f32 alpha);

b8 ui_window_add_icon_button(V2 position, const V2 size, const V4 hover_color, const V4 texture_coordinates, const f32 texture_index, const b8 disable, UiLayout* layout);
V2 ui_window_get_button_dimensions(V2 dimensions, const char* text, f32* x_advance_out);
b8 ui_window_add_button(V2 position, V2* dimensions, const V4* color, const char* text, UiLayout* layout);
b8 ui_window_add_input_field(V2 position, const V2 size, InputBuffer* input, UiLayout* layout);
void ui_window_add_text(V2 position, const char* text, b8 scrolling, UiLayout* layout);
void ui_window_add_text_c(V2 position, V4 color, const char* text, b8 scrolling, UiLayout* layout);
void ui_window_add_text_colored(V2 position, const ColoredCharacterArray* text, b8 scrolling, UiLayout* layout);
void ui_window_add_image(V2 position, V2 image_dimensions, u32 image, UiLayout* layout);
i32 ui_window_add_menu_bar(CharPtrArray* values, V2* position_of_clicked_item);
void ui_window_add_icon(V2 position, const V2 size, const V4 texture_coordinates, const f32 texture_index, UiLayout* layout);
V2 ui_window_get_switch_size();
void ui_window_add_switch(V2 position, b8* selected, f32* x, UiLayout* layout);

f32 ui_window_add_slider(V2 position, V2 size, const f32 min_value, const f32 max_value, f32 value, b8* pressed, UiLayout* layout);
V4 ui_window_add_color_picker(V2 position, V2 size, ColorPicker* picker, UiLayout* layout);
void ui_window_add_border(V2 position, const V2 size, const V4 color, const f32 thickness);
void ui_window_add_rectangle(V2 position, const V2 size, const V4 color, UiLayout* layout);
void ui_window_add_radio_button(V2 position, const V2 size, b8* selected, UiLayout* layout);

// (NOTE): this is very specific for this project and maybe should be implemented outside this ui.
b8 ui_window_add_movable_list(V2 position, DirectoryItemArray* items, MovableList* list);
b8 ui_window_add_folder_list(V2 position, const f32 item_height, DirectoryItemArray* items, List* list, i32* double_clicked_index, UiLayout* layout);
b8 ui_window_add_file_list(V2 position, const f32 item_height, DirectoryItemArray* items, List* list, i32* double_clicked_index, UiLayout* layout);
II32 ui_window_add_directory_item_grid(V2 position, const DirectoryItemArray* folders, const DirectoryItemArray* files, ThreadTaskQueue* task_queue, SafeIdTexturePropertiesArray* textures, SafeObjectThumbnailArray* objects, List* list);
