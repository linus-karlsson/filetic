#include "ui.h"
#include "event.h"
#include "application.h"
#include "opengl_util.h"
#include "texture.h"
#include "logging.h"
#include <string.h>
#include <stdio.h>
#include <glad/glad.h>

#define icon_1_co(width, height)                                               \
    {                                                                          \
        .x = 0.0f / width,                                                     \
        .y = 0.0f / height,                                                    \
        .z = 128.0f / width,                                                   \
        .w = 128.0f / height,                                                  \
    };

global const V4 file_icon_co = icon_1_co(512.0f, 128.0f);
global const V4 arrow_back_icon_co = icon_1_co(384.0f, 128.0f);

#define icon_2_co(width, height)                                               \
    {                                                                          \
        .x = 128.0f / width,                                                   \
        .y = 0.0f / height,                                                    \
        .z = 256.0f / width,                                                   \
        .w = 128.0f / height,                                                  \
    };
global const V4 folder_icon_co = icon_2_co(512.0f, 128.0f);
global const V4 arrow_up_icon_co = icon_2_co(384.0f, 128.0f);

#define icon_3_co(width, height)                                               \
    {                                                                          \
        .x = 256.0f / width,                                                   \
        .y = 0.0f / height,                                                    \
        .z = 384.0f / width,                                                   \
        .w = 128.0f / height,                                                  \
    };
global const V4 pdf_icon_co = icon_3_co(512.0f, 128.0f);
global const V4 arrow_right_icon_co = icon_3_co(384.0f, 128.0f);

#define icon_4_co(width, height)                                               \
    {                                                                          \
        .x = 384.0f / width,                                                   \
        .y = 0.0f / height,                                                    \
        .z = 512.0f / width,                                                   \
        .w = 128.0f / height,                                                  \
    };
global const V4 png_icon_co = icon_4_co(512.0f, 128.0f);

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

typedef struct Dock
{
    u32 id;
    AABB aabb;
} Dock;

typedef struct DockArray
{

    u32 size;
    u32 capacity;
    Dock* data;
} DockArray;

typedef struct V2Array
{
    u32 size;
    u32 capacity;
    V2* data;
} V2Array;

typedef struct UiWindowPtrArray
{
    u32 size;
    u32 capacity;
    UiWindow** data;
} UiWindowPtrArray;

typedef struct UiContext
{
    MVP mvp;

    u32 current_window_id;
    u32 current_index_offset;
    U32Array id_to_index;
    U32Array free_indices;
    UiWindowArray windows;
    AABBArrayArray window_aabbs;
    HoverClickedIndexArray window_hover_clicked_indices;

    RenderingProperties render;

    DockNode* dock_tree;

    U32Array dock_free_indices;
    U32Array dock_id_to_index;
    DockArray docks;
    AABB dock_expanded_spaces[4];
    b8 docking_hit[4];

    FontTTF font;

    V2 dimensions;
    f64 delta_time;

    // TODO: nested ones
    f32 padding;
    f32 row_current;
    f32 column_current;
    b8 row;
    b8 column;

    // TODO: Better solution?
    b8 any_window_hold;

} UiContext;

global UiContext ui_context = { 0 };

internal f32 set_scroll_offset(const f32 total_height, const f32 area_y,
                               f32 offset)
{
    const f32 y_offset = event_get_mouse_wheel_event()->y_offset * 100.0f;
    offset += y_offset;
    offset = clampf32_high(offset, 0.0f);

    f32 low = area_y - total_height;
    low = clampf32_high(low, 0.0f);
    return clampf32_low(offset, low);
}

internal f32 smooth_scroll(const f64 delta_time, const f32 offset,
                           f32 scroll_offset)
{
    scroll_offset += (f32)((offset - scroll_offset) * (delta_time * 15.0f));
    return scroll_offset;
}

internal b8 is_mouse_button_clicked(i32 button)
{
    const MouseButtonEvent* event = event_get_mouse_button_event();
    return event->activated && event->action == FTIC_RELEASE &&
           event->button == button;
}

#define move_to_front(type, array, index)                                      \
    do                                                                         \
    {                                                                          \
        type temp = (array)->data[(index)];                                    \
        for (u32 i = (index); i >= 1; --i)                                     \
        {                                                                      \
            (array)->data[i] = (array)->data[i - 1];                           \
        }                                                                      \
        (array)->data[0] = temp;                                               \
    } while (0)

internal void change_window_order(UiContext* context,
                                  const u32 window_in_focus_index)
{
    if (window_in_focus_index == 0)
    {
        return;
    }
    move_to_front(UiWindow, &context->windows, window_in_focus_index);
    for (u32 i = 0; i < context->windows.size; ++i)
    {
        context->id_to_index.data[context->windows.data[i].id] = i;
        context->windows.data[i].index = i;
        context->windows.data[i].dock_node->window = context->windows.data + i;
    }
    move_to_front(AABBArray, &context->window_aabbs, window_in_focus_index);
    move_to_front(HoverClickedIndex, &context->window_hover_clicked_indices,
                  window_in_focus_index);
}

internal u32 get_id(const u32 size, U32Array* free_indices,
                    U32Array* id_to_index)
{
    u32 id = 0;
    if (free_indices->size)
    {
        id = *array_back(free_indices);
        free_indices->size--;
        id_to_index->data[id] = size;
    }
    else
    {
        id = id_to_index->size;
        array_push(id_to_index, size);
    }
    return id;
}

internal b8 is_key_pressed_repeat(i32 key)
{
    const KeyEvent* event = event_get_key_event();
    return event->activated &&
           (event->action == FTIC_PRESS || event->action == FTIC_REPEAT) &&
           event->key == key;
}

DockNode* dock_node_create(NodeType type, SplitAxis split_axis,
                           UiWindow* window)
{
    DockNode* node = (DockNode*)calloc(1, sizeof(DockNode));
    node->type = type;
    node->split_axis = split_axis;
    node->window = window;
    return node;
}

void dock_node_split(DockNode* node, SplitAxis split_axis, UiWindow* window1,
                     UiWindow* window2)
{
    if (node->type == NODE_LEAF)
    {
        node->type = NODE_PARENT;
        node->split_axis = split_axis;
        node->children[0] = dock_node_create(NODE_LEAF, SPLIT_NONE, window1);
        node->children[1] = dock_node_create(NODE_LEAF, SPLIT_NONE, window2);
    }
}

void dock_node_dock(DockNode* root, SplitAxis split_axis, UiWindow* window1)
{
    DockNode* split_node = dock_node_create(NODE_PARENT, split_axis, NULL);

    split_node->children[1] = root->children[0];
    split_node->children[0] = dock_node_create(NODE_LEAF, SPLIT_NONE, window1);

    root->type = NODE_PARENT;
    root->split_axis = split_axis;
    root->children[0] = split_node;
}

internal void dock_node_set_split(DockNode* split_node, DockNode* parent,
                                  SplitAxis split_axis)
{
    if (split_axis == SPLIT_HORIZONTAL)
    {
        split_node->children[0]->size.width = parent->size.width;
        split_node->children[0]->size.height = parent->size.height * 0.5f;
        split_node->children[1]->size = split_node->children[0]->size;

        split_node->children[0]->position = parent->position;
        split_node->children[1]->position.x = parent->position.x;
        split_node->children[1]->position.y =
            parent->position.y + split_node->children[0]->size.height;
    }
    else if (split_axis == SPLIT_VERTICAL)
    {
        split_node->children[0]->size.width = parent->size.width * 0.5f;
        split_node->children[0]->size.height = parent->size.height;
        split_node->children[1]->size = split_node->children[0]->size;

        split_node->children[0]->position = parent->position;
        split_node->children[1]->position.x =
            parent->position.x + split_node->children[0]->size.width;
        split_node->children[1]->position.y = parent->position.y;
    }
    DockNode* left = split_node->children[0];
    UiWindow* left_window = left->window;
    if (left_window != NULL)
    {
        left_window->position = left->position;
        left_window->size = left->size;
    }
    DockNode* right = split_node->children[1];
    UiWindow* right_window = right->window;
    if (right_window != NULL)
    {
        right_window->position = right->position;
        right_window->size = right->size;
    }
}

void dock_node_resize_traverse(DockNode* split_node)
{
    dock_node_set_split(split_node, split_node, split_node->split_axis);
    if (split_node->children[0]->type == NODE_PARENT)
    {
        dock_node_resize_traverse(split_node->children[0]);
    }
    if (split_node->children[1]->type == NODE_PARENT)
    {
        dock_node_resize_traverse(split_node->children[1]);
    }
}

void dock_node_dock_window(DockNode* root, DockNode* window,
                           SplitAxis split_axis, u8 where)
{
    DockNode* copy_node = root->children[0];
    if (root->type == NODE_LEAF)
    {
        copy_node = dock_node_create(NODE_LEAF, SPLIT_NONE, root->window);
    }
    else if (root->type == NODE_PARENT)
    {
        copy_node = root->children[!where];
    }
    else if (root->type != NODE_ROOT)
    {
        ftic_assert(false);
    }

    if (copy_node == NULL)
    {
        window->window->position = root->position;
        window->window->size = root->size;
        root->children[0] = window;
        return;
    }

    DockNode* split_node = dock_node_create(NODE_PARENT, split_axis, NULL);
    // DockNode* window_node =  dock_node_create(NODE_LEAF, SPLIT_NONE, window);

    // where == 1 Left/Top
    split_node->children[!where] = window;
    split_node->children[where] = copy_node;

    dock_node_set_split(split_node, root, split_axis);
    if (copy_node->type == NODE_PARENT)
    {
        dock_node_resize_traverse(copy_node);
    }
    if (window->type == NODE_PARENT)
    {
        printf("hh\n");
        dock_node_resize_traverse(window);
    }

    root->type = root->type == NODE_ROOT ? NODE_ROOT : NODE_PARENT;
    root->split_axis = split_axis;
    root->window = NULL;
    root->children[0] = split_node;
    root->children[1] = NULL;
}

void dock_node_resize_from_root(DockNode* root, const V2 size)
{
    root->size = size;
    DockNode* left = root->children[0];
    if (left != NULL)
    {
        left->size = size;
        UiWindow* left_window = left->window;
        if (left_window != NULL)
        {
            left_window->position = left->position;
            left_window->size = left->size;
        }
        if (left->type == NODE_PARENT)
        {
            dock_node_resize_traverse(left);
        }
    }
}

InputBuffer ui_input_buffer_create()
{
    InputBuffer input = {
        .input_index = -1,
        .time = 0.4f,
    };
    array_create(&input.buffer, 20);
    return input;
}

void ui_context_create()
{
    array_create(&ui_context.id_to_index, 100);
    array_create(&ui_context.free_indices, 100);
    array_create(&ui_context.windows, 10);
    array_create(&ui_context.window_aabbs, 10);
    array_create(&ui_context.window_hover_clicked_indices, 10);

    ui_context.dock_tree = dock_node_create(NODE_ROOT, SPLIT_NONE, NULL);

    array_create(&ui_context.docks, 10);
    array_create(&ui_context.dock_id_to_index, 10);
    array_create(&ui_context.dock_free_indices, 10);

    VertexBufferLayout vertex_buffer_layout = default_vertex_buffer_layout();

    u32 shader = shader_create("./res/shaders/vertex.glsl",
                               "./res/shaders/fragment.glsl");

    ui_context.font = (FontTTF){ 0 };
    const i32 width_atlas = 512;
    const i32 height_atlas = 512;
    const f32 pixel_height = 16;
    const u32 bitmap_size = width_atlas * height_atlas;
    u8* font_bitmap_temp = (u8*)calloc(bitmap_size, sizeof(u8));
    init_ttf_atlas(width_atlas, height_atlas, pixel_height, 96, 32,
                   "res/fonts/arial.ttf", font_bitmap_temp, &ui_context.font);
    u8* font_bitmap = (u8*)malloc(bitmap_size * 4 * sizeof(u8));
    memset(font_bitmap, UINT8_MAX, bitmap_size * 4 * sizeof(u8));
    for (u32 i = 0, j = 3; i < bitmap_size; ++i, j += 4)
    {
        font_bitmap[j] = font_bitmap_temp[i];
    }
    free(font_bitmap_temp);

    TextureProperties texture_properties = {
        .width = width_atlas,
        .height = height_atlas,
        .bytes = font_bitmap,
    };
    u32 font_texture = texture_create(&texture_properties, GL_RGBA8, GL_RGBA);
    free(texture_properties.bytes);

    u32 default_texture = create_default_texture();
    u32 file_icon_texture = load_icon("res/icons/icon_sheet.png");
    u32 arrow_icon_texture =
        load_icon_as_white("res/icons/arrow_sprite_sheet.png");

    U32Array textures = { 0 };
    array_create(&textures, 10);
    array_push(&textures, default_texture);
    array_push(&textures, font_texture);
    array_push(&textures, file_icon_texture);
    array_push(&textures, arrow_icon_texture);

    ui_context.render = rendering_properties_initialize(
        shader, textures, &vertex_buffer_layout, 100 * 4, 100 * 4);
}

void ui_context_begin(V2 dimensions, f64 delta_time, b8 check_collisions)
{
    if (!v2_equal(ui_context.dimensions, dimensions))
    {
        dock_node_resize_from_root(ui_context.dock_tree, dimensions);
    }
    ui_context.delta_time = delta_time;
    ui_context.dimensions = dimensions;

    ui_context.mvp.projection =
        ortho(0.0f, dimensions.width, dimensions.height, 0.0f, -1.0f, 1.0f);
    ui_context.mvp.view = m4d();
    ui_context.mvp.model = m4d();

    ui_context.render.vertices.size = 0;
    ui_context.current_index_offset = 0;

    ui_context.row = false;
    ui_context.row_current = 0;
    ui_context.padding = 0.0f;

    for (u32 i = 0; i < ui_context.window_hover_clicked_indices.size; ++i)
    {
        HoverClickedIndex* hover_clicked_index =
            ui_context.window_hover_clicked_indices.data + i;
        hover_clicked_index->index = -1;
        hover_clicked_index->clicked = false;
        hover_clicked_index->pressed = false;
        hover_clicked_index->hover = false;
        hover_clicked_index->double_clicked = false;
    }
    ui_context.any_window_hold = false;
    b8 any_top_bar_pressed = false;
    for (u32 i = 0; i < ui_context.windows.size; ++i)
    {
        UiWindow* window = ui_context.windows.data + i;
        window->rendering_index_count = 0;
        window->rendering_index_offset = 0;
        ui_context.any_window_hold |=
            window->top_bar_hold || window->resize_dragging;
        any_top_bar_pressed |= window->top_bar_pressed;
    }
    if (check_collisions && !ui_context.any_window_hold && !any_top_bar_pressed)
    {
        const b8 mouse_button_clicked =
            is_mouse_button_clicked(FTIC_MOUSE_BUTTON_LEFT);

        const MouseButtonEvent* mouse_button_event =
            event_get_mouse_button_event();

        const b8 mouse_button_pressed =
            mouse_button_event->activated &&
            mouse_button_event->action == FTIC_PRESS;

        const V2 mouse_position = event_get_mouse_position();
        for (u32 window_index = 0; window_index < ui_context.windows.size;
             ++window_index)
        {
            const AABBArray* aabbs =
                ui_context.window_aabbs.data + window_index;

            for (i32 aabb_index = aabbs->size; aabb_index >= 0; --aabb_index)
            {
                const AABB* aabb = aabbs->data + aabb_index;
                if (collision_point_in_aabb(mouse_position, aabb))
                {
                    HoverClickedIndex* hover_clicked_index =
                        ui_context.window_hover_clicked_indices.data +
                        window_index;
                    hover_clicked_index->index = aabb_index;
                    hover_clicked_index->hover = true;
                    hover_clicked_index->double_clicked =
                        mouse_button_event->double_clicked;
                    if (mouse_button_clicked || mouse_button_pressed)
                    {
                        hover_clicked_index->pressed = mouse_button_pressed;
                        hover_clicked_index->clicked = mouse_button_clicked;
                        change_window_order(&ui_context, window_index);
                    }
                    goto collision_check_done;
                }
            }
        }
    }
collision_check_done:;
}

internal b8 set_docking(const V2 contracted_position, const V2 contracted_size,
                        const V2 expanded_position, const V2 expanded_size,
                        const u32 index, u32* index_count)
{
    AABB aabb = {
        .min = contracted_position,
        .size = contracted_size,
    };

    b8 result = false;
    if (collision_point_in_aabb(event_get_mouse_position(), &aabb))
    {
        ui_context.dock_expanded_spaces[index] = quad(
            &ui_context.render.vertices, expanded_position, expanded_size,
            v4f(secondary_color.r, secondary_color.g, secondary_color.b, 0.4f),
            0.0f);
        *index_count += 6;
        result = true;
    }
    quad_border_rounded(&ui_context.render.vertices, index_count, aabb.min,
                        aabb.size, secondary_color, 1.0f, 0.4f, 3, 0.0f);
    return result;
}

void ui_context_end()
{
    const u32 index_offset = ui_context.current_index_offset;
    u32 index_count = 0;

#if 1
    if (ui_context.any_window_hold)
    {
        const V2 docking_size_top_bottom = v2f(50.0f, 30.0f);
        const V2 docking_size_left_right = v2f(30.0f, 50.0f);
        const V2 middle_top_bottom = v2f(
            middle(ui_context.dimensions.width, docking_size_top_bottom.width),
            middle(ui_context.dimensions.height,
                   docking_size_top_bottom.height));
        const V2 middle_left_right = v2f(
            middle(ui_context.dimensions.width, docking_size_top_bottom.width),
            middle(ui_context.dimensions.height,
                   docking_size_top_bottom.height));

        const V2 expanded_size_top_bottom = v2f(
            ui_context.dimensions.width, ui_context.dimensions.height * 0.3f);
        const V2 expanded_size_left_right = v2f(
            ui_context.dimensions.width * 0.5f, ui_context.dimensions.height);

        memset(ui_context.docking_hit, 0, sizeof(ui_context.docking_hit));

        // Top
        ui_context.docking_hit[0] = set_docking(
            v2f(middle_top_bottom.x, 10.0f), docking_size_top_bottom, v2d(),
            expanded_size_top_bottom, 0, &index_count);

        // Right
        ui_context.docking_hit[1] = set_docking(
            v2f(ui_context.dimensions.x -
                    (docking_size_left_right.width + 10.0f),
                middle_left_right.y),
            docking_size_left_right,
            v2f(ui_context.dimensions.x - expanded_size_left_right.width, 0.0f),
            expanded_size_left_right, 1, &index_count);

        // Bottom
        ui_context.docking_hit[2] =
            set_docking(v2f(middle_top_bottom.x,
                            ui_context.dimensions.y -
                                (docking_size_top_bottom.height + 10.0f)),
                        docking_size_top_bottom,
                        v2f(0.0f, ui_context.dimensions.y -
                                      expanded_size_top_bottom.height),
                        expanded_size_top_bottom, 2, &index_count);

        // Left
        ui_context.docking_hit[3] = set_docking(
            v2f(10.0f, middle_left_right.y), docking_size_left_right,
            v2f(0, 0.0f), expanded_size_left_right, 3, &index_count);

        ui_context.current_index_offset = index_offset + index_count;
    }
    else
    {
        i32 hit_index = -1;
        for (u32 i = 0; i < 4; ++i)
        {
            if (ui_context.docking_hit[i])
            {
                hit_index = i;
                break;
            }
        }
        UiWindow* focused_window = ui_context.windows.data;
        if (hit_index == 0)
        {
            dock_node_dock_window(ui_context.dock_tree,
                                  focused_window->dock_node, SPLIT_HORIZONTAL,
                                  DOCK_SIDE_TOP);
        }
        else if (hit_index == 1)
        {
            dock_node_dock_window(ui_context.dock_tree,
                                  focused_window->dock_node, SPLIT_VERTICAL,
                                  DOCK_SIDE_RIGHT);
        }
        else if (hit_index == 2)
        {
            dock_node_dock_window(ui_context.dock_tree,
                                  focused_window->dock_node, SPLIT_HORIZONTAL,
                                  DOCK_SIDE_BOTTOM);
        }
        else if (hit_index == 3)
        {
            dock_node_dock_window(ui_context.dock_tree,
                                  focused_window->dock_node, SPLIT_VERTICAL,
                                  DOCK_SIDE_LEFT);
        }
        memset(ui_context.docking_hit, 0, sizeof(ui_context.docking_hit));
    }
#endif

    if (event_get_key_event()->activated &&
        event_get_key_event()->action == FTIC_PRESS)
    {
        u32 key = event_get_key_event()->key - FTIC_KEY_0;
        if(closed_interval(1, key, 5))
        {
            change_window_order(&ui_context, key);
        }
    }

    rendering_properties_check_and_grow_buffers(
        &ui_context.render, ui_context.current_index_offset);

    buffer_set_sub_data(ui_context.render.vertex_buffer_id, GL_ARRAY_BUFFER, 0,
                        sizeof(Vertex) * ui_context.render.vertices.size,
                        ui_context.render.vertices.data);

    rendering_properties_begin_draw(&ui_context.render, &ui_context.mvp);
    for (i32 i = ui_context.windows.size - 1; i >= 0; --i)
    {
        const UiWindow* window = ui_context.windows.data + i;
        AABB scissor = { 0 };
        scissor.min.x = window->position.x;
        scissor.min.y = ui_context.dimensions.y -
                        (window->position.y + window->size.height);
        scissor.size = window->size;
        rendering_properties_draw(window->rendering_index_offset,
                                  window->rendering_index_count, &scissor);
    }
    AABB whole_screen_scissor = { .size = ui_context.dimensions };
    rendering_properties_draw(index_offset, index_count, &whole_screen_scissor);
    rendering_properties_end_draw(&ui_context.render);
}

u32 ui_window_create()
{
    u32 id = get_id(ui_context.windows.size, &ui_context.free_indices,
                    &ui_context.id_to_index);

    UiWindow window = {
        .id = id,
        .docking_id = -1,
        .index = ui_context.windows.size,
        .size = v2f(100.0f, 100.0f),
        .top_color = clear_color,
        .bottom_color = clear_color,
    };
    array_push(&ui_context.windows, window);
    UiWindow* added_window = array_back(&ui_context.windows);
    added_window->dock_node =
        dock_node_create(NODE_LEAF, SPLIT_NONE, added_window);

    AABBArray aabbs = { 0 };
    array_create(&aabbs, 10);
    array_push(&ui_context.window_aabbs, aabbs);

    HoverClickedIndex hover_clicked_index = { .index = -1 };
    array_push(&ui_context.window_hover_clicked_indices, hover_clicked_index);

    return id;
}

UiWindow* ui_window_get(u32 window_id)
{
    return ui_context.windows.data + ui_context.id_to_index.data[window_id];
}

b8 ui_window_begin(u32 window_id, b8 top_bar)
{
    const u32 window_index = ui_context.id_to_index.data[window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;

    if (window->hide) return false;

    ui_context.current_window_id = window_id;

    if (window->docking_id >= 0 && !window->top_bar_hold)
    {
        const Dock* dock = ui_context.docks.data +
                           ui_context.dock_id_to_index.data[window->docking_id];

        window->position = dock->aabb.min;
        window->size = dock->aabb.size;
    }

    window->rendering_index_offset = ui_context.current_index_offset;
    aabbs->size = 0;

    array_push(aabbs,
               quad_gradiant_t_b(&ui_context.render.vertices, window->position,
                                 window->size, window->top_color,
                                 window->bottom_color, 0.0f));
    window->rendering_index_count += 6;
    window->area_hit =
        collision_point_in_aabb(event_get_mouse_position(), array_back(aabbs));
    window->top_bar = top_bar;

    window->first_item_position = window->position;
    window->first_item_position.y += 20.0f;

    window->total_height = 0.0f;

    v2_add_equal(&window->size, window->resize_offset);

    return true;
}

internal void add_scroll_bar(UiWindow* window, AABBArray* aabbs,
                             HoverClickedIndex hover_clicked_index)
{
    const f32 scroll_bar_width = 8.0f;
    V2 position =
        v2f(window->position.x + window->size.width, window->position.y);
    position.x -= scroll_bar_width;

    const f32 area_height = window->size.height;
    const f32 total_height = window->total_height;
    if (area_height < total_height)
    {
        quad(&ui_context.render.vertices, position,
             v2f(scroll_bar_width, area_height), high_light_color, 0.0f);
        window->rendering_index_count += 6;

        const f32 initial_y = position.y;
        const f32 high = 0.0f;
        const f32 low = area_height - total_height;
        const f32 p = (window->current_scroll_offset - low) / (high - low);
        const V2 scroll_bar_dimensions =
            v2f(scroll_bar_width, area_height * (area_height / total_height));

        const f32 lower_end = window->position.y + window->size.height;
        position.y =
            lerp_f32(lower_end - scroll_bar_dimensions.height, initial_y, p);

        V2 mouse_position = event_get_mouse_position();
        b8 collided = hover_clicked_index.index == (i32)aabbs->size;

        AABB scroll_bar_aabb = {
            .min = position,
            .size = scroll_bar_dimensions,
        };
        array_push(aabbs, scroll_bar_aabb);

        if (collided || window->scroll_bar.dragging)
        {
            quad(&ui_context.render.vertices, position, scroll_bar_dimensions,
                 bright_color, 0.0f);
            window->rendering_index_count += 6;
        }
        else
        {
            quad_gradiant_t_b(&ui_context.render.vertices, position,
                              scroll_bar_dimensions, lighter_color, v4ic(0.45f),
                              0.0f);
            window->rendering_index_count += 6;
        }

        if (hover_clicked_index.pressed && collided)
        {
            window->scroll_bar.dragging = true;
            window->scroll_bar.mouse_pointer_offset =
                scroll_bar_aabb.min.y - mouse_position.y;
        }

        if (hover_clicked_index.clicked)
        {
            window->scroll_bar.dragging = false;
        }

        if (window->scroll_bar.dragging)
        {
            const f32 end_position_y = lower_end - scroll_bar_dimensions.height;

            f32 new_y =
                mouse_position.y + window->scroll_bar.mouse_pointer_offset;

            // Clipping
            if (new_y < initial_y)
            {
                window->scroll_bar.mouse_pointer_offset =
                    scroll_bar_aabb.min.y - mouse_position.y;
                new_y = initial_y;
            }
            if (new_y > end_position_y)
            {
                window->scroll_bar.mouse_pointer_offset =
                    scroll_bar_aabb.min.y - mouse_position.y;
                new_y = end_position_y;
            }

            const f32 offset_p =
                (new_y - initial_y) / (end_position_y - initial_y);

            f32 lerp = lerp_f32(high, low, offset_p);

            lerp = clampf32_high(lerp, high);
            lerp = clampf32_low(lerp, low);
            window->current_scroll_offset = lerp;
            window->end_scroll_offset = lerp;
        }
    }
}

void ui_window_end()
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;

    if (window->hide) return;

    const V2 top_bar_dimensions = v2f(window->size.width, 20.0f);
    HoverClickedIndex hover_clicked_index =
        ui_context.window_hover_clicked_indices.data[window_index];

    if (window->top_bar)
    {
        if (hover_clicked_index.index == (i32)aabbs->size)
        {
            if (hover_clicked_index.pressed && !window->top_bar_pressed)
            {
                window->top_bar_offset = event_get_mouse_position();
                window->top_bar_pressed = true;
            }
        }
        if (event_get_mouse_button_event()->action == FTIC_RELEASE)
        {
            window->top_bar_pressed = false;
            window->top_bar_hold = false;
        }

        if (window->top_bar_hold)
        {
            window->position =
                v2_add(event_get_mouse_position(), window->top_bar_offset);
        }
        else if (window->top_bar_pressed)
        {
            if (v2_distance(window->top_bar_offset,
                            event_get_mouse_position()) >= 10.0f)
            {
                window->top_bar_hold = true;
                window->top_bar_offset =
                    v2_sub(window->position, event_get_mouse_position());
            }
        }

        array_push(aabbs, quad(&ui_context.render.vertices, window->position,
                               top_bar_dimensions, high_light_color, 0.0f));
        window->rendering_index_count += 6;

        quad_border(&ui_context.render.vertices, &window->rendering_index_count,
                    window->position, top_bar_dimensions, border_color, 1.0f,
                    0.0f);
    }

    add_scroll_bar(window, aabbs, hover_clicked_index);

    if (!window->scroll_bar.dragging)
    {
        if (event_get_mouse_wheel_event()->activated && window->area_hit)
        {
            window->end_scroll_offset =
                set_scroll_offset(window->total_height, window->size.y,
                                  window->end_scroll_offset);
        }
        window->current_scroll_offset =
            smooth_scroll(ui_context.delta_time, window->end_scroll_offset,
                          window->current_scroll_offset);
    }

    const b8 in_focus = window_index == 0;
    V4 color = border_color;
    if (in_focus)
    {
        color = secondary_color;
    }
    quad_border(&ui_context.render.vertices, &window->rendering_index_count,
                window->position, window->size, color, 1.0f, 0.0f);

    if (event_get_mouse_button_event()->action == FTIC_RELEASE)
    {
        window->resize_dragging = 0;
        window->last_resize_offset = window->resize_offset;
    }

    V2 mouse_position = event_get_mouse_position();
    if (check_bit(window->resizeable, RESIZE_RIGHT))
    {
        AABB resize_aabb = { .min = window->position };
        resize_aabb.min.x += window->size.width - 2.0f;
        resize_aabb.size = v2f(4.0f, window->size.height);

        if (collision_point_in_aabb(mouse_position, &resize_aabb))
        {
            // window_set_cursor(NULL, FTIC_RESIZE_H_CURSOR);
            if (event_get_mouse_button_event()->activated)
            {
                set_bit(window->resize_dragging, RESIZE_RIGHT);
                window->resize_pointer_offset = event_get_mouse_position();
            }
        }
        else
        {
            // window_set_cursor(NULL, FTIC_NORMAL_CURSOR);
        }
    }

    V2 before_offset = v2_sub(window->size, window->resize_offset);
    if (check_bit(window->resize_dragging, RESIZE_RIGHT))
    {
        const f32 before_change = window->resize_offset.x;
        window->resize_offset.x =
            window->last_resize_offset.x +
            (event_get_mouse_position().x - window->resize_pointer_offset.x);

        if (before_offset.x + window->resize_offset.x <= 150.0f)
        {
            window->resize_offset.x = before_change;
        }
    }

    // NOTE(Linus): += also works
    ui_context.current_index_offset =
        window->rendering_index_offset + window->rendering_index_count;
}

void ui_window_row_begin(const f32 padding)
{
    ui_context.row = true;
    ui_context.row_current = 0;
    ui_context.padding = padding;
}

f32 ui_window_row_end()
{
    const f32 result = ui_context.row_current;
    ui_context.row = false;
    ui_context.row_current = 0;
    ui_context.padding = 0.0f;
    return result;
}

b8 ui_window_add_icon_button(V2 position, const V2 size,
                             const V4 texture_coordinates,
                             const f32 texture_index, const b8 disable)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;
    HoverClickedIndex hover_clicked_index =
        ui_context.window_hover_clicked_indices.data[window_index];

    v2_add_equal(&position, window->position);

    if (ui_context.row)
    {
        position.x += ui_context.row_current;
        ui_context.row_current += size.width + ui_context.padding;
    }

    V4 button_color = v4ic(1.0f);
    b8 hover = false;
    b8 clicked = false;
    if (disable)
    {
        button_color = border_color;
    }
    else
    {
        if (hover_clicked_index.index == (i32)aabbs->size)
        {
            hover = hover_clicked_index.hover;
            clicked = hover_clicked_index.clicked;
        }
    }

    AABB button_aabb = {
        .min = position,
        .size = size,
    };
    array_push(aabbs, button_aabb);

    if (hover)
    {
        quad(&ui_context.render.vertices, button_aabb.min, button_aabb.size,
             high_light_color, 0.0f);
        window->rendering_index_count += 6;
    }

    V2 icon_size = v2_s_multi(button_aabb.size, 0.7f);
    V2 icon_position =
        v2_add(position, v2f(middle(size.width, icon_size.width),
                             middle(size.height, icon_size.height)));

    quad_co(&ui_context.render.vertices, icon_position, icon_size, button_color,
            texture_coordinates, texture_index);
    window->rendering_index_count += 6;

    return clicked;
}

internal u32 render_input(const f64 delta_time, const V2 text_position,
                          InputBuffer* input)
{
    u32 index_count = 0;
    // Blinking cursor
    if (input->active)
    {
        if (input->input_index < 0)
        {
            input->input_index = input->buffer.size;
        }
        else
        {
            input->input_index =
                min((i32)input->buffer.size, input->input_index);
        }
        if (is_key_pressed_repeat(FTIC_KEY_LEFT))
        {
            input->input_index = max(input->input_index - 1, 0);
            input->time = 0.4f;
        }
        if (is_key_pressed_repeat(FTIC_KEY_RIGHT))
        {
            input->input_index =
                min(input->input_index + 1, (i32)input->buffer.size);
            input->time = 0.4f;
        }
        input->time += delta_time;
        if (input->time >= 0.4f)
        {
            const f32 x_advance =
                text_x_advance(ui_context.font.chars, input->buffer.data,
                               input->input_index, 1.0f);

            quad(
                &ui_context.render.vertices,
                v2f(text_position.x + x_advance + 1.0f, text_position.y + 2.0f),
                v2f(2.0f, 16.0f), v4i(1.0f), 0.0f);
            index_count += 6;

            input->time = input->time >= 0.8f ? 0 : input->time;
        }
    }
    if (input->buffer.size)
    {
        index_count +=
            text_generation(ui_context.font.chars, input->buffer.data, 1.0f,
                            v2f(text_position.x,
                                text_position.y + ui_context.font.pixel_height),
                            1.0f, ui_context.font.line_height, NULL, NULL, NULL,
                            &ui_context.render.vertices);
    }
    return index_count;
}

internal b8 add_from_key_buffer(const f32 width, InputBuffer* input)
{
    b8 key_typed = false;
    const CharArray* key_buffer = event_get_key_buffer();
    for (u32 i = 0; i < key_buffer->size; ++i)
    {
        char current_char = key_buffer->data[i];
        if (closed_interval(0, (current_char - 32), 96))
        {
            array_push(&input->buffer, current_char);
            f32 x_advance =
                text_x_advance(ui_context.font.chars, input->buffer.data,
                               input->buffer.size, 1.0f);

            input->buffer.data[--input->buffer.size] = '\0';
            if (x_advance >= width)
            {
                return key_typed;
            }
            array_push(&input->buffer, '\0');
            for (i32 j = input->buffer.size - 1; j > input->input_index; --j)
            {
                input->buffer.data[j] = input->buffer.data[j - 1];
            }
            input->buffer.data[input->input_index++] = current_char;
            array_push(&input->buffer, '\0');
            array_push(&input->buffer, '\0');
            array_push(&input->buffer, '\0');
            input->buffer.size -= 3;
            key_typed = true;
        }
    }
    return key_typed;
}

internal b8 erase_char(InputBuffer* input)
{
    const KeyEvent* key_event = event_get_key_event();
    if (key_event->activated &&
        (key_event->action == FTIC_PRESS || key_event->action == FTIC_REPEAT) &&
        (key_event->key == FTIC_KEY_BACKSPACE ||
         (key_event->ctrl_pressed && key_event->key == FTIC_KEY_H)))
    {
        if (input->input_index > 0)
        {
            input->input_index--;
            for (u32 i = input->input_index; i < input->buffer.size; ++i)
            {
                input->buffer.data[i] = input->buffer.data[i + 1];
            }
            *array_back(&input->buffer) = '\0';
            input->buffer.size--;
            return true;
        }
    }
    return false;
}

b8 ui_window_add_input_field(V2 position, const V2 size, InputBuffer* input)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;
    HoverClickedIndex hover_clicked_index =
        ui_context.window_hover_clicked_indices.data[window_index];

    v2_add_equal(&position, window->position);

    if (ui_context.row)
    {
        position.x += ui_context.row_current;
        ui_context.row_current += size.width + ui_context.padding;
    }

    if (is_mouse_button_clicked(FTIC_MOUSE_BUTTON_LEFT) ||
        is_mouse_button_clicked(FTIC_MOUSE_BUTTON_RIGHT))
    {
        input->active = hover_clicked_index.index == (i32)aabbs->size;
    }
    array_push(aabbs, quad(&ui_context.render.vertices, position, size,
                           high_light_color, 0.0f));
    window->rendering_index_count += 6;

    b8 typed = false;
    if (input->active)
    {
        typed = erase_char(input);
        typed |= add_from_key_buffer(size.width - 10.0f, input);
    }
    V2 text_position =
        v2f(position.x + 5.0f,
            position.y + (middle(size.y, ui_context.font.pixel_height) * 0.8f));
    window->rendering_index_count +=
        render_input(ui_context.delta_time, text_position, input);

    quad_border_rounded(&ui_context.render.vertices,
                        &window->rendering_index_count, position, size,
                        border_color, 1.0f, 0.4f, 3, 0.0f);

    return typed;
}

internal u32 display_text_and_truncate_if_necissary(const V2 position,
                                                    const f32 total_width,
                                                    char* text)
{
    const u32 text_len = (u32)strlen(text);
    const i32 i = text_check_length_within_boundary(
        ui_context.font.chars, text, text_len, 1.0f, total_width);
    const b8 too_long = i >= 3;
    char saved_name[4] = "...";
    if (too_long)
    {
        i32 j = i - 3;
        string_swap(text + j, saved_name); // Truncate
    }
    u32 index_count =
        text_generation(ui_context.font.chars, text, 1.0f, position, 1.0f,
                        ui_context.font.pixel_height, NULL, NULL, NULL,
                        &ui_context.render.vertices);
    if (too_long)
    {
        memcpy(text + (i - 3), saved_name, sizeof(saved_name));
    }
    return index_count;
}

internal b8 item_in_view(const f32 position_y, const f32 height,
                         const f32 window_height)
{
    const f32 value = position_y + height;
    return closed_interval(-height, value, window_height + height);
}

internal b8 directory_item(V2 starting_position, V2 item_dimensions,
                           const f32 icon_index, V4 texture_coordinates,
                           const DirectoryItem* item,
                           SelectedItemValues* selected_item_values)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;
    HoverClickedIndex hover_clicked_index =
        ui_context.window_hover_clicked_indices.data[window_index];

    b8 selected = false;
    u32* check_if_selected = NULL;
    if (selected_item_values)
    {
        check_if_selected = hash_table_get_char_u32(
            &selected_item_values->selected_items, item->path);
        selected = check_if_selected ? true : false;
    }
    b8 hit = hover_clicked_index.index == (i32)aabbs->size;
    if (hit && selected_item_values && !check_if_selected &&
        hover_clicked_index.clicked)
    {
        const u32 path_length = (u32)strlen(item->path);
        char* path = (char*)calloc(path_length + 3, sizeof(char));
        memcpy(path, item->path, path_length);
        hash_table_insert_char_u32(&selected_item_values->selected_items, path,
                                   1);
        array_push(&selected_item_values->paths, path);
        selected = true;
    }

    AABB aabb = { .min = starting_position, .size = item_dimensions };
    V4 color = clear_color;
    V4 left_side_color = clear_color;
    if (hit || selected)
    {
        const V4 end_color = selected ? v4ic(0.45f) : v4ic(0.3f);
#if 0
        color = v4_lerp(selected ? v4ic(0.5f) : border_color, end_color,
                        ((f32)sin(pulse_x) + 1.0f) / 2.0f);
#else
        color = end_color;
#endif

        starting_position.x += 8.0f;
        item_dimensions.width -= 6.0f;
    }
    array_push(aabbs, aabb);
    quad_gradiant_l_r(&ui_context.render.vertices, aabb.min, aabb.size, color,
                      clear_color, 0.0f);
    window->rendering_index_count += 6;

    if (v4_equal(texture_coordinates, file_icon_co))
    {
        const char* extension =
            file_get_extension(item->name, (u32)strlen(item->name));
        if (extension)
        {
            if (!strcmp(extension, ".png"))
            {
                texture_coordinates = png_icon_co;
            }
            else if (!strcmp(extension, ".pdf"))
            {
                texture_coordinates = pdf_icon_co;
            }
        }
    }
    // Icon
    aabb =
        quad_co(&ui_context.render.vertices,
                v2f(starting_position.x + 5.0f, starting_position.y + 3.0f),
                v2f(20.0f, 20.0f), v4i(1.0f), texture_coordinates, icon_index);
    window->rendering_index_count += 6;

    V2 text_position =
        v2_s_add(starting_position, ui_context.font.pixel_height + 3.0f);

    text_position.x += aabb.size.x;

    f32 x_advance = 0.0f;
    if (item->size) // NOTE(Linus): Add the size text to the right side
    {
        char buffer[100] = { 0 };
        file_format_size(item->size, buffer, 100);
        x_advance = text_x_advance(ui_context.font.chars, buffer,
                                   (u32)strlen(buffer), 1.0f);
        V2 size_text_position = text_position;
        size_text_position.x =
            starting_position.x + item_dimensions.width - x_advance - 5.0f;
        window->rendering_index_count += text_generation(
            ui_context.font.chars, buffer, 1.0f, size_text_position, 1.0f,
            ui_context.font.pixel_height, NULL, NULL, NULL,
            &ui_context.render.vertices);
    }
    window->rendering_index_count += display_text_and_truncate_if_necissary(
        text_position,
        (item_dimensions.width - aabb.size.x - 10.0f - x_advance - 10.0f),
        item->name);
    return hit && hover_clicked_index.double_clicked;
}

i32 ui_window_add_directory_item_item_list(
    V2 position, const DirectoryItemArray* items, const f32 icon_index,
    const V4 texture_coordinates, const f32 item_height,
    SelectedItemValues* selected_item_values)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    V2 relative_position = position;
    v2_add_equal(&position, window->position);

    position.y += window->current_scroll_offset;
    i32 hit_index = -1;
    for (i32 i = 0; i < (i32)items->size; ++i)
    {
        if (item_in_view(position.y, item_height, ui_context.dimensions.height))
        {
            V2 item_dimensions =
                v2f(window->size.width - relative_position.x, item_height);
            if (directory_item(position, item_dimensions, icon_index,
                               texture_coordinates, &items->data[i],
                               selected_item_values))
            {
                hit_index = i;
            }
        }
        position.y += item_height;
        relative_position.y += item_height;
    }
    // TODO: add this for all components.
    window->total_height =
        max(window->total_height, relative_position.y + item_height);
    return hit_index;
}

void ui_window_add_selected(V2 position, char* text, const b8 selected)
{
}

void ui_window_add_directory_list(V2 position)
{
}

b8 ui_window_add_folder_list(V2 position, const f32 item_height,
                             const DirectoryItemArray* items,
                             SelectedItemValues* selected_item_values,
                             i32* item_selected)
{
    i32 selected_index = ui_window_add_directory_item_item_list(
        position, items, 2.0f, folder_icon_co, item_height,
        selected_item_values);
    if (item_selected) *item_selected = selected_index;
    return selected_index != -1;
}

b8 ui_window_add_file_list(V2 position, const f32 item_height,
                           const DirectoryItemArray* items,
                           SelectedItemValues* selected_item_values,
                           i32* item_selected)
{
    i32 selected_index = ui_window_add_directory_item_item_list(
        position, items, 2.0f, file_icon_co, item_height, selected_item_values);
    if (item_selected) *item_selected = selected_index;
    return selected_index != -1;
}
