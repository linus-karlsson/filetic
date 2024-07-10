#include "ui.h"
#include "event.h"
#include "application.h"
#include "opengl_util.h"
#include "texture.h"
#include "logging.h"
#include "set.h"
#include "hash.h"
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

typedef struct DockNodePtrArray
{

    u32 size;
    u32 capacity;
    DockNode** data;
} DockNodePtrArray;

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

    u32 window_in_focus;
    u32 current_window_id;
    u32 current_index_offset;
    U32Array id_to_index;
    U32Array free_indices;

    U32Array last_frame_windows;
    U32Array current_frame_windows;

    UiWindowArray windows;
    AABBArrayArray window_aabbs;
    HoverClickedIndexArray window_hover_clicked_indices;

    RenderingProperties render;
    u32 default_textures_offset;

    u32 extra_index_offset;
    u32 extra_index_count;
    i32 dock_side_hit;

    DockNode* dock_tree;
    DockNode* dock_hit_node;

    AABB dock_resize_aabb;
    AABB dock_space;

    V2 mouse_drag_box_point;
    AABB mouse_drag_box;
    b8 mouse_box_is_dragging;

    FontTTF font;

    V4 docking_color;

    V2 dimensions;
    f64 delta_time;

    // TODO: nested ones
    f32 padding;
    f32 row_current;
    f32 column_current;
    b8 row;
    b8 column;

    // TODO: Better solution?
    b8 any_window_top_bar_hold;
    b8 any_window_hold;

    b8 dock_resize;
    b8 dock_resize_hover;
    b8 check_collisions;

} UiContext;

typedef struct UU32
{
    u32 first;
    u32 second;
} UU32;

typedef struct UU32Array
{
    u32 size;
    u32 capacity;
    UU32* data;
} UU32Array;

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

internal void push_window_to_front(UiContext* context, U32Array* window_array,
                                   const u32 index)
{
    if ((!window_array->size) || (index == 0))
    {
        return;
    }
    u32 temp = window_array->data[(index)];
    for (u32 i = (index); i >= 1; --i)
    {
        window_array->data[i] = window_array->data[i - 1];
    }
    window_array->data[0] = temp;
    /*
    move_to_front(UiWindow, &context->windows, window_in_focus_index);
    for (u32 i = 0; i < context->windows.size; ++i)
    {
        context->id_to_index.data[context->windows.data[i].id] = i;
        context->windows.data[i].dock_node->window = context->windows.data + i;
    }
    move_to_front(AABBArray, &context->window_aabbs, window_in_focus_index);
    move_to_front(HoverClickedIndex, &context->window_hover_clicked_indices,
                  window_in_focus_index);
                  */
}

internal void push_window_to_first_docked(UiContext* context,
                                          U32Array* window_array,
                                          const u32 index)
{
    if ((!window_array->size) || (index == 0))
    {
        return;
    }
    u32 index_to_insert = 0;
    u32 temp = window_array->data[(index)];
    for (u32 i = (index); i >= 1; --i)
    {
        if (ui_window_get(window_array->data[i - 1])->docked)
        {
            index_to_insert = i;
            break;
        }
        window_array->data[i] = window_array->data[i - 1];
    }
    window_array->data[index_to_insert] = temp;
}

#define move_to_back(type, array, index)                                       \
    do                                                                         \
    {                                                                          \
        type temp = (array)->data[(index)];                                    \
        for (u32 i = (index); i < (array)->size - 1; ++i)                      \
        {                                                                      \
            (array)->data[i] = (array)->data[i + 1];                           \
        }                                                                      \
        (array)->data[(array)->size - 1] = temp;                               \
    } while (0)

internal void push_window_to_back(UiContext* context, U32Array* window_array,
                                  const u32 index)
{
    if ((!window_array->size) || (index == (window_array->size - 1)))
    {
        return;
    }
    u32 temp = window_array->data[index];
    for (u32 i = index; i < window_array->size - 1; ++i)
    {
        window_array->data[i] = window_array->data[i + 1];
    }
    window_array->data[window_array->size - 1] = temp;
    /*
    move_to_back(UiWindow, &context->windows, window_in_focus_index);
    for (u32 i = 0; i < context->windows.size; ++i)
    {
        context->id_to_index.data[context->windows.data[i].id] = i;
        context->windows.data[i].dock_node->window = context->windows.data + i;
    }
    move_to_back(AABBArray, &context->window_aabbs, window_in_focus_index);
    move_to_back(HoverClickedIndex, &context->window_hover_clicked_indices,
                 window_in_focus_index);
                 */
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

internal u32 get_window_index(const u32 id_index)
{
    return ui_context.id_to_index
        .data[ui_context.last_frame_windows.data[id_index]];
}

void ui_window_start_size_animation(UiWindow* window, V2 start_size,
                                    V2 end_size)
{
    window->size_animation_precent = 0.0f;
    window->size_before_animation = start_size;
    window->size_after_animation = end_size;
    window->size_animation_on = true;
}

void ui_window_start_position_animation(UiWindow* window, V2 start_position,
                                        V2 end_position)
{
    window->position_animation_precent = 0.0f;
    window->position_before_animation = start_position;
    window->position_after_animation = end_position;
    window->position_animation_on = true;
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
             ui_context.docking_color, 0.0f);
        *index_count += 6;
        result = true;
    }
    quad_border_rounded(&ui_context.render.vertices, index_count, aabb.min,
                        aabb.size, secondary_color, 1.0f, 0.4f, 3, 0.0f);
    return result;
}

internal void update_window_dock_nodes(DockNode* dock_node)
{
    for (u32 i = 0; i < dock_node->windows.size; ++i)
    {
        ui_window_get(dock_node->windows.data[i])->dock_node = dock_node;
    }
}

DockNode* dock_node_create_(NodeType type, SplitAxis split_axis)
{
    DockNode* node = (DockNode*)calloc(1, sizeof(DockNode));
    node->type = type;
    node->split_axis = split_axis;
    node->size_ratio = 0.5f;
    return node;
}

DockNode* dock_node_create(NodeType type, SplitAxis split_axis, i32 window)
{
    DockNode* node = dock_node_create_(type, split_axis);
    if (window != -1)
    {
        array_create(&node->windows, 2);
        array_push(&node->windows, window);
    }
    return node;
}

DockNode* dock_node_create_multiple_windows(NodeType type, SplitAxis split_axis,
                                            U32Array windows)
{
    DockNode* node = dock_node_create_(type, split_axis);
    node->windows = windows;
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
    for (u32 i = 0; i < left->windows.size; ++i)
    {
        UiWindow* left_window = ui_window_get(left->windows.data[i]);
        left_window->position = left->aabb.min;
        if (ui_context.dock_resize)
        {
            left_window->position = left->aabb.min;
            left_window->size = left->aabb.size;
        }
        else
        {
            ui_window_start_position_animation(
                left_window, left_window->position, left->aabb.min);
            ui_window_start_size_animation(left_window, left_window->size,
                                           left->aabb.size);
        }
    }
    DockNode* right = split_node->children[1];
    for (u32 i = 0; i < right->windows.size; ++i)
    {
        UiWindow* right_window = ui_window_get(right->windows.data[i]);
        if (ui_context.dock_resize)
        {
            right_window->position = right->aabb.min;
            right_window->size = right->aabb.size;
        }
        else
        {
            ui_window_start_position_animation(
                right_window, right_window->position, right->aabb.min);
            ui_window_start_size_animation(right_window, right_window->size,
                                           right->aabb.size);
        }
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
        if (copy_node == NULL)
        {
            for (u32 i = 0; i < window->windows.size; ++i)
            {
                UiWindow* ui_window = ui_window_get(window->windows.data[i]);
                ui_window->position = root->aabb.min;
                ui_window->size = root->aabb.size;
            }
            root->children[0] = window;
            return;
        }
        // TODO: might be a memory leak
        split_node = dock_node_create(NODE_PARENT, split_axis, -1);
    }
    else if (root->type == NODE_LEAF)
    {
        copy_node = dock_node_create_multiple_windows(NODE_LEAF, SPLIT_NONE,
                                                      root->windows);
        copy_node->window_in_focus = root->window_in_focus;
        update_window_dock_nodes(copy_node);
    }
    else
    {
        ftic_assert(false);
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
    root->windows.size = 0;
    root->windows.data = NULL;
}

void dock_node_resize_from_root(DockNode* root, const AABB* aabb)
{
    root->aabb = *aabb;
    DockNode* left = root->children[0];
    if (left != NULL)
    {
        left->aabb = root->aabb;
        for (u32 i = 0; i < left->windows.size; ++i)
        {
            UiWindow* left_window = ui_window_get(left->windows.data[i]);
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
    // TODO: i believe we can check the adress of the first data
    if ((root->type == NODE_LEAF) && (root->windows.data == node->windows.data))
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
        root->children[0]->windows.data == node_to_remove->windows.data)
    {
        root->children[0] = NULL;
        return;
    }
    DockNode* node_before = find_node(root->children[0], root, node_to_remove);
    DockNode* left = node_before->children[0];
    DockNode* right = node_before->children[1];
    if (left->type == NODE_LEAF &&
        left->windows.data == node_to_remove->windows.data)
    {
        node_before->type = right->type;
        node_before->split_axis = right->split_axis;
        node_before->windows = right->windows;
        node_before->window_in_focus = right->window_in_focus;
        node_before->children[0] = right->children[0];
        node_before->children[1] = right->children[1];
        node_before->size_ratio = right->size_ratio;
        if (right->type == NODE_LEAF)
        {
            update_window_dock_nodes(node_before);
        }
        free(right);
    }
    else if (right->type == NODE_LEAF &&
             right->windows.data == node_to_remove->windows.data)
    {
        node_before->type = left->type;
        node_before->split_axis = left->split_axis;
        node_before->windows = left->windows;
        node_before->window_in_focus = left->window_in_focus;
        node_before->children[0] = left->children[0];
        node_before->children[1] = left->children[1];
        node_before->size_ratio = left->size_ratio;
        if (left->type == NODE_LEAF)
        {
            update_window_dock_nodes(node_before);
        }
        free(left);
    }
    dock_node_resize_from_root(root, &root->aabb);
}

internal i32 display_docking(const AABB* dock_aabb, const V2 top_position,
                             const V2 right_position, const V2 bottom_position,
                             const V2 left_position,
                             const b8 should_have_middle)
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

    if (should_have_middle)
    {
        const V2 docking_size_middle = v2i(50.0f);
        const f32 x =
            middle((right_position.x + docking_size_left_right.width) -
                       left_position.x,
                   docking_size_middle.width);
        const f32 y =
            middle((bottom_position.y + docking_size_top_bottom.height) -
                       top_position.y,
                   docking_size_middle.height);

        const V2 middle_position = v2f(left_position.x + x, top_position.y + y);
        if (set_docking(middle_position, docking_size_middle, dock_aabb->min,
                        dock_aabb->size, &ui_context.extra_index_count))
        {
            index = 4;
        }
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
        v2f(middle_top_bottom.x,
            (dock_node->aabb.min.y + dock_node->aabb.size.height) -
                (docking_size_top_bottom.height + 10.0f));

    const V2 left_position =
        v2f(dock_node->aabb.min.x + 10.0f, middle_left_right.y);

    i32 index = display_docking(&dock_node->aabb, top_position, right_position,
                                bottom_position, left_position, false);
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
                                bottom_position, left_position, true);
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
    array_create(&input.chars, 20);
    array_create(&input.chars_selected, 20);
    return input;
}

void ui_input_buffer_clear_selection(InputBuffer* input)
{
    input->chars_selected.size = 0;
    input->start_selection_index = -1;
    input->end_selection_index = -1;
}

char* ui_input_buffer_get_selection_as_string(InputBuffer* input)
{
    char* text = (char*)calloc(input->chars_selected.size + 1, sizeof(char));
    for (u32 i = 0; i < input->chars_selected.size; ++i)
    {
        text[i] = input->chars_selected.data[i].character;
    }
    return text;
}

void ui_input_buffer_copy_selection_to_clipboard(InputBuffer* input)
{
    char* text = ui_input_buffer_get_selection_as_string(input);
    window_set_clipboard(text);
    free(text);
}

void ui_input_buffer_erase_from_selection(InputBuffer* input)
{
    for (u32 i = input->start_selection_index, j = input->end_selection_index;
         j < input->buffer.size; ++i, ++j)
    {
        input->buffer.data[i] = input->buffer.data[j];
    }
    input->input_index = input->start_selection_index;
    input->buffer.size -=
        input->end_selection_index - input->start_selection_index;

    ui_input_buffer_clear_selection(input);
    input->buffer.data[input->buffer.size] = '\0';
}

internal void insert_window(DockNode* node, const u32 id, const b8 docked)
{
    UiWindow window = {
        .position = v2f(200.0f, 200.0f),
        .id = id,
        .size = v2f(200.0f, 200.0f),
        .top_color = clear_color,
        .bottom_color = clear_color,
        .dock_node = node,
        .docked = docked,
    };
    array_push(&ui_context.windows, window);

    AABBArray aabbs = { 0 };
    array_create(&aabbs, 10);
    array_push(&ui_context.window_aabbs, aabbs);

    HoverClickedIndex hover_clicked_index = { .index = -1 };
    array_push(&ui_context.window_hover_clicked_indices, hover_clicked_index);
}

internal void write_node(FILE* file, DockNode* node)
{
    if (node == NULL)
    {
        b8 is_null = true;
        fwrite(&is_null, sizeof(is_null), 1, file);
        return;
    }

    b8 is_null = false;
    fwrite(&is_null, sizeof(is_null), 1, file);
    fwrite(&node->type, sizeof(node->type), 1, file);
    fwrite(&node->split_axis, sizeof(node->split_axis), 1, file);
    fwrite(&node->window_in_focus, sizeof(node->window_in_focus), 1, file);
    fwrite(&node->windows.size, sizeof(node->windows.size), 1, file);
    for (u32 i = 0; i < node->windows.size; ++i)
    {
        fwrite(&node->windows.data[i], sizeof(node->windows.data[i]), 1, file);
    }
    fwrite(&node->size_ratio, sizeof(node->size_ratio), 1, file);

    write_node(file, node->children[0]);
    write_node(file, node->children[1]);
}

internal void save_layout()
{
    FILE* file = fopen("saved/ui_layout.txt", "wb");
    if (file == NULL)
    {
        log_file_error("saved/ui_layout.txt");
        return;
    }
    write_node(file, ui_context.dock_tree);
    fclose(file);
}

DockNode* read_node(FILE* file)
{
    b8 is_null = false;
    size_t count = fread(&is_null, sizeof(uint8_t), 1, file);
    if (is_null || !count)
    {
        return NULL;
    }

    DockNode* node = (DockNode*)calloc(1, sizeof(DockNode));
    ftic_assert(fread(&node->type, sizeof(node->type), 1, file));
    ftic_assert(fread(&node->split_axis, sizeof(node->split_axis), 1, file));
    ftic_assert(
        fread(&node->window_in_focus, sizeof(node->window_in_focus), 1, file));
    ftic_assert(
        fread(&node->windows.size, sizeof(node->windows.size), 1, file));

    if (node->windows.size)
    {
        u32 size = node->windows.size;
        array_create(&node->windows, 2);
        for (u32 i = 0; i < size; ++i)
        {
            u32 window_id = 0;
            ftic_assert(fread(&window_id, sizeof(window_id), 1, file));
            array_push(&node->windows, window_id);
            insert_window(node, window_id, true);
            if (i != node->window_in_focus)
            {
                array_back(&ui_context.windows)->hide = true;
            }
        }
    }
    else
    {
        node->windows.capacity = 0;
        node->windows.data = NULL;
    }

    ftic_assert(fread(&node->size_ratio, sizeof(node->size_ratio), 1, file));

    node->children[0] = read_node(file);
    node->children[1] = read_node(file);

    return node;
}

internal DockNode* load_layout()
{
    FILE* file = fopen("saved/ui_layout.txt", "rb");
    if (file == NULL)
    {
        log_file_error("saved/ui_layout.txt");
        return NULL;
    }

    DockNode* root = read_node(file);
    fclose(file);
    return root;
}

void ui_context_create()
{
    array_create(&ui_context.id_to_index, 100);
    array_create(&ui_context.free_indices, 100);

    array_create(&ui_context.last_frame_windows, 10);
    array_create(&ui_context.current_frame_windows, 10);

    array_create(&ui_context.windows, 10);
    array_create(&ui_context.window_aabbs, 10);
    array_create(&ui_context.window_hover_clicked_indices, 10);

    DockNode* saved_tree = load_layout();
    if (saved_tree)
    {
        ui_context.dock_tree = saved_tree;
    }
    else
    {
        ui_context.dock_tree = dock_node_create(NODE_ROOT, SPLIT_NONE, -1);
    }
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

    // TODO: make all of these icons into a sprite sheet.
    u32 default_texture = create_default_texture();
    u32 arrow_icon_texture =
        load_icon_as_white("res/icons/arrow_sprite_sheet.png");
    u32 list_icon_texture = load_icon_as_white("res/icons/list.png");
    u32 grid_icon_texture = load_icon_as_white("res/icons/grid.png");

    u32 folder_icon_texture = load_icon("res/icons/folder.png");
    u32 file_icon_texture = load_icon("res/icons/file.png");
    u32 file_png_icon_texture = load_icon("res/icons/png.png");
    u32 file_jpg_icon_texture = load_icon("res/icons/jpg.png");
    u32 file_pdf_icon_texture = load_icon("res/icons/pdf.png");
    u32 file_java_icon_texture = load_icon("res/icons/java.png");
    u32 file_cpp_icon_texture = load_icon("res/icons/cpp.png");
    u32 file_c_icon_texture = load_icon("res/icons/c.png");

    u32 folder_icon_big_texture = load_icon("res/icons/folderbig.png");
    u32 file_icon_big_texture = load_icon("res/icons/filebig.png");
    u32 file_png_icon_big_texture = load_icon("res/icons/pngbig.png");
    u32 file_jpg_icon_big_texture = load_icon("res/icons/jpgbig.png");
    u32 file_pdf_icon_big_texture = load_icon("res/icons/pdfbig.png");
    u32 file_java_icon_big_texture = load_icon("res/icons/javabig.png");
    u32 file_cpp_icon_big_texture = load_icon("res/icons/cppbig.png");
    u32 file_c_icon_big_texture = load_icon("res/icons/cbig.png");

    U32Array textures = { 0 };
    array_create(&textures, 21);
    array_push(&textures, default_texture);
    array_push(&textures, font_texture);
    array_push(&textures, arrow_icon_texture);
    array_push(&textures, list_icon_texture);
    array_push(&textures, grid_icon_texture);

    array_push(&textures, folder_icon_texture);
    array_push(&textures, file_icon_texture);
    array_push(&textures, file_png_icon_texture);
    array_push(&textures, file_jpg_icon_texture);
    array_push(&textures, file_pdf_icon_texture);
    array_push(&textures, file_java_icon_texture);
    array_push(&textures, file_cpp_icon_texture);
    array_push(&textures, file_c_icon_texture);

    array_push(&textures, folder_icon_big_texture);
    array_push(&textures, file_icon_big_texture);
    array_push(&textures, file_png_icon_big_texture);
    array_push(&textures, file_jpg_icon_big_texture);
    array_push(&textures, file_pdf_icon_big_texture);
    array_push(&textures, file_java_icon_big_texture);
    array_push(&textures, file_cpp_icon_big_texture);
    array_push(&textures, file_c_icon_big_texture);

    ui_context.render = rendering_properties_initialize(
        shader, textures, &vertex_buffer_layout, 100 * 4, 100 * 4);

    ui_context.default_textures_offset = textures.size;

    ui_context.docking_color =
        v4f(secondary_color.r, secondary_color.g, secondary_color.b, 0.4f);
}

void ui_context_begin(const V2 dimensions, const AABB* dock_space,
                      const f64 delta_time, const b8 check_collisions)
{
    if (!aabb_equal(&ui_context.dock_space, dock_space))
    {
        dock_node_resize_from_root(ui_context.dock_tree, dock_space);
    }
    ui_context.check_collisions = check_collisions;
    ui_context.dock_space = *dock_space;
    ui_context.dimensions = dimensions;
    ui_context.render.textures.size = ui_context.default_textures_offset;

    ui_context.delta_time = delta_time;

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
    ui_context.any_window_top_bar_hold = false;
    ui_context.any_window_hold = false;
    i32 index_window_dragging = -1;
    for (u32 i = 0; i < ui_context.last_frame_windows.size; ++i)
    {
        u32 window_index =
            ui_context.id_to_index.data[ui_context.last_frame_windows.data[i]];
        UiWindow* window = ui_context.windows.data + window_index;
        window->rendering_index_count = 0;
        window->rendering_index_offset = 0;
        window->area_hit = window->any_holding;
        ui_context.any_window_hold |= window->any_holding;
        ui_context.any_window_top_bar_hold |= window->top_bar_hold;
        if (window->top_bar_hold)
        {
            index_window_dragging = i;
        }
    }
    if (index_window_dragging != -1)
    {
        ui_context.window_in_focus = get_window_index(index_window_dragging);
        push_window_to_back(&ui_context, &ui_context.last_frame_windows,
                            index_window_dragging);
    }
    if (check_collisions && !ui_context.dock_resize_hover &&
        !ui_context.dock_resize && !ui_context.any_window_top_bar_hold &&
        !ui_context.any_window_hold)
    {
        const b8 mouse_button_clicked =
            event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_LEFT);

        const MouseButtonEvent* mouse_button_event =
            event_get_mouse_button_event();

        const b8 mouse_button_pressed =
            mouse_button_event->activated &&
            mouse_button_event->action == FTIC_PRESS;

        const V2 mouse_position = event_get_mouse_position();
        for (i32 i = ((i32)ui_context.last_frame_windows.size) - 1; i >= 0; --i)
        {
            u32 window_index = get_window_index(i);
            UiWindow* window = ui_context.windows.data + window_index;

            if (window->hide) continue;

            const AABBArray* aabbs =
                ui_context.window_aabbs.data + window_index;

            if (aabbs->size)
            {
                // NOTE: the first aabb is the window area
                if (!collision_point_in_aabb(mouse_position, &aabbs->data[0]))
                {
                    continue;
                }
                else
                {
                    window->area_hit = true;
                }
            }

            for (i32 aabb_index = ((i32)aabbs->size) - 1; aabb_index >= 0;
                 --aabb_index)
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
                        if (!window->docked)
                        {
                            push_window_to_back(
                                &ui_context, &ui_context.last_frame_windows, i);
                        }
                        ui_context.window_in_focus = window_index;
                    }
                    goto collision_check_done;
                }
            }
        }
    }
collision_check_done:;
}

internal b8 look_for_stale_windows(DockNode* root)
{
    if (root == NULL) return false;

    if (root->type == NODE_LEAF)
    {
        i32 window_id_to_remove = -1;
        for (u32 i = 0; i < ui_context.last_frame_windows.size; ++i)
        {
            for (u32 j = 0; j < root->windows.size; ++j)
            {
                if (root->windows.data[j] ==
                    ui_context.last_frame_windows.data[i])
                {
                    window_id_to_remove = j;
                    break;
                }
            }
        }
        /*
        if (window_id_to_remove == -1)
        {
            if (root->windows.size == 1)
            {
                dock_node_remove_node(ui_context.dock_tree, root);
            }
            for (u32 i = 0; i < root->windows.size; ++i)
            {
                ui_window_get(root->windows.data[i])->docked = false;
            }
            return true;
        }
        */
    }
    else
    {
        b8 removed = look_for_stale_windows(root->children[0]);
        if (removed) return removed;
        removed = look_for_stale_windows(root->children[1]);
        if (removed) return removed;
    }
    return false;
}

internal void clean_dock_tree_of_stale_windows()
{
    while (look_for_stale_windows(ui_context.dock_tree))
        ;
}

void ui_context_end()
{
    ui_context.extra_index_offset = ui_context.current_index_offset;
    ui_context.extra_index_count = 0;

#if 1
    if (ui_context.any_window_top_bar_hold)
    {
        i32 hit = root_display_docking(ui_context.dock_tree);
        ui_context.dock_side_hit = hit;
        hit = dock_node_docking_display_traverse(ui_context.dock_tree);
        if (hit != -1) ui_context.dock_side_hit = hit;
    }
    else if (!ui_context.any_window_hold)
    {
        b8 any_hit = false;
        UiWindow* focused_window =
            ui_context.windows.data + ui_context.window_in_focus;
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
        else if (ui_context.dock_side_hit == 4)
        {
            DockNode* dock_node = focused_window->dock_node;
            for (u32 i = 0; i < dock_node->windows.size; ++i)
            {
                u32 window_id = dock_node->windows.data[i];
                array_push(&ui_context.dock_hit_node->windows, window_id);

                UiWindow* window = ui_window_get(window_id);
                window->dock_node = ui_context.dock_hit_node;
                window->position = ui_context.dock_hit_node->aabb.min;
                window->size = ui_context.dock_hit_node->aabb.size;
                window->docked = true;
            }
            for (u32 i = 0; i < ui_context.dock_hit_node->windows.size; ++i)
            {
                DockNode* node = ui_context.dock_hit_node;
                u32 window_id = node->windows.data[i];
                UiWindow* window = ui_window_get(window_id);
                if (window_id == focused_window->id)
                {
                    window->hide = false;
                    node->window_in_focus = i;
                    ui_context.window_in_focus =
                        ui_context.id_to_index.data[window->id];
                }
                else
                {
                    window->hide = true;
                }
            }
            free(dock_node->windows.data);
            dock_node->windows.data = NULL;
            free(dock_node);
        }
        if (any_hit)
        {
            focused_window->docked = true;
            push_window_to_first_docked(&ui_context,
                                        &ui_context.last_frame_windows,
                                        ui_context.last_frame_windows.size - 1);
        }
        ui_context.dock_side_hit = -1;
        // ui_context.dock_hit_node = NULL;

        const MouseButtonEvent* event = event_get_mouse_button_event();
        if (event->action == FTIC_RELEASE)
        {
            ui_context.dock_resize = false;
        }
        ui_context.dock_resize_hover = false;

        if (ui_context.check_collisions && !event_get_key_event()->alt_pressed)
        {
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

                    FTicWindow* ftic_window = window_get_current();
                    if (ui_context.dock_hit_node->split_axis ==
                        SPLIT_HORIZONTAL)
                    {
                        window_set_cursor(ftic_window, FTIC_RESIZE_V_CURSOR);
                    }
                    else
                    {
                        window_set_cursor(ftic_window, FTIC_RESIZE_H_CURSOR);
                    }
                    quad(&ui_context.render.vertices,
                         ui_context.dock_resize_aabb.min,
                         ui_context.dock_resize_aabb.size,
                         v4_s_multi(secondary_color, 0.8f), 0.0f);
                    ui_context.extra_index_count += 6;
                }
                else
                {
                    window_set_cursor(window_get_current(), FTIC_NORMAL_CURSOR);
                }
            }
            else
            {

                const V2 relative_mouse_position =
                    v2_sub(event_get_mouse_position(),
                           ui_context.dock_hit_node->aabb.min);

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
                if (closed_interval(0.1f, new_ratio, 0.9f))
                {
                    ui_context.dock_hit_node->size_ratio = new_ratio;
                    dock_node_resize_traverse(ui_context.dock_hit_node);
                }
                ui_context.dock_resize_aabb =
                    dock_node_set_resize_aabb(ui_context.dock_hit_node);
                quad(&ui_context.render.vertices,
                     ui_context.dock_resize_aabb.min,
                     ui_context.dock_resize_aabb.size, secondary_color, 0.0f);
                ui_context.extra_index_count += 6;
            }
        }
    }

#endif

    if (event_is_mouse_button_pressed_once(FTIC_MOUSE_BUTTON_LEFT))
    {
        ui_context.mouse_drag_box_point = event_get_mouse_position();
    }

    if (event_is_mouse_button_pressed(FTIC_MOUSE_BUTTON_LEFT) &&
        event_get_key_event()->alt_pressed)
    {
        const V2 mouse_position = event_get_mouse_position();
        const V2 min_point =
            v2f(min(ui_context.mouse_drag_box_point.x, mouse_position.x),
                min(ui_context.mouse_drag_box_point.y, mouse_position.y));
        const V2 max_point =
            v2f(max(ui_context.mouse_drag_box_point.x, mouse_position.x),
                max(ui_context.mouse_drag_box_point.y, mouse_position.y));

        AABB mouse_drag_box = {
            .min = min_point,
            .size = v2_sub(max_point, min_point),
        };

        quad(&ui_context.render.vertices, mouse_drag_box.min,
             mouse_drag_box.size, ui_context.docking_color, 0.0f);
        ui_context.extra_index_count += 6;

        V4 drag_border_color = ui_context.docking_color;
        drag_border_color.a = 0.9f;
        quad_border(&ui_context.render.vertices, &ui_context.extra_index_count,
                    mouse_drag_box.min, mouse_drag_box.size, drag_border_color,
                    1.0f, 0.0f);

        ui_context.mouse_drag_box = mouse_drag_box;
        ui_context.mouse_box_is_dragging = true;
    }
    else
    {
        ui_context.mouse_drag_box = (AABB){ 0 };
        ui_context.mouse_box_is_dragging = false;
    }

    ui_context.current_index_offset =
        ui_context.extra_index_offset + ui_context.extra_index_count;

    b8 any_change = false;
    // TODO: can be very expensive. Consider a more efficient way.
    for (u32 i = 0; i < ui_context.last_frame_windows.size; ++i)
    {
        u32 window = ui_context.last_frame_windows.data[i];
        b8 exist = false;
        for (u32 j = 0; j < ui_context.current_frame_windows.size; ++j)
        {
            if (ui_context.current_frame_windows.data[j] == window)
            {
                push_window_to_back(&ui_context,
                                    &ui_context.current_frame_windows, j--);
                ui_context.current_frame_windows.size--;
                exist = true;
                break;
            }
        }

        if (!exist)
        {
            push_window_to_back(&ui_context, &ui_context.last_frame_windows,
                                i--);
            ui_context.last_frame_windows.size--;
            any_change = true;
        }
    }

    for (u32 i = 0; i < ui_context.current_frame_windows.size; ++i)
    {
        array_push(&ui_context.last_frame_windows,
                   ui_context.current_frame_windows.data[i]);
        any_change = true;
    }
    ui_context.current_frame_windows.size = 0;

    // NOTE: this is probably more expensive because the window count is usually
    // under 10
    // SetU64 dock_cache = {0};
    // set_create_u64(100, hash_u64);
    DockNodePtrArray dock_spaces = { 0 };
    array_create(&dock_spaces, 10);

    for (u32 i = 0; i < ui_context.last_frame_windows.size; ++i)
    {
        u32 window_index = get_window_index(i);
        UiWindow* window = ui_context.windows.data + window_index;
        b8 exist = false;
        for (u32 j = 0; j < dock_spaces.size; ++j)
        {
            if (dock_spaces.data[j] == window->dock_node)
            {
                exist = true;
            }
        }
        if (!exist)
        {
            array_push(&dock_spaces, window->dock_node);
        }
    }

    // NOTE: these are here to change the tab after the render
    DockNode* dock_space_to_change = NULL;
    u32 new_window_focus = 0;
    UiWindow* window_to_hide = NULL;
    UiWindow* window_to_show = NULL;
    b8 close_tab = false;

    const V2 mouse_position = event_get_mouse_position();
    b8 any_tab_hit = false;
    b8 any_hit = false;
    UU32Array dock_spaces_index_offsets_and_counts = { 0 };
    array_create(&dock_spaces_index_offsets_and_counts, dock_spaces.size);
    for (i32 i = ((i32)dock_spaces.size) - 1; i >= 0; --i)
    {
        DockNode* dock_space = dock_spaces.data[i];
        UiWindow* window = ui_window_get(
            dock_space->windows.data[dock_space->window_in_focus]);
        UU32 index_offset_and_count = {
            .first = ui_context.current_index_offset,
        };
        if (dock_space->window_in_focus >= dock_space->windows.size)
        {
            log_u64("Window in focus: ", dock_space->window_in_focus);
            log_u64("Size: ", dock_space->windows.size);
            if (window->title)
                log_message(window->title, strlen(window->title));
        }
        if (window->top_bar)
        {
            const b8 should_check_collision =
                ui_context.check_collisions && !ui_context.dock_resize_hover &&
                !ui_context.dock_resize &&
                !ui_context.any_window_top_bar_hold &&
                !ui_context.any_window_hold;

            const V2 top_bar_dimensions = v2f(window->size.width, 20.0f);
            AABB aabb = quad_gradiant_t_b(&ui_context.render.vertices,
                                          window->position, top_bar_dimensions,
                                          v4ic(0.25f), v4ic(0.2f), 0.0f);
            index_offset_and_count.second += 6;

            V2 tab_position = window->position;
            for (i32 j = 0; j < (i32)dock_space->windows.size; ++j)
            {
                UiWindow* tab_window =
                    ui_window_get(dock_space->windows.data[j]);

                const V2 button_size = v2i(top_bar_dimensions.height - 4.0f);

                f32 x_advance =
                    tab_window->title
                        ? text_x_advance(ui_context.font.chars,
                                         tab_window->title,
                                         (u32)strlen(tab_window->title), 1.0f)
                        : 0.0f;

                const f32 tab_padding = 8.0f;
                V2 tab_dimensions = v2f(x_advance + (tab_padding * 2.0f) +
                                            button_size.width + 2.0f,
                                        top_bar_dimensions.height);
                AABB tab_aabb = {
                    .min = tab_position,
                    .size = tab_dimensions,
                };

                V4 tab_color = v4ic(0.25f);
                if (j == (i32)dock_space->window_in_focus)
                {
                    tab_color = v4ic(0.35f);
                }
                b8 tab_collided = false;
                if (should_check_collision && !close_tab && !any_tab_hit &&
                    collision_point_in_aabb(mouse_position, &tab_aabb))
                {
                    if (j != (i32)dock_space->window_in_focus)
                    {
                        if (event_is_mouse_button_clicked(
                                FTIC_MOUSE_BUTTON_LEFT))
                        {
                            dock_space_to_change = dock_space;
                            new_window_focus = j;
                            window_to_hide = window;
                            window_to_show = tab_window;
                        }
                        tab_color = v4ic(0.3f);
                    }
                    any_tab_hit = true;
                    tab_collided = true;
                }

                quad(&ui_context.render.vertices, tab_position, tab_dimensions,
                     tab_color, 0.0f);
                index_offset_and_count.second += 6;

                V2 text_position =
                    v2f(tab_position.x + tab_padding,
                        tab_position.y + ui_context.font.pixel_height);
                f32 title_advance = 0.0f;
                if (tab_window->title)
                {
                    index_offset_and_count.second += text_generation(
                        ui_context.font.chars, tab_window->title,
                        UI_FONT_TEXTURE, text_position, 1.0f,
                        ui_context.font.line_height, NULL, &title_advance, NULL,
                        &ui_context.render.vertices);
                }

                V2 button_position =
                    v2f(text_position.x + x_advance + tab_padding,
                        tab_position.y + 2.0f);

                AABB button_aabb = {
                    .min = button_position,
                    .size = button_size,
                };

                b8 collided =
                    collision_point_in_aabb(mouse_position, &button_aabb);

                if (should_check_collision && collided)
                {
                    if (event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_LEFT))
                    {
                        close_tab = true;
                        dock_space_to_change = dock_space;
                        window_to_hide = tab_window;
                        new_window_focus = j;
                    }
                }

                V4 button_color = tab_color;
                if (collided &&
                    event_get_mouse_button_event()->action != FTIC_PRESS)
                {
                    button_color = v4ic(0.5f);
                }
                else if (tab_collided)
                {
                    button_color = v4ic(0.4f);
                }

                quad(&ui_context.render.vertices, button_position, button_size,
                     button_color, 0.0f);
                index_offset_and_count.second += 6;

                tab_position.x +=
                    x_advance + (tab_padding * 2.0f) + button_size.width + 5.0f;
            }

            quad_border(&ui_context.render.vertices,
                        &index_offset_and_count.second, window->position,
                        top_bar_dimensions, border_color, 1.0f, 0.0f);

            if (should_check_collision && !close_tab && !any_tab_hit &&
                !any_hit && collision_point_in_aabb(mouse_position, &aabb) &&
                event_is_mouse_button_pressed_once(FTIC_MOUSE_BUTTON_LEFT))
            {
                window->top_bar_offset = event_get_mouse_position();
                window->top_bar_pressed = true;
                window->any_holding = true;
                any_hit = true;
            }
        }
        dock_spaces_index_offsets_and_counts.data[i] = index_offset_and_count;
        ui_context.current_index_offset =
            index_offset_and_count.first + index_offset_and_count.second;
    }

    /*
    if (any_change)
    {
        clean_dock_tree_of_stale_windows();
    }
    */

    rendering_properties_check_and_grow_buffers(
        &ui_context.render, ui_context.current_index_offset);

    buffer_set_sub_data(ui_context.render.vertex_buffer_id, GL_ARRAY_BUFFER, 0,
                        sizeof(Vertex) * ui_context.render.vertices.size,
                        ui_context.render.vertices.data);

    rendering_properties_begin_draw(&ui_context.render, &ui_context.mvp);
    for (u32 i = 0; i < dock_spaces.size; ++i)
    {
        DockNode* node = dock_spaces.data[i];
        UiWindow* window =
            ui_window_get(node->windows.data[node->window_in_focus]);

        AABB scissor = { 0 };
        if (ui_context.dimensions.y)
        {
            scissor.min.x = window->position.x;
            scissor.min.y = ui_context.dimensions.y -
                            (window->position.y + window->size.height);
            scissor.size = window->size;
        }
        rendering_properties_draw(window->rendering_index_offset,
                                  window->rendering_index_count, &scissor);

        UU32 index_offset_and_count =
            dock_spaces_index_offsets_and_counts.data[i];
        rendering_properties_draw(index_offset_and_count.first,
                                  index_offset_and_count.second, &scissor);
    }
    AABB whole_screen_scissor = { .size = ui_context.dimensions };
    rendering_properties_draw(ui_context.extra_index_offset,
                              ui_context.extra_index_count,
                              &whole_screen_scissor);
    rendering_properties_end_draw(&ui_context.render);

    if (dock_space_to_change)
    {
        b8 change_window_to_focus = false;
        if (close_tab)
        {
            b8 focused_window =
                new_window_focus == dock_space_to_change->window_in_focus;

            for (u32 i = new_window_focus;
                 i < dock_space_to_change->windows.size; ++i)
            {
                dock_space_to_change->windows.data[i] =
                    dock_space_to_change->windows.data[i + 1];
                if ((i + 1) == dock_space_to_change->window_in_focus)
                {
                    dock_space_to_change->window_in_focus = i;
                }
            }
            dock_space_to_change->windows.size--;

            window_to_hide->closing = true;
            window_to_hide->hide = false;
            window_to_hide->docked = false;
            window_to_hide->dock_node =
                dock_node_create(NODE_LEAF, SPLIT_NONE, window_to_hide->id);
            if (focused_window)
            {
                dock_space_to_change->window_in_focus = 0;
                window_to_show = ui_window_get(
                    dock_space_to_change->windows
                        .data[dock_space_to_change->window_in_focus]);
                window_to_show->position = window_to_hide->position;
                window_to_show->size = window_to_hide->size;
                window_to_show->hide = false;
            }
        }
        else
        {
            dock_space_to_change->window_in_focus = new_window_focus;
            window_to_hide->hide = true;
            window_to_show->position = window_to_hide->position;
            window_to_show->size = window_to_hide->size;
            window_to_show->hide = false;
        }
        ui_context.window_in_focus =
            ui_context.id_to_index.data[window_to_show->id];
    }

    // set_clear_u64(&dock_cache);
    // free(dock_cache.cells);
    free(dock_spaces.data);
    free(dock_spaces_index_offsets_and_counts.data);
}

void ui_context_destroy()
{
    save_layout();
}

u32 ui_window_create()
{
    u32 id = get_id(ui_context.windows.size, &ui_context.free_indices,
                    &ui_context.id_to_index);

    for (u32 i = 0; i < ui_context.windows.size; ++i)
    {
        UiWindow* window = ui_context.windows.data + i;
        if (window->id == id)
        {
            ui_context.id_to_index.data[id] = i;
            return id;
        }
    }
    DockNode* node = dock_node_create(NODE_LEAF, SPLIT_NONE, id);
    insert_window(node, id, false);
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

internal void window_animate(UiWindow* window)
{
    const f32 animation_speed = 4.0f;
    if (window->size_animation_on)
    {
        if (window->size_animation_precent >= 1.0f)
        {
            window->size = window->size_after_animation;
            window->size_animation_on = false;
        }
        else
        {
            window->size = v2_lerp(
                window->size_before_animation, window->size_after_animation,
                ease_out_cubic(window->size_animation_precent));
        }
        window->size_animation_precent +=
            clampf32_low((f32)ui_context.delta_time, 0.0f) * animation_speed;

        if (window->top_bar_hold)
        {
            const V2 mouse_position = event_get_mouse_position();
            window->position.x = mouse_position.x - (window->size.width * 0.5f);
            window->top_bar_offset = v2_sub(window->position, mouse_position);
        }
    }
    if (window->position_animation_on)
    {
        if (window->position_animation_precent >= 1.0f)
        {
            window->position = window->position_after_animation;
            window->position_animation_on = false;
        }
        else
        {
            window->position =
                v2_lerp(window->position_before_animation,
                        window->position_after_animation,
                        ease_out_cubic(window->position_animation_precent));
        }
        window->position_animation_precent +=
            clampf32_low((f32)ui_context.delta_time, 0.0f) * animation_speed;
    }
}

b8 ui_window_begin(u32 window_id, const char* title, b8 top_bar)
{
    const u32 window_index = ui_context.id_to_index.data[window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;

    if (!window->closing || window->size_animation_on ||
        window->position_animation_on)
    {
        array_push(&ui_context.current_frame_windows, window->id);
    }

    ui_context.current_window_id = window_id;

    window->title = title;
    window->rendering_index_offset = ui_context.current_index_offset;
    aabbs->size = 0;

    const f32 top_bar_height = 20.0f;
    window->top_bar = top_bar;
    if (window->top_bar && !ui_context.mouse_box_is_dragging)
    {
        if (window->top_bar_hold)
        {
            window->position =
                v2_add(event_get_mouse_position(), window->top_bar_offset);

            window->position.y =
                clampf32_high(window->position.y, ui_context.dimensions.height -
                                                      window->size.height);
            window->position.y = clampf32_low(window->position.y, 0.0f);

            window->position.x =
                clampf32_high(window->position.x,
                              ui_context.dimensions.width - window->size.width);
            window->position.x = clampf32_low(window->position.x, 0.0f);

            window->dock_node->aabb.min = window->position;
            window->dock_node->aabb.size = window->size;
        }
        else if (window->top_bar_pressed)
        {
            if (v2_distance(window->top_bar_offset,
                            event_get_mouse_position()) >= 10.0f)
            {
                const V2 mouse_position = event_get_mouse_position();
                if (window->docked)
                {
                    window->docked = false;
                    dock_node_remove_node(ui_context.dock_tree,
                                          window->dock_node);

                    window->position.y =
                        mouse_position.y - (top_bar_height * 0.5f);

                    ui_window_start_size_animation(window, window->size,
                                                   v2i(200.0f));
                }
                window->top_bar_hold = true;
                window->top_bar_offset =
                    v2_sub(window->position, mouse_position);
            }
        }
    }

    window_animate(window);

    window->first_item_position = window->position;
    window->first_item_position.y += top_bar_height * top_bar;

    window->total_height = 0.0f;
    window->total_width = 0.0f;

    if (window->hide) return false;

    array_push(aabbs,
               quad_gradiant_t_b(&ui_context.render.vertices, window->position,
                                 window->size, window->top_color,
                                 window->bottom_color, 0.0f));
    window->rendering_index_count += 6;

    // v2_add_equal(&window->size, window->resize_offset);

    return true;
}

internal void add_scroll_bar_height(UiWindow* window, AABBArray* aabbs,
                                    HoverClickedIndex hover_clicked_index)
{
    const f32 scroll_bar_width = 8.0f;
    V2 position = v2f(window->position.x + window->size.width,
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
            window->any_holding = true;
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

internal void add_scroll_bar_width(UiWindow* window, AABBArray* aabbs,
                                   HoverClickedIndex hover_clicked_index)
{
    const f32 scroll_bar_height = 8.0f;
    V2 position = v2f(window->position.x, window->position.y + window->size.y);
    position.y -= scroll_bar_height;

    const f32 area_width = window->size.width;
    const f32 total_width = window->total_width;
    if (area_width < total_width)
    {
        quad(&ui_context.render.vertices, position,
             v2f(area_width, scroll_bar_height), high_light_color, 0.0f);
        window->rendering_index_count += 6;

        const f32 initial_x = position.x;
        const f32 high = 0.0f;
        const f32 low = area_width - total_width;
        const f32 p =
            (window->current_scroll_offset_width - low) / (high - low);
        const V2 scroll_bar_dimensions =
            v2f(area_width * (area_width / total_width), scroll_bar_height);

        const f32 lower_end = window->position.x + window->size.width;
        position.x =
            lerp_f32(lower_end - scroll_bar_dimensions.width, initial_x, p);

        V2 mouse_position = event_get_mouse_position();
        b8 collided = hover_clicked_index.index == (i32)aabbs->size;

        AABB scroll_bar_aabb = {
            .min = position,
            .size = scroll_bar_dimensions,
        };
        array_push(aabbs, scroll_bar_aabb);

        if (collided || window->scroll_bar_width.dragging)
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
            window->any_holding = true;
            window->scroll_bar_width.dragging = true;
            window->scroll_bar_width.mouse_pointer_offset =
                scroll_bar_aabb.min.x - mouse_position.x;
        }

        if (window->scroll_bar_width.dragging)
        {
            const f32 end_position_x = lower_end - scroll_bar_dimensions.width;

            f32 new_x = mouse_position.x +
                        window->scroll_bar_width.mouse_pointer_offset;

            // Clipping
            if (new_x < initial_x)
            {
                window->scroll_bar_width.mouse_pointer_offset =
                    scroll_bar_aabb.min.x - mouse_position.x;
                new_x = initial_x;
            }
            if (new_x > end_position_x)
            {
                window->scroll_bar_width.mouse_pointer_offset =
                    scroll_bar_aabb.min.x - mouse_position.x;
                new_x = end_position_x;
            }

            const f32 offset_p =
                (new_x - initial_x) / (end_position_x - initial_x);

            f32 lerp = lerp_f32(high, low, offset_p);

            lerp = clampf32_high(lerp, high);
            lerp = clampf32_low(lerp, low);
            window->current_scroll_offset_width = lerp;
            window->end_scroll_offset_width = lerp;
        }
    }
}

b8 ui_window_end()
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;

    if (window->hide) return false;

    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;

    const V2 top_bar_dimensions = v2f(window->size.width, 20.0f);
    HoverClickedIndex hover_clicked_index =
        ui_context.window_hover_clicked_indices.data[window_index];

    if (event_get_mouse_button_event()->action == FTIC_RELEASE)
    {
        window->top_bar_pressed = false;
        window->top_bar_hold = false;
        window->any_holding = false;
        window->resize_dragging = false;
        window->scroll_bar.dragging = false;
        window->scroll_bar_width.dragging = false;
        window->last_resize_offset = window->resize_offset;
    }

    if (window->top_bar)
    {
        if (hover_clicked_index.index == (i32)aabbs->size)
        {
            if (hover_clicked_index.pressed && !window->top_bar_pressed)
            {
                window->top_bar_offset = event_get_mouse_position();
                window->top_bar_pressed = true;
                window->any_holding = true;
            }
        }

        f32 x_advance = 0.0f;

        /*
        if (closable)
        {
            x_advance += 10.0f;

            V2 button_position =
                v2f(window->position.x + x_advance, window->position.y + 2.0f);
            V2 button_size = v2i(top_bar_dimensions.height - 4.0f);

            b8 collided = hover_clicked_index.index == (i32)aabbs->size;
            if (collided && hover_clicked_index.clicked)
            {
                window->closing = true;
                V2 end_position =
                    v2_add(window->position, v2_s_multi(window->size, 0.5f));
                V2 end_size = v2i(0.0f);
                ui_window_start_position_animation(window, window->position,
                                                   end_position);
                ui_window_start_size_animation(window, window->size, end_size);
            }

            V4 button_color = v4ic(0.3f);
            if (collided &&
                event_get_mouse_button_event()->action != FTIC_PRESS)
            {
                button_color = v4ic(0.4f);
            }
            array_push(aabbs, quad(&ui_context.render.vertices, button_position,
                                   button_size, button_color, 0.0f));
            window->rendering_index_count += 6;
        }
        */
    }

    add_scroll_bar_height(window, aabbs, hover_clicked_index);
    add_scroll_bar_width(window, aabbs, hover_clicked_index);

    if (!window->scroll_bar.dragging)
    {
        if (event_get_mouse_wheel_event()->activated && window->area_hit)
        {
            window->end_scroll_offset =
                set_scroll_offset(window->total_height, window->size.y,
                                  window->end_scroll_offset);
        }
        if (ui_context.delta_time < 0.5f)
        {
            window->current_scroll_offset =
                smooth_scroll(ui_context.delta_time, window->end_scroll_offset,
                              window->current_scroll_offset);
        }
    }

    const b8 in_focus = window_index == ui_context.window_in_focus;
    V4 color = border_color;
    if (in_focus)
    {
        color = secondary_color;
    }
    quad_border(&ui_context.render.vertices, &window->rendering_index_count,
                window->position, window->size, color, 1.0f, 0.0f);

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
                window->any_holding = true;
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

    if (window->closing && !window->size_animation_on &&
        !window->position_animation_on)
    {
        window->closing = false;
        return true;
    }
    return false;
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

b8 ui_window_add_icon_button(V2 position, const V2 size, const V4 hover_color,
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

    if (hover && event_get_mouse_button_event()->action == FTIC_RELEASE)
    {
        quad(&ui_context.render.vertices, button_aabb.min, button_aabb.size,
             hover_color, 0.0f);
        window->rendering_index_count += 6;
    }

    V2 icon_size = v2_s_multi(button_aabb.size, 0.7f);
    V2 icon_position =
        v2_add(position, v2f(middle(size.width, icon_size.width),
                             middle(size.height, icon_size.height)));
    v2_add_equal(&position, window->first_item_position);

    quad_co(&ui_context.render.vertices, icon_position, icon_size, button_color,
            texture_coordinates, texture_index);
    window->rendering_index_count += 6;

    return clicked;
}

internal void input_buffer_select_char(InputBuffer* input, u32 index)
{
    if (input->start_selection_index == -1)
    {
        input->start_selection_index = index;
    }
    input->end_selection_index = index + 1;
    array_push(&input->chars_selected, input->chars.data[index]);
}

internal u32 render_input(const f64 delta_time, const V2 text_position,
                          InputBuffer* input)
{
    u32 index_count = 0;
    // Blinking cursor
    if (input->active && input->chars_selected.size == 0)
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
        if (event_is_key_pressed_repeat(FTIC_KEY_LEFT))
        {
            input->input_index = max(input->input_index - 1, 0);
            input->time = 0.4f;
        }
        if (event_is_key_pressed_repeat(FTIC_KEY_RIGHT))
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
                v2f(1.0f, 16.0f), v4i(1.0f), 0.0f);
            index_count += 6;

            input->time = input->time >= 0.8f ? 0 : input->time;
        }
    }
    input->chars.size = 0;
    if (input->buffer.size)
    {
        index_count += text_generation(
            ui_context.font.chars, input->buffer.data, UI_FONT_TEXTURE,
            v2f(text_position.x,
                text_position.y + ui_context.font.pixel_height),
            1.0f, ui_context.font.line_height, NULL, NULL, &input->chars,
            &ui_context.render.vertices);
    }
    if (event_is_mouse_button_pressed(FTIC_MOUSE_BUTTON_LEFT))
    {
        const V2 mouse_position = event_get_mouse_position();
        if (event_get_mouse_button_event()->activated)
        {
            // This happens only once when the button is pressed.
            input->selected_pivot_point = mouse_position.x;
            input->selected = true;
            input->time = 0.4f;
        }
        ui_input_buffer_clear_selection(input);
        if (input->active && input->chars.size)
        {
            for (u32 i = 0; i < input->chars.size; ++i)
            {
                const AABB* current_aabb = &input->chars.data[i].aabb;
                const f32 middle_point =
                    current_aabb->min.x + (current_aabb->size.width * 0.5f);

                if (mouse_position.x > middle_point)
                {
                    input->input_index = i + 1;
                    if (mouse_position.x > input->selected_pivot_point &&
                        middle_point > input->selected_pivot_point)
                    {
                        input_buffer_select_char(input, i);
                    }
                }
                else if (mouse_position.x < input->selected_pivot_point &&
                         middle_point < input->selected_pivot_point)
                {
                    input_buffer_select_char(input, i);
                }
            }
        }
    }
    else
    {
        input->selected = false;
    }
    if (input->chars_selected.size)
    {
        if (event_is_ctrl_and_key_pressed(FTIC_KEY_C))
        {
            ui_input_buffer_copy_selection_to_clipboard(input);
        }
        const f32 position_x = input->chars_selected.data[0].aabb.min.x;
        const AABB* last_char = &array_back(&input->chars_selected)->aabb;
        const f32 size_width =
            (last_char->min.x + last_char->size.width) - position_x;
        quad(&ui_context.render.vertices, v2f(position_x, text_position.y),
             v2f(size_width, ui_context.font.pixel_height + 5.0f),
             ui_context.docking_color, 0.0f);
        index_count += 6;
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
            if (input->chars_selected.size)
            {
                ui_input_buffer_erase_from_selection(input);
            }
            else
            {
                input->input_index--;
                for (u32 i = input->input_index; i < input->buffer.size; ++i)
                {
                    input->buffer.data[i] = input->buffer.data[i + 1];
                }
                input->buffer.data[--input->buffer.size] = '\0';
            }
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

    if (event_is_mouse_button_pressed(FTIC_MOUSE_BUTTON_LEFT) ||
        event_is_mouse_button_pressed(FTIC_MOUSE_BUTTON_RIGHT))
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

        if (event_is_ctrl_and_key_pressed(FTIC_KEY_V))
        {
            const char* clip_board = window_get_clipboard();
            if (clip_board)
            {
                const u32 clip_board_length = (u32)strlen(clip_board);
                for (u32 i = 0; i < clip_board_length; ++i)
                {
                    array_push(&input->buffer, clip_board[i]);
                    for (i32 j = ((i32)input->buffer.size) - 1;
                         j > input->input_index; --j)
                    {
                        input->buffer.data[j] = input->buffer.data[j - 1];
                    }
                    input->buffer.data[input->input_index++] = clip_board[i];
                }
                array_push(&input->buffer, '\0');
                input->buffer.size--;
                typed = true;
            }
        }
        else if (event_is_ctrl_and_key_pressed(FTIC_KEY_X))
        {
            if (input->chars_selected.size)
            {
                ui_input_buffer_copy_selection_to_clipboard(input);
                ui_input_buffer_erase_from_selection(input);
                typed = true;
            }
        }
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
        text_generation(ui_context.font.chars, text, UI_FONT_TEXTURE, position,
                        1.0f, ui_context.font.pixel_height, NULL, NULL, NULL,
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

internal b8 check_directory_item_collision(
    V2 starting_position, V2 item_dimensions, const DirectoryItem* item,
    V4* color, b8* selected, SelectedItemValues* selected_item_values)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;
    HoverClickedIndex hover_clicked_index =
        ui_context.window_hover_clicked_indices.data[window_index];

    u32* check_if_selected = NULL;
    if (selected_item_values)
    {
        check_if_selected = hash_table_get_char_u32(
            &selected_item_values->selected_items, item->path);
        *selected = check_if_selected ? true : false;
    }

    AABB aabb = { .min = starting_position, .size = item_dimensions };
    b8 hit = hover_clicked_index.index == (i32)aabbs->size ||
             (collision_aabb_in_aabb(&ui_context.mouse_drag_box, &aabb) &&
              event_get_key_event()->alt_pressed);

    if (hit && selected_item_values &&
        event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_LEFT))
    {
        if (!check_if_selected)
        {
            const u32 path_length = (u32)strlen(item->path);
            char* path = (char*)calloc(path_length + 3, sizeof(char));
            memcpy(path, item->path, path_length);
            hash_table_insert_char_u32(&selected_item_values->selected_items,
                                       path, 1);
            array_push(&selected_item_values->paths, path);
            *selected = true;
        }
        else
        {
            directory_remove_selected_item(selected_item_values, item->path);
        }
    }

    *color = clear_color;
    V4 left_side_color = clear_color;
    if (hit || *selected)
    {
        const V4 end_color = *selected ? v4ic(0.45f) : v4ic(0.3f);
#if 0
        color = v4_lerp(selected ? v4ic(0.5f) : border_color, end_color,
                        ((f32)sin(pulse_x) + 1.0f) / 2.0f);
#else
        *color = end_color;
#endif
    }

    return hit;
}

internal f32 get_file_icon_based_on_extension(const f32 icon_index,
                                              const char* name)
{
    b8 small = icon_index == UI_FILE_ICON_TEXTURE;
    if (small || icon_index == UI_FILE_ICON_BIG_TEXTURE)
    {
        const char* extension = file_get_extension(name, (u32)strlen(name));
        if (extension)
        {
            if (strcmp(extension, ".png") == 0)
            {
                return small ? UI_FILE_PNG_ICON_TEXTURE
                             : UI_FILE_PNG_ICON_BIG_TEXTURE;
            }
            else if (strcmp(extension, ".jpg") == 0)
            {
                return small ? UI_FILE_JPG_ICON_TEXTURE
                             : UI_FILE_JPG_ICON_BIG_TEXTURE;
            }
            else if (strcmp(extension, ".pdf") == 0)
            {
                return small ? UI_FILE_PDF_ICON_TEXTURE
                             : UI_FILE_PDF_ICON_BIG_TEXTURE;
            }
            else if (strcmp(extension, ".cpp") == 0)
            {
                return small ? UI_FILE_CPP_ICON_TEXTURE
                             : UI_FILE_CPP_ICON_BIG_TEXTURE;
            }
            else if (strcmp(extension, ".c") == 0 || strcmp(extension, ".h") == 0)
            {
                return small ? UI_FILE_C_ICON_TEXTURE
                             : UI_FILE_C_ICON_BIG_TEXTURE;
            }
            else if (strcmp(extension, ".java") == 0)
            {
                return small ? UI_FILE_JAVA_ICON_TEXTURE
                             : UI_FILE_JAVA_ICON_BIG_TEXTURE;
            }
        }
    }
    return icon_index;
}

internal b8 directory_item(V2 starting_position, V2 item_dimensions,
                           f32 icon_index, const DirectoryItem* item,
                           SelectedItemValues* selected_item_values)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;
    HoverClickedIndex hover_clicked_index =
        ui_context.window_hover_clicked_indices.data[window_index];

    V4 color = clear_color;
    b8 selected = false;
    b8 hit =
        check_directory_item_collision(starting_position, item_dimensions, item,
                                       &color, &selected, selected_item_values);

    array_push(aabbs,
               quad_gradiant_l_r(&ui_context.render.vertices, starting_position,
                                 item_dimensions, color, clear_color, 0.0f));
    window->rendering_index_count += 6;

    if (selected || hit)
    {
        starting_position.x += 8.0f;
        item_dimensions.width -= 6.0f;
    }

    icon_index = get_file_icon_based_on_extension(icon_index, item->name);

    const V2 icon_size = v2i(24.0f);
    AABB icon_aabb =
        quad(&ui_context.render.vertices,
             v2f(starting_position.x + 5.0f, starting_position.y + 3.0f),
             icon_size, v4i(1.0f), icon_index);
    window->rendering_index_count += 6;

    V2 text_position =
        v2_s_add(starting_position, ui_context.font.pixel_height + 5.0f);

    text_position.x += icon_aabb.size.width - 5.0f;

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
            ui_context.font.chars, buffer, UI_FONT_TEXTURE, size_text_position,
            1.0f, ui_context.font.pixel_height, NULL, NULL, NULL,
            &ui_context.render.vertices);
    }
    window->rendering_index_count += display_text_and_truncate_if_necissary(
        text_position,
        (item_dimensions.width - icon_aabb.size.x - 20.0f - x_advance),
        item->name);

    return hit && hover_clicked_index.double_clicked;
}

internal b8 directory_item_grid(V2 starting_position, V2 item_dimensions,
                                f32 icon_index, const DirectoryItem* item,
                                SelectedItemValues* selected_item_values)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;
    HoverClickedIndex hover_clicked_index =
        ui_context.window_hover_clicked_indices.data[window_index];

    V4 color = clear_color;
    b8 selected = false;
    b8 hit =
        check_directory_item_collision(starting_position, item_dimensions, item,
                                       &color, &selected, selected_item_values);

    array_push(aabbs, quad(&ui_context.render.vertices, starting_position,
                           item_dimensions, color, 0.0f));
    window->rendering_index_count += 6;

    icon_index = get_file_icon_based_on_extension(icon_index, item->name);

    const V2 icon_size = v2i(128.0f);
    AABB icon_aabb =
        quad(&ui_context.render.vertices,
             v2f(starting_position.x +
                     middle(item_dimensions.width, icon_size.width),
                 starting_position.y + 3.0f),
             icon_size, v4i(1.0f), icon_index);
    window->rendering_index_count += 6;

    const f32 total_available_width_for_text = item_dimensions.width;

    f32 x_advance = text_x_advance(ui_context.font.chars, item->name,
                                   (u32)strlen(item->name), 1.0f);

    x_advance = min(x_advance, total_available_width_for_text);

    V2 text_position = starting_position;
    text_position.x += middle(total_available_width_for_text, x_advance);
    text_position.y += ui_context.font.pixel_height;
    text_position.y += icon_aabb.size.height;

    window->rendering_index_count += display_text_and_truncate_if_necissary(
        text_position, total_available_width_for_text, item->name);

    return hit && hover_clicked_index.double_clicked;
}

i32 ui_window_add_directory_item_item_list(
    V2 position, const DirectoryItemArray* items, const f32 icon_index,
    const f32 item_height, SelectedItemValues* selected_item_values)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    V2 relative_position = position;
    v2_add_equal(&position, window->first_item_position);

    position.y += window->current_scroll_offset;
    position.x += window->current_scroll_offset_width;

    V2 item_dimensions =
        v2f(window->size.width - relative_position.x, item_height);

    i32 hit_index = -1;
    for (i32 i = 0; i < (i32)items->size; ++i)
    {
        if (item_in_view(position.y, item_dimensions.height,
                         position.y + window->size.height))
        {
            if (directory_item(position, item_dimensions, icon_index,
                               &items->data[i], selected_item_values))
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

internal void display_grid_item(const V2 position, const i32 index,
                                const DirectoryItemArray* folders,
                                const DirectoryItemArray* files,
                                const V2 item_dimensions, II32* hit_index,
                                SelectedItemValues* selected_item_values)
{
    if (index < (i32)folders->size)
    {
        DirectoryItem* item = folders->data + index;
        if (directory_item_grid(position, item_dimensions,
                                UI_FOLDER_ICON_BIG_TEXTURE, item,
                                selected_item_values))
        {
            hit_index->first = 0;
            hit_index->second = index;
        }
    }
    else
    {
        const i32 new_index = index - folders->size;
        DirectoryItem* item = files->data + new_index;
        if (directory_item_grid(position, item_dimensions,
                                UI_FILE_ICON_BIG_TEXTURE, item,
                                selected_item_values))
        {
            hit_index->first = 1;
            hit_index->second = index - new_index;
        }
    }
}

II32 ui_window_add_directory_item_grid(V2 position,
                                       const DirectoryItemArray* folders,
                                       const DirectoryItemArray* files,
                                       const f32 item_height,
                                       SelectedItemValues* selected_item_values)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    V2 relative_position = position;
    v2_add_equal(&position, window->first_item_position);

    position.y += window->current_scroll_offset;
    position.x += window->current_scroll_offset_width;

    V2 item_dimensions = v2i(item_height);
    item_dimensions.height += (ui_context.font.pixel_height + 5.0f);

    const u32 item_count = folders->size + files->size;
    const f32 total_area_space = window->size.width - relative_position.x;
    const f32 grid_padding = 5.0f;
    const i32 columns = max(
        (i32)(total_area_space / (item_dimensions.width + grid_padding)), 1);
    const i32 rows = item_count / columns;
    const i32 last_row = item_count % columns;

    const f32 start_x = position.x;

    II32 hit_index = { .first = -1, .second = -1 };
    for (i32 row = 0; row < rows; ++row)
    {
        if (item_in_view(position.y, item_dimensions.height,
                         position.y + window->size.height))
        {
            for (i32 column = 0; column < columns; ++column)
            {
                const i32 index = (row * columns) + column;
                display_grid_item(position, index, folders, files,
                                  item_dimensions, &hit_index,
                                  selected_item_values);
                position.x += item_dimensions.width + grid_padding;
            }
            position.x = start_x;
        }
        position.y += item_dimensions.height + grid_padding;
        relative_position.y += item_dimensions.height + grid_padding;
    }
    if (item_in_view(position.y, item_dimensions.height,
                     position.y + window->size.height))
    {
        for (i32 column = 0; column < last_row; ++column)
        {
            const i32 index = (rows * columns) + column;
            display_grid_item(position, index, folders, files, item_dimensions,
                              &hit_index, selected_item_values);
            position.x += item_height + grid_padding;
        }
    }
    relative_position.y +=
        (item_dimensions.height + grid_padding) * (last_row > 0);

    window->total_height = max(
        window->total_height, relative_position.y + item_height + grid_padding);

    return hit_index;
}

b8 ui_window_add_folder_list(V2 position, const f32 item_height,
                             const DirectoryItemArray* items,
                             SelectedItemValues* selected_item_values,
                             i32* item_selected)
{
    i32 selected_index = ui_window_add_directory_item_item_list(
        position, items, UI_FOLDER_ICON_TEXTURE, item_height,
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
        position, items, UI_FILE_ICON_TEXTURE, item_height,
        selected_item_values);
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
    if (in_focus && event_is_key_pressed_repeat(FTIC_KEY_TAB))
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

    b8 mouse_button_clicked =
        event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_1);
    b8 enter_clicked = event_is_key_clicked(FTIC_KEY_ENTER);
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
           event_is_key_clicked(FTIC_KEY_ESCAPE);
}

void ui_window_add_text(V2 position, const char* text, b8 scrolling)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;

    V2 relative_position = position;
    v2_add_equal(&position, window->first_item_position);

    if (ui_context.row)
    {
        position.x += ui_context.row_current;
    }

    position.y += window->current_scroll_offset + ui_context.font.pixel_height;
    position.x += window->current_scroll_offset_width;

    u32 new_lines = 0;
    f32 x_advance = 0.0f;
    window->rendering_index_count +=
        text_generation(ui_context.font.chars, text, UI_FONT_TEXTURE, position,
                        1.0f, ui_context.font.line_height, &new_lines,
                        &x_advance, NULL, &ui_context.render.vertices);

    if (ui_context.row)
    {
        ui_context.row_current += x_advance + ui_context.padding;
    }

    if (scrolling)
    {
        window->total_height = max(
            window->total_height,
            relative_position.y + (++new_lines * ui_context.font.line_height));

        window->total_width =
            max(window->total_width, relative_position.x + x_advance + 20.0f);
    }
}

void ui_window_add_text_colored(V2 position, const ColoredCharacterArray* text,
                                b8 scrolling)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;

    V2 relative_position = position;
    v2_add_equal(&position, window->first_item_position);

    if (ui_context.row)
    {
        position.x += ui_context.row_current;
    }

    position.y += window->current_scroll_offset + ui_context.font.pixel_height;
    position.x += window->current_scroll_offset_width;

    u32 new_lines = 0;
    f32 x_advance = 0.0f;
    window->rendering_index_count += text_generation_colored_char(
        ui_context.font.chars, text, UI_FONT_TEXTURE, position, 1.0f,
        ui_context.font.line_height, &new_lines, &x_advance, NULL,
        &ui_context.render.vertices);

    if (ui_context.row)
    {
        ui_context.row_current += x_advance + ui_context.padding;
    }

    if (scrolling)
    {
        window->total_height = max(
            window->total_height,
            relative_position.y + (++new_lines * ui_context.font.line_height));

        window->total_width =
            max(window->total_width, relative_position.x + x_advance + 20.0f);
    }
}

b8 ui_window_set_overlay()
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;

    // TODO
    for (i32 i = ((i32)ui_context.last_frame_windows.size) - 1; i >= 0; --i)
    {
        if (ui_context.last_frame_windows.data[i] == window->id)
        {
            push_window_to_back(&ui_context, &ui_context.last_frame_windows, i);
            ui_context.window_in_focus =
                ui_context.id_to_index.data[window->id];
        }
    }

    return !window->area_hit &&
           (event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_LEFT) ||
            event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_RIGHT));
}

void ui_window_add_image(V2 position, V2 image_dimensions, u32 image)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;

    V2 relative_position = position;
    v2_add_equal(&position, window->first_item_position);

    position.x += window->current_scroll_offset_width;
    position.y += window->current_scroll_offset;

    f32 texture_index = (f32)ui_context.render.textures.size;
    array_push(&ui_context.render.textures, image);

    quad(&ui_context.render.vertices, position, image_dimensions, v4i(1.0f),
         texture_index);
    window->rendering_index_count += 6;

    window->total_height = max(window->total_height,
                               relative_position.y + image_dimensions.height);

    window->total_width =
        max(window->total_width, relative_position.x + image_dimensions.width);
}

b8 ui_window_add_button(V2 position, V2* dimensions, const V4* color,
                        const char* text)
{

    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;
    HoverClickedIndex hover_clicked_index =
        ui_context.window_hover_clicked_indices.data[window_index];

    V2 relative_position = position;
    v2_add_equal(&position, window->first_item_position);

    f32 x_advance = 20.0f;
    f32 pixel_height = 10.0f;
    if (text)
    {
        x_advance += text_x_advance(ui_context.font.chars, text,
                                    (u32)strlen(text), 1.0f);
        pixel_height += ui_context.font.pixel_height;
    }

    V2 end_dimensions = v2f(max(dimensions->width, x_advance),
                            max(dimensions->height, pixel_height));

    *dimensions = end_dimensions;
    if (ui_context.row)
    {
        position.x += ui_context.row_current;
        ui_context.row_current += end_dimensions.width + ui_context.padding;
    }

    b8 collided = hover_clicked_index.index == (i32)aabbs->size;
    V4 button_color = color ? *color : v4ic(0.3f);
    if (collided && event_get_mouse_button_event()->action != FTIC_PRESS)
    {
        button_color = v4_s_multi(button_color, 1.2f);
        button_color.a = 1.0f;
        if (!color)
        {
            quad(&ui_context.render.vertices, position, end_dimensions,
                 button_color, 0.0f);
            window->rendering_index_count += 6;
        }
    }
    if (color)
    {
        quad(&ui_context.render.vertices, position, end_dimensions,
             button_color, 0.0f);
        window->rendering_index_count += 6;
    }

    AABB button_aabb = { .min = position, .size = end_dimensions };
    array_push(aabbs, button_aabb);

    if (text)
    {
        V2 text_position =
            v2f(position.x + 10.0f,
                position.y + ui_context.font.pixel_height + 1.0f);
        window->rendering_index_count +=
            text_generation(ui_context.font.chars, text, UI_FONT_TEXTURE,
                            text_position, 1.0f, ui_context.font.line_height,
                            NULL, NULL, NULL, &ui_context.render.vertices);
    }

    return collided && hover_clicked_index.clicked;
}

i32 ui_window_add_menu_bar(CharPtrArray* values, V2* position_of_clicked_item)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;

    i32 index = -1;

    f32 pixel_height = 10.0f + ui_context.font.pixel_height;
    quad(&ui_context.render.vertices, window->position,
         v2f(ui_context.dimensions.width, pixel_height), v4ic(0.1f), 0.0f);
    window->rendering_index_count += 6;

    ui_window_row_begin(0.0f);
    for (u32 i = 0; i < values->size; i++)
    {
        V2 current_dimensions = v2d();
        const f32 current_x_position = ui_context.row_current;
        if (ui_window_add_button(window->position, &current_dimensions, NULL,
                                 values->data[i]))
        {
            if (position_of_clicked_item)
            {
                *position_of_clicked_item =
                    v2f(current_x_position,
                        window->position.y + current_dimensions.height);
            }
            index = i;
        }
    }
    ui_window_row_end();

    return index;
}

void ui_window_add_icon(V2 position, const V2 size,
                        const V4 texture_coordinates, const f32 texture_index)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;

    v2_add_equal(&position, window->first_item_position);

    if (ui_context.row)
    {
        position.x += ui_context.row_current;
        ui_context.row_current += size.width + ui_context.padding;
    }
    quad_co(&ui_context.render.vertices, position, size, v4ic(1.0f),
            texture_coordinates, texture_index);
    window->rendering_index_count += 6;
}
