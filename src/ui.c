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

    FTicWindow* window;

    u32 window_in_focus;
    u32 current_window_id;
    u32 current_index_offset;
    u32 window_count_last;
    u32 window_count;
    U32Array id_to_index;
    U32Array free_indices;
    UiWindowArray windows;
    AABBArrayArray window_aabbs;
    HoverClickedIndexArray window_hover_clicked_indices;

    RenderingProperties render;

    u32 extra_index_offset;
    u32 extra_index_count;
    i32 dock_side_hit;

    DockNode* dock_tree;
    DockNode* dock_hit_node;

    AABB dock_resize_aabb;
    AABB dock_space;

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

    b8 dock_resize;
    b8 dock_resize_hover;

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

internal void push_window_to_front(UiContext* context,
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

#define move_to_back(type, array, index)                                       \
    do                                                                         \
    {                                                                          \
        type temp = (array)->data[(index)];                                    \
        for (u32 i = (index); i < ui_context.window_count_last - 1; ++i)       \
        {                                                                      \
            (array)->data[i] = (array)->data[i + 1];                           \
        }                                                                      \
        (array)->data[ui_context.window_count_last - 1] = temp;                \
    } while (0)

internal void push_window_to_back(UiContext* context,
                                  const u32 window_in_focus_index)
{
    if (window_in_focus_index == context->windows.size - 1)
    {
        return;
    }
    move_to_back(UiWindow, &context->windows, window_in_focus_index);
    for (u32 i = 0; i < context->windows.size; ++i)
    {
        context->id_to_index.data[context->windows.data[i].id] = i;
        context->windows.data[i].index = i;
        context->windows.data[i].dock_node->window = context->windows.data + i;
    }
    move_to_back(AABBArray, &context->window_aabbs, window_in_focus_index);
    move_to_back(HoverClickedIndex, &context->window_hover_clicked_indices,
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

internal b8 set_docking(const V2 contracted_position, const V2 contracted_size,
                        const V2 expanded_position, const V2 expanded_size,
                        u32* index_count)
{
    AABB aabb = {
        .min = contracted_position,
        .size = contracted_size,
    };

    b8 result = false;
    if (collision_point_in_aabb(event_get_mouse_position(), &aabb))
    {
        quad(&ui_context.render.vertices, expanded_position, expanded_size,
             v4f(secondary_color.r, secondary_color.g, secondary_color.b, 0.4f),
             0.0f);
        *index_count += 6;
        result = true;
    }
    quad_border_rounded(&ui_context.render.vertices, index_count, aabb.min,
                        aabb.size, secondary_color, 1.0f, 0.4f, 3, 0.0f);
    return result;
}

DockNode* dock_node_create(NodeType type, SplitAxis split_axis,
                           UiWindow* window)
{
    DockNode* node = (DockNode*)calloc(1, sizeof(DockNode));
    node->type = type;
    node->split_axis = split_axis;
    node->window = window;
    node->size_ratio = 0.5f;
    return node;
}

internal void dock_node_set_split(DockNode* split_node, DockNode* parent,
                                  SplitAxis split_axis)
{
    if (split_axis == SPLIT_HORIZONTAL)
    {
        f32 split_size = parent->aabb.size.height * split_node->size_ratio;

        split_node->children[0]->aabb.size.width = parent->aabb.size.width;
        split_node->children[0]->aabb.size.height = split_size;
        split_node->children[1]->aabb.size.width = parent->aabb.size.width;
        split_node->children[1]->aabb.size.height =
            parent->aabb.size.height - split_size;

        split_node->children[0]->aabb.min = parent->aabb.min;
        split_node->children[1]->aabb.min.x = parent->aabb.min.x;
        split_node->children[1]->aabb.min.y = parent->aabb.min.y + split_size;
    }
    else if (split_axis == SPLIT_VERTICAL)
    {
        f32 split_size = parent->aabb.size.width * split_node->size_ratio;

        split_node->children[0]->aabb.size.width = split_size;
        split_node->children[0]->aabb.size.height = parent->aabb.size.height;
        split_node->children[1]->aabb.size.width =
            parent->aabb.size.width - split_size;
        split_node->children[1]->aabb.size.height = parent->aabb.size.height;

        split_node->children[0]->aabb.min = parent->aabb.min;
        split_node->children[1]->aabb.min.x = parent->aabb.min.x + split_size;
        split_node->children[1]->aabb.min.y = parent->aabb.min.y;
    }
    DockNode* left = split_node->children[0];
    UiWindow* left_window = left->window;
    if (left_window != NULL)
    {
        left_window->position = left->aabb.min;
        left_window->size = left->aabb.size;
    }
    DockNode* right = split_node->children[1];
    UiWindow* right_window = right->window;
    if (right_window != NULL)
    {
        right_window->position = right->aabb.min;
        right_window->size = right->aabb.size;
    }
}

void dock_node_resize_traverse(DockNode* split_node)
{
    dock_node_set_split(split_node, split_node, split_node->split_axis);
    DockNode* left = split_node->children[0];
    DockNode* right = split_node->children[1];
    if (left->type == NODE_PARENT)
    {
        dock_node_resize_traverse(left);
    }
    if (right->type == NODE_PARENT)
    {
        dock_node_resize_traverse(right);
    }
}

void dock_node_dock_window(DockNode* root, DockNode* window,
                           SplitAxis split_axis, u8 where)
{
    DockNode* copy_node = root->children[0];
    DockNode* split_node = root;
    if (root->type == NODE_ROOT)
    {
        split_node = dock_node_create(NODE_PARENT, split_axis, NULL);
    }
    else if (root->type == NODE_LEAF)
    {
        copy_node = dock_node_create(NODE_LEAF, SPLIT_NONE, root->window);
        root->window->dock_node = copy_node;
    }
    else
    {
        ftic_assert(false);
    }

    if (copy_node == NULL)
    {
        window->window->position = root->aabb.min;
        window->window->size = root->aabb.size;
        root->children[0] = window;
        return;
    }

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
        dock_node_resize_traverse(window);
    }

    if (root->type == NODE_ROOT)
    {
        split_node->aabb = root->aabb;
        root->children[0] = split_node;
        root->children[1] = NULL;
    }
    else
    {
        root->type = NODE_PARENT;
    }
    root->split_axis = split_axis;
    root->window = NULL;
}

void dock_node_resize_from_root(DockNode* root, const AABB* aabb)
{
    root->aabb = *aabb;
    DockNode* left = root->children[0];
    if (left != NULL)
    {
        left->aabb = root->aabb;
        UiWindow* left_window = left->window;
        if (left_window != NULL)
        {
            left_window->position = left->aabb.min;
            left_window->size = left->aabb.size;
        }
        if (left->type == NODE_PARENT)
        {
            dock_node_resize_traverse(left);
        }
    }
}

DockNode* find_node(DockNode* root, DockNode* node_before, DockNode* node)
{
    if (root->type == NODE_LEAF && root->window->id == node->window->id)
    {
        return node_before;
    }
    if (root->type != NODE_LEAF)
    {
        for (int i = 0; i < 2; i++)
        {
            if (root->children[i])
            {
                DockNode* found = find_node(root->children[i], root, node);
                if (found)
                {
                    return found;
                }
            }
        }
    }
    return NULL;
}

void dock_node_remove_node(DockNode* root, DockNode* node_to_remove)
{
    if (root->children[0]->type == NODE_LEAF &&
        root->children[0]->window->id == node_to_remove->window->id)
    {
        root->children[0] = NULL;
        return;
    }
    DockNode* node_before = find_node(root->children[0], root, node_to_remove);
    DockNode* left = node_before->children[0];
    DockNode* right = node_before->children[1];
    if (left->type == NODE_LEAF &&
        left->window->id == node_to_remove->window->id)
    {
        node_before->type = right->type;
        node_before->split_axis = right->split_axis;
        node_before->window = right->window;
        node_before->children[0] = right->children[0];
        node_before->children[1] = right->children[1];
        node_before->size_ratio = right->size_ratio;
        if (right->type == NODE_LEAF)
        {
            node_before->window->dock_node = node_before;
        }
        free(right);
    }
    else if (right->type == NODE_LEAF &&
             right->window->id == node_to_remove->window->id)
    {
        node_before->type = left->type;
        node_before->split_axis = left->split_axis;
        node_before->window = left->window;
        node_before->children[0] = left->children[0];
        node_before->children[1] = left->children[1];
        node_before->size_ratio = left->size_ratio;
        if (left->type == NODE_LEAF)
        {
            node_before->window->dock_node = node_before;
        }
        free(left);
    }
    dock_node_resize_from_root(root, &root->aabb);
}

internal i32 display_docking(const AABB* dock_aabb, const V2 top_position,
                             const V2 right_position, const V2 bottom_position,
                             const V2 left_position)
{
    const V2 docking_size_top_bottom = v2f(50.0f, 30.0f);
    const V2 docking_size_left_right = v2f(30.0f, 50.0f);

    const V2 expanded_size_top_bottom =
        v2f(dock_aabb->size.width, dock_aabb->size.height * 0.5f);
    const V2 expanded_size_left_right =
        v2f(dock_aabb->size.width * 0.5f, dock_aabb->size.height);

    i32 index = -1;
    // Top
    if (set_docking(top_position, docking_size_top_bottom, dock_aabb->min,
                    expanded_size_top_bottom, &ui_context.extra_index_count))
    {
        index = 0;
    }

    // Right
    if (set_docking(right_position, docking_size_left_right,
                    v2f(dock_aabb->min.x + expanded_size_left_right.width,
                        dock_aabb->min.y),
                    expanded_size_left_right, &ui_context.extra_index_count))
    {
        index = 1;
    }

    // Bottom

    if (set_docking(bottom_position, docking_size_top_bottom,
                    v2f(dock_aabb->min.x,
                        dock_aabb->min.y + expanded_size_top_bottom.height),
                    expanded_size_top_bottom, &ui_context.extra_index_count))
    {
        index = 2;
    }

    // Left
    if (set_docking(left_position, docking_size_left_right, dock_aabb->min,
                    expanded_size_left_right, &ui_context.extra_index_count))
    {
        index = 3;
    }
    return index;
}

i32 root_display_docking(DockNode* dock_node)
{
    const V2 docking_size_top_bottom = v2f(50.0f, 30.0f);
    const V2 docking_size_left_right = v2f(30.0f, 50.0f);

    const V2 middle_top_bottom =
        v2f(dock_node->aabb.min.x + middle(dock_node->aabb.size.width,
                                           docking_size_top_bottom.width),
            dock_node->aabb.min.y + middle(dock_node->aabb.size.height,
                                           docking_size_top_bottom.height));

    const V2 middle_left_right =
        v2f(dock_node->aabb.min.x + middle(dock_node->aabb.size.width,
                                           docking_size_left_right.width),
            dock_node->aabb.min.y + middle(dock_node->aabb.size.height,
                                           docking_size_left_right.height));

    const V2 top_position =
        v2f(middle_top_bottom.x, dock_node->aabb.min.y + 10.0f);

    const V2 right_position = v2f(dock_node->aabb.size.width -
                                      (docking_size_left_right.width + 10.0f),
                                  middle_left_right.y);

    const V2 bottom_position =
        v2f(middle_top_bottom.x, dock_node->aabb.size.height -
                                     (docking_size_top_bottom.height + 10.0f));

    const V2 left_position =
        v2f(dock_node->aabb.min.x + 10.0f, middle_left_right.y);

    i32 index = display_docking(&dock_node->aabb, top_position, right_position,
                                bottom_position, left_position);
    if (index != -1) ui_context.dock_hit_node = dock_node;
    return index;
}

i32 leaf_display_docking(DockNode* dock_node)
{
    const V2 docking_size_top_bottom = v2f(50.0f, 30.0f);
    const V2 docking_size_left_right = v2f(30.0f, 50.0f);

    const V2 middle_top_bottom =
        v2f(dock_node->aabb.min.x + middle(dock_node->aabb.size.width,
                                           docking_size_top_bottom.width),
            dock_node->aabb.min.y + middle(dock_node->aabb.size.height,
                                           docking_size_top_bottom.height));

    const V2 middle_left_right =
        v2f(dock_node->aabb.min.x + middle(dock_node->aabb.size.width,
                                           docking_size_left_right.width),
            dock_node->aabb.min.y + middle(dock_node->aabb.size.height,
                                           docking_size_left_right.height));

    const V2 top_position =
        v2f(middle_top_bottom.x,
            middle_top_bottom.y - (docking_size_top_bottom.height + 20.0f));

    const V2 right_position =
        v2f(middle_left_right.x + (docking_size_left_right.width + 20.0f),
            middle_left_right.y);

    const V2 bottom_position =
        v2f(middle_top_bottom.x,
            middle_top_bottom.y + (docking_size_top_bottom.height + 20.0f));

    const V2 left_position =
        v2f(middle_left_right.x - (docking_size_left_right.width + 20.0f),
            middle_left_right.y);

    i32 index = display_docking(&dock_node->aabb, top_position, right_position,
                                bottom_position, left_position);
    if (index != -1) ui_context.dock_hit_node = dock_node;
    return index;
}

i32 look_for_collisions(DockNode* parent)
{
    const V2 mouse_position = event_get_mouse_position();

    DockNode* left = parent->children[0];
    DockNode* right = parent->children[1];

    if (left->type == NODE_LEAF)
    {
        if (collision_point_in_aabb(mouse_position, &left->aabb))
        {
            return leaf_display_docking(left);
        }
    }
    if (right->type == NODE_LEAF)
    {
        if (collision_point_in_aabb(mouse_position, &right->aabb))
        {
            return leaf_display_docking(right);
        }
    }

    if (left->type == NODE_PARENT)
    {
        i32 hit = look_for_collisions(left);
        if (hit != -1) return hit;
    }
    if (right->type == NODE_PARENT)
    {
        i32 hit = look_for_collisions(right);
        if (hit != -1) return hit;
    }
    return -1;
}

i32 dock_node_docking_display_traverse(DockNode* root)
{
    if (root->children[0] && root->children[0]->type == NODE_PARENT)
    {
        return look_for_collisions(root->children[0]);
    }
    return -1;
}

AABB dock_node_set_resize_aabb(DockNode* node)
{
    DockNode* right = node->children[1];
    AABB resize_aabb = { .min = right->aabb.min };
    if (node->split_axis == SPLIT_HORIZONTAL)
    {
        resize_aabb.min.y -= 3.0f;
        resize_aabb.size.width = right->aabb.size.width;
        resize_aabb.size.height = 5.0f;
    }
    else // SPLIT_VERTICAL
    {
        resize_aabb.min.x -= 3.0f;
        resize_aabb.size.width = 5.0f;
        resize_aabb.size.height = right->aabb.size.height;
    }
    return resize_aabb;
}

b8 look_for_resize_collisions(DockNode* parent)
{
    const V2 mouse_position = event_get_mouse_position();

    AABB resize_aabb = dock_node_set_resize_aabb(parent);
    if (collision_point_in_aabb(mouse_position, &resize_aabb))
    {
        ui_context.dock_resize_aabb = resize_aabb;
        ui_context.dock_hit_node = parent;
        return true;
    }

    DockNode* left = parent->children[0];
    if (left->type == NODE_PARENT)
    {
        b8 hit = look_for_resize_collisions(left);
        if (hit) return hit;
    }
    DockNode* right = parent->children[1];
    if (right->type == NODE_PARENT)
    {
        b8 hit = look_for_resize_collisions(right);
        if (hit) return hit;
    }
    return false;
}

b8 dock_node_resize_collision_traverse(DockNode* root)
{
    if (root->children[0] && root->children[0]->type == NODE_PARENT)
    {
        return look_for_resize_collisions(root->children[0]);
    }
    return false;
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

void ui_context_create(FTicWindow* window)
{
    ui_context.window = window;

    array_create(&ui_context.id_to_index, 100);
    array_create(&ui_context.free_indices, 100);
    array_create(&ui_context.windows, 10);
    array_create(&ui_context.window_aabbs, 10);
    array_create(&ui_context.window_hover_clicked_indices, 10);

    ui_context.dock_tree = dock_node_create(NODE_ROOT, SPLIT_NONE, NULL);
    ui_context.dock_side_hit = -1;

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

void ui_context_begin(const V2 dimensions, const AABB* dock_space,
                      const f64 delta_time, const b8 check_collisions)
{
    if (!aabb_equal(&ui_context.dock_space, dock_space))
    {
        dock_node_resize_from_root(ui_context.dock_tree, dock_space);
    }
    ui_context.dock_space = *dock_space;
    ui_context.dimensions = dimensions;

    ui_context.delta_time = delta_time;
    ui_context.window_count_last = ui_context.window_count;
    ui_context.window_count = 0;

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
    i32 index_window_dragging = -1;
    for (u32 i = 0; i < ui_context.windows.size; ++i)
    {
        UiWindow* window = ui_context.windows.data + i;
        window->rendering_index_count = 0;
        window->rendering_index_offset = 0;
        ui_context.any_window_hold |=
            window->top_bar_hold || window->resize_dragging;
        any_top_bar_pressed |= window->top_bar_pressed;
        if (window->top_bar_hold)
        {
            index_window_dragging = i;
        }
    }
    if (index_window_dragging != -1)
    {
        push_window_to_front(&ui_context, index_window_dragging);
        ui_context.window_in_focus = 0;
    }
    if (check_collisions && !ui_context.dock_resize_hover &&
        !ui_context.dock_resize && !ui_context.any_window_hold &&
        !any_top_bar_pressed)
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

            if(aabbs->size)
            {
                // NOTE: the first aabb is the window area
                if (!collision_point_in_aabb(mouse_position, &aabbs->data[0]))
                {
                    continue;
                }
            }

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
                        if (!ui_context.windows.data[window_index].docked)
                        {
                            push_window_to_front(&ui_context, window_index);
                            ui_context.window_in_focus = 0;
                        }
                        else
                        {
                            ui_context.window_in_focus = window_index;
                        }
                    }
                    goto collision_check_done;
                }
            }
        }
    }
collision_check_done:;
}

void ui_context_end()
{
    ui_context.extra_index_offset = ui_context.current_index_offset;
    ui_context.extra_index_count = 0;

#if 1
    if (ui_context.any_window_hold)
    {
        i32 hit = root_display_docking(ui_context.dock_tree);
        ui_context.dock_side_hit = hit;
        hit = dock_node_docking_display_traverse(ui_context.dock_tree);
        if (hit != -1) ui_context.dock_side_hit = hit;

        ui_context.current_index_offset =
            ui_context.extra_index_offset + ui_context.extra_index_count;
    }
    else
    {
        b8 any_hit = false;
        UiWindow* focused_window = ui_context.windows.data;
        if (ui_context.dock_side_hit == 0)
        {
            dock_node_dock_window(ui_context.dock_hit_node,
                                  focused_window->dock_node, SPLIT_HORIZONTAL,
                                  DOCK_SIDE_TOP);
            any_hit = true;
        }
        else if (ui_context.dock_side_hit == 1)
        {
            dock_node_dock_window(ui_context.dock_hit_node,
                                  focused_window->dock_node, SPLIT_VERTICAL,
                                  DOCK_SIDE_RIGHT);
            any_hit = true;
        }
        else if (ui_context.dock_side_hit == 2)
        {
            dock_node_dock_window(ui_context.dock_hit_node,
                                  focused_window->dock_node, SPLIT_HORIZONTAL,
                                  DOCK_SIDE_BOTTOM);
            any_hit = true;
        }
        else if (ui_context.dock_side_hit == 3)
        {
            dock_node_dock_window(ui_context.dock_hit_node,
                                  focused_window->dock_node, SPLIT_VERTICAL,
                                  DOCK_SIDE_LEFT);
            any_hit = true;
        }
        if (any_hit)
        {
            focused_window->docked = true;
            push_window_to_back(&ui_context, 0);
        }
        ui_context.dock_side_hit = -1;
        // ui_context.dock_hit_node = NULL;

        const MouseButtonEvent* event = event_get_mouse_button_event();
        if (event->action == FTIC_RELEASE)
        {
            ui_context.dock_resize = false;
            window_set_cursor(ui_context.window, FTIC_NORMAL_CURSOR);
        }
        ui_context.dock_resize_hover = false;

        if (!ui_context.dock_resize)
        {
            if (dock_node_resize_collision_traverse(ui_context.dock_tree))
            {
                ui_context.dock_resize_hover = true;
                if (event->activated && event->action == FTIC_PRESS &&
                    event->button == FTIC_MOUSE_BUTTON_LEFT)
                {

                    ui_context.dock_resize = true;
                }

                if (ui_context.dock_hit_node->split_axis == SPLIT_HORIZONTAL)
                {
                    window_set_cursor(ui_context.window, FTIC_RESIZE_V_CURSOR);
                }
                else
                {
                    window_set_cursor(ui_context.window, FTIC_RESIZE_H_CURSOR);
                }
                quad(&ui_context.render.vertices,
                     ui_context.dock_resize_aabb.min,
                     ui_context.dock_resize_aabb.size,
                     v4_s_multi(secondary_color, 0.8f), 0.0f);
                ui_context.extra_index_count += 6;
            }
            else
            {
                window_set_cursor(ui_context.window, FTIC_NORMAL_CURSOR);
            }
        }
        else
        {

            const V2 relative_mouse_position = v2_sub(
                event_get_mouse_position(), ui_context.dock_hit_node->aabb.min);

            f32 new_ratio = 0.0f;
            if (ui_context.dock_hit_node->split_axis == SPLIT_HORIZONTAL)
            {
                new_ratio = relative_mouse_position.y /
                            ui_context.dock_hit_node->aabb.size.height;
            }
            else // SPLIT_VERTICAL
            {
                new_ratio = relative_mouse_position.x /
                            ui_context.dock_hit_node->aabb.size.width;
            }
            if (closed_interval(0.2f, new_ratio, 0.8f))
            {
                ui_context.dock_hit_node->size_ratio = new_ratio;
                dock_node_resize_traverse(ui_context.dock_hit_node);
            }
            ui_context.dock_resize_aabb =
                dock_node_set_resize_aabb(ui_context.dock_hit_node);
            quad(&ui_context.render.vertices, ui_context.dock_resize_aabb.min,
                 ui_context.dock_resize_aabb.size, secondary_color, 0.0f);
            ui_context.extra_index_count += 6;
        }
    }

#endif

    rendering_properties_check_and_grow_buffers(
        &ui_context.render, ui_context.current_index_offset);

    buffer_set_sub_data(ui_context.render.vertex_buffer_id, GL_ARRAY_BUFFER, 0,
                        sizeof(Vertex) * ui_context.render.vertices.size,
                        ui_context.render.vertices.data);

#if 1
    // TODO: make sure all docked windows is always at the back
    u32 first_docked_window = ui_context.window_count;
    for (u32 i = 0; i < ui_context.window_count; ++i)
    {
        UiWindow* window = ui_context.windows.data + i;
        if (window->docked)
        {
            if(first_docked_window == ui_context.window_count)
            {
                first_docked_window = i;
            }
        }
        else if (i > first_docked_window)
        {
            log_message("h",1);
            for (u32 j = first_docked_window; j < ui_context.window_count; ++j)
            {
                window = ui_context.windows.data + j;
                if (window->docked)
                {
                    push_window_to_back(&ui_context, j);
                }
            }
            break;
        }
    }
#endif

    rendering_properties_begin_draw(&ui_context.render, &ui_context.mvp);
    for (i32 i = ui_context.window_count - 1; i >= 0; --i)
    {
        const UiWindow* window = ui_context.windows.data + i;
        AABB scissor = { 0 };
        scissor.min.x = window->position.x;
        scissor.min.y = ui_context.dimensions.y -
                        (window->position.y + window->size.height);
        scissor.size = window->size;
        scissor.size.width += 1.0f;
        rendering_properties_draw(window->rendering_index_offset,
                                  window->rendering_index_count, &scissor);
    }
    AABB whole_screen_scissor = { .size = ui_context.dimensions };
    rendering_properties_draw(ui_context.extra_index_offset,
                              ui_context.extra_index_count,
                              &whole_screen_scissor);
    rendering_properties_end_draw(&ui_context.render);
}

u32 ui_window_create()
{
    u32 id = get_id(ui_context.windows.size, &ui_context.free_indices,
                    &ui_context.id_to_index);

    UiWindow window = {
        .id = id,
        .index = ui_context.windows.size,
        .size = v2f(200.0f, 200.0f),
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

u32 ui_window_in_focus()
{
    return ui_context.windows.data[ui_context.window_in_focus].id;
}

b8 ui_window_begin(u32 window_id, b8 top_bar)
{
    const u32 window_index = ui_context.id_to_index.data[window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;

    if (window->hide) return false;

    ui_context.current_window_id = window_id;

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
    window->first_item_position.y += 20.0f * top_bar;

    window->total_height = 0.0f;

    v2_add_equal(&window->size, window->resize_offset);

    return true;
}

internal void add_scroll_bar(UiWindow* window, AABBArray* aabbs,
                             HoverClickedIndex hover_clicked_index)
{
    const f32 scroll_bar_width = 8.0f;
    V2 position = v2f(window->first_item_position.x + window->size.width,
                      window->first_item_position.y);
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

void ui_window_end(const char* title)
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
            window->dock_node->aabb.min = window->position;
        }
        else if (window->top_bar_pressed)
        {
            if (v2_distance(window->top_bar_offset,
                            event_get_mouse_position()) >= 10.0f)
            {
                if (window->docked)
                {
                    window->docked = false;
                    dock_node_remove_node(ui_context.dock_tree,
                                          window->dock_node);

                    window->size = v2f(200.0f, 200.0f);
                    window->dock_node->aabb.size = window->size;
                }
                window->top_bar_hold = true;
                window->top_bar_offset =
                    v2_sub(window->position, event_get_mouse_position());
            }
        }

        array_push(aabbs,
                   quad_gradiant_t_b(&ui_context.render.vertices,
                                     window->position, top_bar_dimensions,
                                     v4ic(0.25f), v4ic(0.2f), 0.0f));
        window->rendering_index_count += 6;

        quad_border(&ui_context.render.vertices, &window->rendering_index_count,
                    window->position, top_bar_dimensions, border_color, 1.0f,
                    0.0f);

        if (title)
        {
            const V2 text_position =
                v2f(window->position.x + 10.0f,
                    window->position.y + ui_context.font.pixel_height);
            window->rendering_index_count += text_generation(
                ui_context.font.chars, title, 1.0f, text_position, 1.0f,
                ui_context.font.line_height, NULL, NULL, NULL,
                &ui_context.render.vertices);
        }
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

    const b8 in_focus = window_index == ui_context.window_in_focus;
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
    ui_context.window_count++;
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

    v2_add_equal(&position, window->first_item_position);

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

    v2_add_equal(&position, window->first_item_position);

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
    v2_add_equal(&position, window->first_item_position);

    position.y += window->current_scroll_offset;
    i32 hit_index = -1;
    for (i32 i = 0; i < (i32)items->size; ++i)
    {
        if (item_in_view(position.y, item_height, position.y + window->size.height))
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

b8 ui_window_add_drop_down_menu(V2 position, DropDownMenu* drop_down_menu,
                                void* option_data)
{
    const u32 drop_down_item_count = drop_down_menu->options.size;
    if (drop_down_item_count == 0) return true;

    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;
    HoverClickedIndex hover_clicked_index =
        ui_context.window_hover_clicked_indices.data[window_index];

    v2_add_equal(&position, window->first_item_position);

    drop_down_menu->x += (f32)(ui_context.delta_time * 2.0f);
    const f32 drop_down_item_height = 40.0f;
    const f32 end_height = drop_down_item_count * drop_down_item_height;
    const f32 current_y = ease_out_elastic(drop_down_menu->x) * end_height;
    const f32 precent = current_y / end_height;
    const f32 drop_down_width = 200.0f;
    const f32 drop_down_border_width = 1.0f;
    const f32 border_extra_padding = drop_down_border_width * 2.0f;
    const f32 drop_down_outer_padding = 10.0f + border_extra_padding;

    quad_border_gradiant(
        &ui_context.render.vertices, &window->rendering_index_count,
        v2f(position.x - drop_down_border_width,
            position.y - drop_down_border_width),
        v2f(drop_down_width + border_extra_padding,
            current_y + border_extra_padding),
        lighter_color, lighter_color, drop_down_border_width, 0.0f);

    b8 in_focus = window_index == ui_context.window_in_focus;
    if (in_focus && is_key_pressed_repeat(FTIC_KEY_TAB))
    {
        if (event_get_key_event()->shift_pressed)
        {
            if (--drop_down_menu->tab_index < 0)
            {
                drop_down_menu->tab_index = drop_down_item_count - 1;
            }
        }
        else
        {
            drop_down_menu->tab_index++;
            drop_down_menu->tab_index %= drop_down_item_count;
        }
    }

    b8 mouse_button_clicked = is_mouse_button_clicked(FTIC_MOUSE_BUTTON_1);
    b8 enter_clicked = is_key_clicked(FTIC_KEY_ENTER);
    b8 item_clicked = false;
    b8 any_drop_down_item_hit = false;
    b8 should_close = false;
    const f32 promt_item_text_padding = 5.0f;
    V2 promt_item_position = position;
    for (u32 i = 0; i < drop_down_item_count; ++i)
    {
        b8 drop_down_item_hit = drop_down_menu->tab_index == (i32)i;
        if (drop_down_item_hit) item_clicked = enter_clicked;

        b8 collision = hover_clicked_index.index == (i32)aabbs->size;
        if (collision && (drop_down_menu->tab_index == -1 ||
                          event_get_mouse_move_event()->activated))
        {
            drop_down_item_hit = true;
            drop_down_menu->tab_index = -1;
            any_drop_down_item_hit = true;
            item_clicked = mouse_button_clicked;
        }

        const V4 drop_down_color =
            drop_down_item_hit ? border_color : high_light_color;
        V2 size = v2f(drop_down_width, drop_down_item_height * precent);
        array_push(aabbs, quad(&ui_context.render.vertices, promt_item_position,
                               size, drop_down_color, 0.0f));
        window->rendering_index_count += 6;

        V2 promt_item_text_position = promt_item_position;
        promt_item_text_position.y +=
            ui_context.font.pixel_height + promt_item_text_padding + 3.0f;
        promt_item_text_position.x += promt_item_text_padding;

        V4 text_color = v4i(1.0f);
        should_close = drop_down_menu->menu_options_selection(
            i, drop_down_item_hit, should_close, item_clicked, &text_color,
            option_data);

        window->rendering_index_count += text_generation_color(
            ui_context.font.chars, drop_down_menu->options.data[i], 1.0f,
            promt_item_text_position, 1.0f,
            ui_context.font.pixel_height * precent, text_color, NULL, NULL,
            NULL, &ui_context.render.vertices);

        promt_item_position.y += size.y;
    }
    return should_close ||
           ((!any_drop_down_item_hit) && mouse_button_clicked) ||
           is_key_clicked(FTIC_KEY_ESCAPE);
}
