#include "ui.h"
#include "event.h"
#include "application.h"
#include "opengl_util.h"
#include "texture.h"
#include "logging.h"
#include "set.h"
#include "hash.h"
#include "globals.h"
#include "particle_system.h"
#include "random.h"
#include <string.h>
#include <stdio.h>
#include <glad/glad.h>
#include <math.h>

global u32 random_seed = 0;

#define MAX_PATH 260

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

    U32Array last_frame_top_bar_windows;
    U32Array current_frame_top_bar_windows;

    U32Array last_frame_overlay_windows;
    U32Array current_frame_overlay_windows;

    RenderingProperties frosted_render;
    f32 frosted_blur_amount;
    i32 frosted_samples;
    i32 frosted_blur_amount_location;
    i32 frosted_samples_location;

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

    UnorderedCircularParticleBuffer particles;
    u32 particles_index_offset;

    V4 docking_color;

    char font_path[MAX_PATH];

    V2 dimensions;
    f64 delta_time;

    // TODO: nested ones
    f32 row_padding;
    f32 row_current;
    f32 column_padding;
    f32 column_current;
    b8 row;
    b8 column;

    // TODO: Better solution?
    b8 any_window_top_bar_hold;
    b8 any_window_hold;

    b8 dock_resize;
    b8 dock_resize_hover;
    b8 check_collisions;

    b8 animation_off;
    b8 highlight_fucused_window_off;

    b8 non_docked_window_hover;
} UiContext;

typedef struct UU32Array
{
    u32 size;
    u32 capacity;
    UU32* data;
} UU32Array;

global UiContext ui_context = { 0 };
global f32 ui_big_icon_size = 84.0f;

f32 ui_get_big_icon_size()
{
    return ui_big_icon_size;
}
void ui_set_big_icon_size(f32 new_size)
{
    ui_big_icon_size = new_size;
    ui_big_icon_size = clampf32_high(ui_big_icon_size, 128.0f);
    ui_big_icon_size = clampf32_low(ui_big_icon_size, 64.0f);
}

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
                           const f32 speed, f32 scroll_offset)
{
    scroll_offset += (f32)((offset - scroll_offset) * (delta_time * speed));
    scroll_offset += (offset - scroll_offset) * ui_context.animation_off;
    return scroll_offset;
}

internal void push_window_to_front(U32Array* window_array, const u32 index)
{
    if ((!window_array->size) || (index == 0) || (index >= window_array->size))
    {
        return;
    }
    u32 temp = window_array->data[index];
    for (u32 i = index; i >= 1; --i)
    {
        window_array->data[i] = window_array->data[i - 1];
    }
    window_array->data[0] = temp;
}

internal void push_window_to_first_docked(U32Array* window_array,
                                          const u32 index)
{
    if ((!window_array->size) || (index == 0) || (index >= window_array->size))
    {
        return;
    }
    u32 index_to_insert = 0;
    u32 temp = window_array->data[index];
    for (u32 i = index; i >= 1; --i)
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

internal void push_window_to_back(U32Array* window_array, const u32 index)
{
    if ((!window_array->size) || (index == (window_array->size - 1)) ||
        (index >= window_array->size))
    {
        return;
    }
    u32 temp = window_array->data[index];
    for (u32 i = index; i < window_array->size - 1; ++i)
    {
        window_array->data[i] = window_array->data[i + 1];
    }
    window_array->data[window_array->size - 1] = temp;
}

internal u32 generate_id(const u32 index, U32Array* free_ids,
                         U32Array* id_to_index)
{
    u32 id = 0;
    if (free_ids->size)
    {
        id = *array_back(free_ids);
        free_ids->size--;
        id_to_index->data[id] = index;
    }
    else
    {
        id = id_to_index->size;
        array_push(id_to_index, index);
    }
    return id;
}

internal u32 get_window_index(const u32 id_index)
{
    return ui_context.id_to_index
        .data[ui_context.last_frame_windows.data[id_index]];
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
                        aabb.size, global_get_secondary_color(), 2.0f, 0.4f, 3,
                        0.0f);
    return result;
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
    if (set_docking(top_position, docking_size_top_bottom, dock_aabb->min,
                    expanded_size_top_bottom, &ui_context.extra_index_count))
    {
        index = 0;
    }
    if (set_docking(right_position, docking_size_left_right,
                    v2f(dock_aabb->min.x + expanded_size_left_right.width,
                        dock_aabb->min.y),
                    expanded_size_left_right, &ui_context.extra_index_count))
    {
        index = 1;
    }
    if (set_docking(bottom_position, docking_size_top_bottom,
                    v2f(dock_aabb->min.x,
                        dock_aabb->min.y + expanded_size_top_bottom.height),
                    expanded_size_top_bottom, &ui_context.extra_index_count))
    {
        index = 2;
    }
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

internal void update_window_dock_nodes(DockNode* dock_node)
{
    for (u32 i = 0; i < dock_node->windows.size; ++i)
    {
        ui_window_get(dock_node->windows.data[i])->dock_node = dock_node;
    }
}

internal DockNode* dock_node_create_(NodeType type, SplitAxis split_axis)
{
    DockNode* node = (DockNode*)calloc(1, sizeof(DockNode));
    node->type = type;
    node->split_axis = split_axis;
    node->size_ratio = 0.5f;
    return node;
}

internal DockNode* dock_node_create(NodeType type, SplitAxis split_axis,
                                    i32 window)
{
    DockNode* node = dock_node_create_(type, split_axis);
    if (window != -1)
    {
        array_create(&node->windows, 2);
        array_push(&node->windows, window);
    }
    return node;
}

internal DockNode* dock_node_create_multiple_windows(NodeType type,
                                                     SplitAxis split_axis,
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

internal void dock_node_resize_traverse(DockNode* split_node)
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

internal void dock_node_dock_window(DockNode* root, DockNode* window,
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

internal void dock_node_resize_from_root(DockNode* root, const AABB* aabb)
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

internal DockNode* find_node(DockNode* root, DockNode* node_before,
                             DockNode* node)
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

internal void dock_node_remove_node(DockNode* root, DockNode* node_to_remove)
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

internal u32 remove_window_from_shared_dock_space(
    const u32 window_index, UiWindow* window_to_remove,
    DockNode* dock_space_to_change)
{
    b8 focused_window = window_index == dock_space_to_change->window_in_focus;

    for (u32 i = window_index; i < dock_space_to_change->windows.size - 1; ++i)
    {
        dock_space_to_change->windows.data[i] =
            dock_space_to_change->windows.data[i + 1];
        if ((i + 1) == dock_space_to_change->window_in_focus)
        {
            dock_space_to_change->window_in_focus = i;
        }
    }
    dock_space_to_change->windows.size--;

    window_to_remove->closing = true;
    window_to_remove->hide = false;
    window_to_remove->docked = false;
    window_to_remove->dock_node =
        dock_node_create(NODE_LEAF, SPLIT_NONE, window_to_remove->id);

    if (focused_window)
    {
        dock_space_to_change->window_in_focus = 0;
        UiWindow* window_to_show =
            ui_window_get(dock_space_to_change->windows
                              .data[dock_space_to_change->window_in_focus]);
        window_to_show->position = window_to_remove->position;
        window_to_show->size = window_to_remove->size;
        window_to_show->hide = false;
    }
    return dock_space_to_change->windows
        .data[dock_space_to_change->window_in_focus];
}

internal i32 root_display_docking(DockNode* dock_node)
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

internal i32 leaf_display_docking(DockNode* dock_node)
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

internal i32 look_for_docking_collisions(DockNode* parent)
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
        i32 hit = look_for_docking_collisions(left);
        if (hit != -1) return hit;
    }
    if (right->type == NODE_PARENT)
    {
        i32 hit = look_for_docking_collisions(right);
        if (hit != -1) return hit;
    }
    return -1;
}

internal i32 dock_node_docking_display_traverse(DockNode* root)
{
    if (root->children[0] && root->children[0]->type == NODE_PARENT)
    {
        return look_for_docking_collisions(root->children[0]);
    }
    return -1;
}

internal AABB dock_node_set_resize_aabb(DockNode* node)
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

internal b8 look_for_resize_collisions(DockNode* parent)
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

internal b8 dock_node_resize_collision_traverse(DockNode* root)
{
    if (root->children[0] && root->children[0]->type == NODE_PARENT)
    {
        return look_for_resize_collisions(root->children[0]);
    }
    return false;
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

void ui_input_buffer_delete(InputBuffer* input)
{
    array_free(&input->buffer);
    array_free(&input->chars);
    array_free(&input->chars_selected);
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
        .top_color = global_get_clear_color(),
        .bottom_color = global_get_clear_color(),
        .dock_node = node,
        .docked = docked,
        .alpha = 1.0f,
        .back_ground_alpha = 1.0f,
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

internal f32 gamma_correct(f32 value, f32 gamma)
{
    return powf(value, 1.0f / gamma);
}

internal void get_window_resize_aabbs(UiWindow* window, AABB* left, AABB* top,
                                      AABB* right, AABB* bottom)
{
    const f32 size = 6.0f;
    const f32 half_size = size * 0.5f;
    *left = (AABB){
        .min =
            v2f(window->position.x - half_size, window->position.y - half_size),
        .size = v2f(size, window->size.height + size),
    };
    *right = (AABB){
        .min = v2f(window->position.x + window->size.width - half_size,
                   window->position.y - half_size),
        .size = left->size,
    };
    *top = (AABB){
        .min =
            v2f(window->position.x - half_size, window->position.y - half_size),
        .size = v2f(window->size.width + size, size),
    };
    *bottom = (AABB){
        .min = v2f(window->position.x - half_size,
                   window->position.y + window->size.height - half_size),
        .size = top->size,
    };
}

internal void set_resize_cursor(u8 resize)
{

    if ((check_bit(resize, RESIZE_LEFT) && check_bit(resize, RESIZE_TOP)) ||
        (check_bit(resize, RESIZE_RIGHT) && check_bit(resize, RESIZE_BOTTOM)))
    {
        window_set_cursor(window_get_current(), FTIC_RESIZE_NWSE_CURSOR);
    }
    else if ((check_bit(resize, RESIZE_LEFT) &&
              check_bit(resize, RESIZE_BOTTOM)) ||
             (check_bit(resize, RESIZE_RIGHT) && check_bit(resize, RESIZE_TOP)))
    {
        window_set_cursor(window_get_current(), FTIC_RESIZE_NESW_CURSOR);
    }
    else if (check_bit(resize, RESIZE_LEFT) || check_bit(resize, RESIZE_RIGHT))
    {
        window_set_cursor(window_get_current(), FTIC_RESIZE_H_CURSOR);
    }
    else if (check_bit(resize, RESIZE_TOP) || check_bit(resize, RESIZE_BOTTOM))
    {
        window_set_cursor(window_get_current(), FTIC_RESIZE_V_CURSOR);
    }
}

internal u32 load_font_texture(const f32 pixel_height)
{
    ui_context.font = (FontTTF){ 0 };
    const i32 width_atlas = 256;
    const i32 height_atlas = 256;
    const u32 bitmap_size = width_atlas * height_atlas;
    u8* font_bitmap_temp = (u8*)calloc(bitmap_size, sizeof(u8));
    init_ttf_atlas(width_atlas, height_atlas, pixel_height, 96, 32,
                   ui_context.font_path, font_bitmap_temp, &ui_context.font);

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
    u32 font_texture =
        texture_create(&texture_properties, GL_RGBA8, GL_RGBA, GL_NEAREST);
    free(texture_properties.bytes);

    return font_texture;
}

void ui_context_set_font_path(const char* new_path)
{
    memset(ui_context.font_path, 0, sizeof(ui_context.font_path));
    memcpy(ui_context.font_path, new_path, strlen(new_path));
}

void ui_context_change_font_pixel_height(const f32 pixel_height)
{
    if (closed_interval(12.0f, pixel_height, 26.0f))
    {
        texture_delete(ui_context.render.render.textures.data[1]);
        ui_context.render.render.textures.data[(u32)UI_FONT_TEXTURE] =
            load_font_texture(pixel_height);
    }
}

const char* ui_context_get_font_path()
{
    return ui_context.font_path;
}

void ui_context_create()
{
    array_create(&ui_context.id_to_index, 100);
    array_create(&ui_context.free_indices, 100);

    array_create(&ui_context.last_frame_windows, 10);
    array_create(&ui_context.current_frame_windows, 10);

    array_create(&ui_context.last_frame_top_bar_windows, 10);
    array_create(&ui_context.current_frame_top_bar_windows, 10);

    array_create(&ui_context.last_frame_overlay_windows, 10);
    array_create(&ui_context.current_frame_overlay_windows, 10);

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

    u32 shader =
        shader_create("res/shaders/vertex.glsl", "res/shaders/fragment.glsl");

    u32 frosted_shader = shader_create("res/shaders/vertex.glsl",
                                       "res/shaders/fragment_blur.glsl");

    ftic_assert(shader);
    ftic_assert(frosted_shader);

    char* font_path = "C:/Windows/Fonts/arial.ttf";
    memset(ui_context.font_path, 0, sizeof(ui_context.font_path));
    memcpy(ui_context.font_path, font_path, strlen(font_path));

    u32 font_texture = load_font_texture(16);

    // TODO: make all of these icons into a sprite sheet.
    u32 default_texture = create_default_texture();
    u32 arrow_icon_texture =
        load_icon_as_white_resize("res/icons/arrow_sprite_sheet.png", 72, 24);
    u32 list_icon_texture = load_icon_as_white("res/icons/list.png");
    u32 grid_icon_texture = load_icon_as_white("res/icons/grid.png");

    u32 circle_texture = load_icon("res/Circle.png");

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
    array_create(&textures, 22);
    array_push(&textures, default_texture);
    array_push(&textures, font_texture);
    array_push(&textures, arrow_icon_texture);
    array_push(&textures, list_icon_texture);
    array_push(&textures, grid_icon_texture);

    array_push(&textures, circle_texture);

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

    array_create(&ui_context.render.vertices, 100 * 4);
    u32 vertex_buffer_id = vertex_buffer_create();
    vertex_buffer_orphan(vertex_buffer_id,
                         ui_context.render.vertices.capacity * sizeof(Vertex),
                         GL_STREAM_DRAW, NULL);
    ui_context.render.vertex_buffer_capacity =
        ui_context.render.vertices.capacity;

    array_create(&ui_context.render.indices, 100 * 6);
    generate_indicies(&ui_context.render.indices, 0, 100);
    u32 index_buffer_id = index_buffer_create();
    index_buffer_orphan(index_buffer_id,
                        ui_context.render.indices.size * sizeof(u32),
                        GL_STATIC_DRAW, ui_context.render.indices.data);
    free(ui_context.render.indices.data);

    ui_context.render.render =
        render_create(shader, textures, &vertex_buffer_layout, vertex_buffer_id,
                      index_buffer_id);

    ui_context.default_textures_offset = textures.size;

    {
        ui_context.frosted_blur_amount = 0.00132f;
        ui_context.frosted_samples = 12;

        ui_context.frosted_blur_amount_location =
            glGetUniformLocation(frosted_shader, "blurAmount");
        ftic_assert(ui_context.frosted_blur_amount_location != -1);
        ui_context.frosted_samples_location =
            glGetUniformLocation(frosted_shader, "samples");
        ftic_assert(ui_context.frosted_samples_location != -1);

        U32Array frosted_textures = { 0 };
        array_create(&frosted_textures, 2);

        array_create(&ui_context.frosted_render.vertices, 10 * 4);
        u32 frosted_vertex_buffer_id = vertex_buffer_create();
        vertex_buffer_orphan(frosted_vertex_buffer_id,
                             ui_context.frosted_render.vertices.capacity *
                                 sizeof(Vertex),
                             GL_STREAM_DRAW, NULL);
        ui_context.frosted_render.vertex_buffer_capacity =
            ui_context.frosted_render.vertices.capacity;

        array_create(&ui_context.frosted_render.indices, 10 * 6);
        generate_indicies(&ui_context.frosted_render.indices, 0, 10);
        u32 frosted_index_buffer_id = index_buffer_create();
        index_buffer_orphan(
            frosted_index_buffer_id,
            ui_context.frosted_render.indices.size * sizeof(u32),
            GL_STATIC_DRAW, ui_context.frosted_render.indices.data);
        free(ui_context.frosted_render.indices.data);

        ui_context.frosted_render.render = render_create(
            frosted_shader, frosted_textures, &vertex_buffer_layout,
            frosted_vertex_buffer_id, frosted_index_buffer_id);
    }

    ui_context.docking_color =
        v4f(global_get_secondary_color().r, global_get_secondary_color().g,
            global_get_secondary_color().b, 0.4f);

    particle_buffer_create(&ui_context.particles, 1000);
}

internal u8 look_for_window_resize(UiWindow* window)
{
    AABB left, top, right, bottom;
    get_window_resize_aabbs(window, &left, &top, &right, &bottom);

    const V2 mouse_position = event_get_mouse_position();
    const b8 mouse_button_pressed_once =
        event_is_mouse_button_pressed_once(FTIC_MOUSE_BUTTON_LEFT);

    u8 result = RESIZE_NONE;
    if (collision_point_in_aabb(mouse_position, &left))
    {
        if (mouse_button_pressed_once)
        {
            window->resize_dragging |= RESIZE_LEFT;
        }
        result |= RESIZE_LEFT;
    }
    else if (collision_point_in_aabb(mouse_position, &right))
    {
        if (mouse_button_pressed_once)
        {
            window->resize_dragging |= RESIZE_RIGHT;
        }
        result |= RESIZE_RIGHT;
    }

    if (collision_point_in_aabb(mouse_position, &top))
    {
        if (mouse_button_pressed_once)
        {
            window->resize_dragging |= RESIZE_TOP;
        }
        result |= RESIZE_TOP;
    }
    else if (collision_point_in_aabb(mouse_position, &bottom))
    {
        if (mouse_button_pressed_once)
        {
            window->resize_dragging |= RESIZE_BOTTOM;
        }
        result |= RESIZE_BOTTOM;
    }
    return result;
}

internal b8 check_window_collisions(const u32 window_index, const u32 i)
{
    const b8 mouse_button_clicked =
        event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_LEFT);
    const MouseButtonEvent* mouse_button_event = event_get_mouse_button_event();
    const b8 mouse_button_pressed = mouse_button_event->activated &&
                                    mouse_button_event->action == FTIC_PRESS;
    const V2 mouse_position = event_get_mouse_position();

    UiWindow* window = ui_context.windows.data + window_index;

    if (window->hide) return false;

    if (window->resizeable && !window->docked && !window->resize_dragging)
    {
        u8 any_collision = look_for_window_resize(window);
        if (window->resize_dragging)
        {
            window->resize_pointer_offset = event_get_mouse_position();
            window->resize_size_offset = window->size;
        }
        if (any_collision)
        {
            set_resize_cursor(any_collision);
            return true;
        }
    }

    const AABBArray* aabbs = ui_context.window_aabbs.data + window_index;

    if (aabbs->size)
    {
        AABB back_ground_aabb = {
            .min = window->position,
            .size = window->size,
        };
        if (!collision_point_in_aabb(mouse_position, &back_ground_aabb))
        {
            return false;
        }
        else
        {
            if (!window->docked)
            {
                ui_context.non_docked_window_hover = true;
            }
            window->area_hit = true;
        }
    }

    for (i32 aabb_index = ((i32)aabbs->size) - 1; aabb_index >= 0; --aabb_index)
    {
        const AABB* aabb = aabbs->data + aabb_index;
        if (collision_point_in_aabb(mouse_position, aabb))
        {
            HoverClickedIndex* hover_clicked_index =
                ui_context.window_hover_clicked_indices.data + window_index;
            hover_clicked_index->index = aabb_index;
            hover_clicked_index->hover = true;
            hover_clicked_index->double_clicked =
                mouse_button_event->double_clicked;
            if (mouse_button_clicked || mouse_button_pressed)
            {
                hover_clicked_index->pressed = mouse_button_pressed;
                hover_clicked_index->clicked = mouse_button_clicked;
                if (!window->overlay && !window->docked)
                {
                    push_window_to_back(&ui_context.last_frame_windows, i);
                }
                ui_context.window_in_focus = window_index;
            }
            return true;
        }
    }
    return window->area_hit;
}

f32 ui_context_get_font_pixel_height()
{
    return ui_context.font.pixel_height;
}

const FontTTF* ui_context_get_font()
{
    return &ui_context.font;
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
    ui_context.render.render.textures.size = ui_context.default_textures_offset;

    ui_context.delta_time = clampf64_low(clampf64_high(delta_time, 0.5), 0.0);

    ui_context.mvp.projection =
        ortho(0.0f, dimensions.width, dimensions.height, 0.0f, -1.0f, 1.0f);
    ui_context.mvp.view = m4d();
    ui_context.mvp.model = m4d();

    ui_context.render.vertices.size = 0;
    ui_context.current_index_offset = 0;

    ui_context.row = false;
    ui_context.row_current = 0;
    ui_context.row_padding = 0.0f;

    for (u32 i = 0; i < ui_context.window_hover_clicked_indices.size; ++i)
    {
        ui_context.window_hover_clicked_indices.data[i] =
            (HoverClickedIndex){ .index = -1 };
    }
    ui_context.any_window_top_bar_hold = false;
    ui_context.any_window_hold = false;
    ui_context.non_docked_window_hover = false;
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
    for (u32 i = 0; i < ui_context.last_frame_overlay_windows.size; ++i)
    {
        u32 window_index =
            ui_context.id_to_index
                .data[ui_context.last_frame_overlay_windows.data[i]];
        UiWindow* window = ui_context.windows.data + window_index;
        window->rendering_index_count = 0;
        window->rendering_index_offset = 0;
        window->area_hit = false;
    }

    if (index_window_dragging != -1)
    {
        ui_context.window_in_focus = get_window_index(index_window_dragging);
        push_window_to_back(&ui_context.last_frame_windows,
                            index_window_dragging);
    }
    if (check_collisions && !ui_context.dock_resize_hover &&
        !ui_context.dock_resize && !ui_context.any_window_top_bar_hold &&
        !ui_context.any_window_hold)
    {
        for (i32 i = ((i32)ui_context.last_frame_overlay_windows.size) - 1;
             i >= 0; --i)
        {
            u32 window_index =
                ui_context.id_to_index
                    .data[ui_context.last_frame_overlay_windows.data[i]];
            if (check_window_collisions(window_index, i))
            {
                goto collision_check_done;
            }
        }
        for (i32 i = ((i32)ui_context.last_frame_windows.size) - 1; i >= 0; --i)
        {
            u32 window_index = get_window_index(i);
            if (check_window_collisions(window_index, i))
            {
                goto collision_check_done;
            }
        }
    }
collision_check_done:;
}

internal void check_if_window_should_be_docked()
{
    b8 any_hit = false;
    // TODO: This could cause issues if we have an overlay window, it makes it
    // the window_in_focus. Solution, save the window that is dragging.
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
            window->release_from_dock_space = false;
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
        DockNode* dock_node = focused_window->dock_node;
        for (u32 i = 0; i < dock_node->windows.size; ++i)
        {
            u32 id = dock_node->windows.data[i];
            UiWindow* window_to_undock = ui_window_get(id);
            window_to_undock->docked = true;
            window_to_undock->release_from_dock_space = false;
        }
        push_window_to_first_docked(&ui_context.last_frame_windows,
                                    ui_context.last_frame_windows.size - 1);
    }
    ui_context.dock_side_hit = -1;
}

internal void emit_aabb_particles(const AABB* aabb, const V4 color,
                                  const V2 size_min_max,
                                  const u32 random_count_high)
{
    if (aabb->size.height < 1.0f || aabb->size.width < 1.0f) return;

    const f32 flated_line = aabb->size.width + aabb->size.height;
    const f32 probability_top_or_bottom =
        aabb->size.width / (flated_line + flated_line);
    const f32 probability_left_or_right =
        aabb->size.height / (flated_line + flated_line);

    u32 random_count = random_u32ss(random_seed++, 0, random_count_high);
    for (u32 i = 0; i < random_count; ++i)
    {
        const f32 r = random_f32s(random_seed++, 0.0f, 1.0f);
        u32 random_side = 0; // 0 - 3
        f32 threshold = probability_top_or_bottom;
        random_side += (r >= threshold);
        threshold += probability_top_or_bottom;
        random_side += (r >= threshold);
        threshold += probability_left_or_right;
        random_side += (r >= threshold);

        V2 current = aabb->min;
        switch (random_side)
        {
            case 1:
            {
                current.y += aabb->size.height;
            }
            case 0:
            {
                current.x += random_f32s(random_seed++, 0.0f, aabb->size.width);
                break;
            }
            case 2:
            {
                current.x += aabb->size.width;
            }
            case 3:
            {
                current.y +=
                    random_f32s(random_seed++, 0.0f, aabb->size.height);
                break;
            }
            default: break;
        }

        Particle* particle = particle_buffer_get_next(&ui_context.particles);
        particle->position = current;
        particle->velocity = v2f(random_f32s(random_seed, -20.0f, 20.0f),
                                 random_f32s(random_seed + 1, -20.0f, 20.0f));
        particle->acceleration =
            v2f(particle->velocity.x, particle->velocity.y);
        particle->dimension = v2i(
            random_f32s(random_seed + 2, size_min_max.min, size_min_max.max));
        particle->life = v2i(random_f32s(random_seed + 3, 0.6f, 0.8f));
        particle->color = color;
        particle->alpha_change = false;
        particle->size_change = true;
        particle_buffer_set_next(&ui_context.particles, particle);

        random_seed += 4;
    }
}

internal void check_dock_space_resize()
{
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
                if (ui_context.dock_hit_node->split_axis == SPLIT_HORIZONTAL)
                {
                    window_set_cursor(ftic_window, FTIC_RESIZE_V_CURSOR);
                }
                else
                {
                    window_set_cursor(ftic_window, FTIC_RESIZE_H_CURSOR);
                }
                const V4 color = v4_s_multi(global_get_secondary_color(), 0.8f);

                emit_aabb_particles(&ui_context.dock_resize_aabb, color,
                                    v2f(2.0f, 4.0f), 10);
                quad(&ui_context.render.vertices,
                     ui_context.dock_resize_aabb.min,
                     ui_context.dock_resize_aabb.size, color, 0.0f);
                ui_context.extra_index_count += 6;
            }
            else
            {
                window_set_cursor(window_get_current(), FTIC_NORMAL_CURSOR);
            }
        }

        if (ui_context.dock_resize)
        {
#if 0
            emit_aabb_particles(&ui_context.dock_resize_aabb,
                                global_get_secondary_color(), v2f(2.0f, 4.0f),
                                15);
#endif
            quad(&ui_context.render.vertices, ui_context.dock_resize_aabb.min,
                 ui_context.dock_resize_aabb.size, global_get_secondary_color(),
                 0.0f);
            ui_context.extra_index_count += 6;
        }
    }
}

internal void check_and_display_mouse_drag_box()
{
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
}

internal void sync_current_frame_windows(U32Array* current_frame,
                                         U32Array* last_frame)
{
    // TODO: can be very expensive. Consider a more efficient way.
    for (u32 i = 0; i < last_frame->size; ++i)
    {
        u32 window = last_frame->data[i];
        b8 exist = false;
        for (u32 j = 0; j < current_frame->size; ++j)
        {
            if (current_frame->data[j] == window)
            {
                push_window_to_back(current_frame, j--);
                current_frame->size--;
                exist = true;
                break;
            }
        }

        if (!exist)
        {
            push_window_to_back(last_frame, i--);
            last_frame->size--;
        }
    }

    for (u32 i = 0; i < current_frame->size; ++i)
    {
        array_push(last_frame, current_frame->data[i]);
    }
    current_frame->size = 0;
}

internal DockNodePtrArray get_active_dock_spaces()
{
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
    return dock_spaces;
}

internal u32 display_text_and_truncate_if_necissary(const V2 position,
                                                    const f32 total_width,
                                                    const f32 alpha, char* text)
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
    u32 index_count = text_generation_color(
        ui_context.font.chars, text, UI_FONT_TEXTURE, position, 1.0f,
        ui_context.font.pixel_height, v4a(global_get_text_color(), alpha), NULL,
        NULL, NULL, &ui_context.render.vertices);
    if (too_long)
    {
        memcpy(text + (i - 3), saved_name, sizeof(saved_name));
    }
    return index_count;
}

typedef struct TabChange
{
    DockNode* dock_space;
    UiWindow* window_to_hide;
    UiWindow* window_to_show;
    u32 window_focus_index;
    b8 close_tab;
} TabChange;

internal TabChange update_tabs(DockNodePtrArray* dock_spaces,
                               UU32Array* dock_spaces_index_offsets_and_counts)
{
    TabChange tab_change = { 0 };
    const V2 mouse_position = event_get_mouse_position();
    const b8 should_check_collision =
        ui_context.check_collisions && !ui_context.dock_resize_hover &&
        !ui_context.dock_resize && !ui_context.any_window_top_bar_hold &&
        !ui_context.any_window_hold;

    const f32 top_bar_height = ui_context.font.pixel_height + 6.0f;

    b8 any_tab_hit = false;
    b8 any_hit = false;
    for (i32 i = ((i32)dock_spaces->size) - 1; i >= 0; --i)
    {
        DockNode* dock_space = dock_spaces->data[i];
        UiWindow* window = ui_window_get(
            dock_space->windows.data[dock_space->window_in_focus]);
        UU32 index_offset_and_count = {
            .first = ui_context.current_index_offset,
        };
        const b8 in_focus = ui_context.id_to_index.data[window->id] ==
                                ui_context.window_in_focus &&
                            !ui_context.highlight_fucused_window_off;
        V4 window_border_color = global_get_border_color();

        const f32 min_tab_width = window->size.width / dock_space->windows.size;
        if (in_focus)
        {
            window_border_color = global_get_secondary_color();
#if 0
            AABB aabb = {
                .min = window->position,
                .size = window->size,
            };
            emit_aabb_particles(&aabb, window_border_color, v2f(1.0f, 2.5f), 4);
#endif
        }

        if (window->top_bar)
        {
            const V2 top_bar_dimensions =
                v2f(window->size.width, top_bar_height);
            const V4 top_bar_color =
                v4a(v4_s_multi(global_get_clear_color(), 1.2f), 1.0f);
            const AABB aabb =
                quad_gradiant_t_b(&ui_context.render.vertices, window->position,
                                  top_bar_dimensions, top_bar_color,
                                  global_get_clear_color(), 0.0f);
            index_offset_and_count.second += 6;

            V2 tab_position = window->position;
            for (i32 j = 0; j < (i32)dock_space->windows.size; ++j)
            {
                UiWindow* tab_window =
                    ui_window_get(dock_space->windows.data[j]);

                const V2 button_size = v2i(top_bar_dimensions.height - 4.0f);

                const f32 tab_padding = 8.0f;
                V2 tab_dimensions = v2f(100.0f + (tab_padding * 2.0f) +
                                            button_size.width + 8.0f,
                                        top_bar_dimensions.height);

                tab_dimensions.width = min(tab_dimensions.width, min_tab_width);

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
                if (should_check_collision && window->area_hit &&
                    !tab_change.close_tab && !any_tab_hit &&
                    collision_point_in_aabb(mouse_position, &tab_aabb))
                {
                    if (j != (i32)dock_space->window_in_focus)
                    {
                        if (event_is_mouse_button_clicked(
                                FTIC_MOUSE_BUTTON_LEFT))
                        {
                            tab_change.dock_space = dock_space;
                            tab_change.window_focus_index = j;
                            tab_change.window_to_hide = window;
                            tab_change.window_to_show = tab_window;
                        }
                        tab_color = v4ic(0.3f);
                    }

                    if (event_is_mouse_button_pressed_once(
                            FTIC_MOUSE_BUTTON_LEFT))
                    {
                        tab_window->release_from_dock_space = true;
                        tab_window->top_bar_offset = event_get_mouse_position();
                        tab_window->top_bar_pressed = true;
                    }

                    any_hit = true;

                    tab_collided = true;
                }

                quad(&ui_context.render.vertices, tab_position, tab_dimensions,
                     tab_color, 0.0f);
                index_offset_and_count.second += 6;

                V2 button_position = v2f(tab_position.x + tab_dimensions.width -
                                             button_size.width - 5.0f,
                                         tab_position.y + 2.0f);

                AABB button_aabb = {
                    .min = button_position,
                    .size = button_size,
                };

                V2 text_position =
                    v2f(tab_position.x + tab_padding,
                        tab_position.y + ui_context.font.pixel_height);
                f32 title_advance = 0.0f;
                if (tab_window->title.size)
                {
                    index_offset_and_count.second +=
                        display_text_and_truncate_if_necissary(
                            text_position, button_position.x - text_position.x,
                            tab_window->alpha, tab_window->title.data);
                }

                b8 collided =
                    collision_point_in_aabb(mouse_position, &button_aabb);

                if (should_check_collision && collided && window->area_hit)
                {
                    if (event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_LEFT))
                    {
                        tab_change.close_tab = true;
                        tab_change.dock_space = dock_space;
                        tab_change.window_focus_index = j;
                        tab_change.window_to_hide = tab_window;
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

                tab_position.x += tab_dimensions.width;
            }

            quad_border(&ui_context.render.vertices,
                        &index_offset_and_count.second, window->position,
                        top_bar_dimensions, global_get_border_color(), 1.0f,
                        0.0f);

            if (should_check_collision && window->area_hit &&
                !tab_change.close_tab && !any_tab_hit && !any_hit &&
                collision_point_in_aabb(mouse_position, &aabb) &&
                event_is_mouse_button_pressed_once(FTIC_MOUSE_BUTTON_LEFT))
            {
                window->top_bar_offset = event_get_mouse_position();
                window->top_bar_pressed = true;
                window->any_holding = true;
                any_hit = true;
            }
        }
        quad_border(&ui_context.render.vertices, &index_offset_and_count.second,
                    window->position, window->size, window_border_color, 1.0f,
                    0.0f);
        dock_spaces_index_offsets_and_counts->data[i] = index_offset_and_count;
        ui_context.current_index_offset =
            index_offset_and_count.first + index_offset_and_count.second;
    }
    return tab_change;
}

internal void add_frosted_background(V2 position, const V2 size,
                                     const u32 frosted_texture_index)
{
    V2 saved_position = position;
    position.y = ui_context.dimensions.y - (position.y + size.height);

    TextureCoordinates texture_coordinates;
    texture_coordinates.coordinates[0] =
        v2f(position.x / ui_context.dimensions.width,
            (position.y + size.height) / ui_context.dimensions.height);
    texture_coordinates.coordinates[1] =
        v2f(position.x / ui_context.dimensions.width,
            position.y / ui_context.dimensions.height);
    texture_coordinates.coordinates[2] =
        v2f((position.x + size.width) / ui_context.dimensions.width,
            position.y / ui_context.dimensions.height);
    texture_coordinates.coordinates[3] =
        v2f((position.x + size.width) / ui_context.dimensions.width,
            (position.y + size.height) / ui_context.dimensions.height);

    set_up_verticies(&ui_context.frosted_render.vertices, saved_position, size,
                     v4ic(1.0f), (f32)frosted_texture_index,
                     texture_coordinates);
}

internal void render_ui(const DockNodePtrArray* dock_spaces,
                        const UU32Array* dock_spaces_index_offsets_and_counts)
{
    const f32 top_bar_height = ui_context.font.pixel_height + 6.0f;
    AABB whole_screen_scissor = { .size = ui_context.dimensions };

    render_begin_draw(&ui_context.render.render,
                      ui_context.render.render.shader_properties.shader,
                      &ui_context.mvp);
    for (u32 i = 0; i < dock_spaces->size; ++i)
    {
        DockNode* node = dock_spaces->data[i];
        UiWindow* window =
            ui_window_get(node->windows.data[node->window_in_focus]);

        AABB scissor = { 0 };
        if (ui_context.dimensions.y)
        {
            scissor.min.x = window->position.x;
            scissor.min.y = ui_context.dimensions.y -
                            (window->position.y + window->size.height);
            scissor.size = window->size;
            scissor.size.height -= (window->top_bar * top_bar_height);
            scissor.size.height = clampf32_low(scissor.size.height, 0.0f);
        }
        render_draw(window->rendering_index_offset,
                    window->rendering_index_count, &scissor);

        scissor.size = window->size;
        UU32 index_offset_and_count =
            dock_spaces_index_offsets_and_counts->data[i];
        render_draw(index_offset_and_count.first, index_offset_and_count.second,
                    &scissor);
    }
    render_draw(ui_context.extra_index_offset, ui_context.extra_index_count,
                &whole_screen_scissor);

    render_draw(ui_context.particles_index_offset,
                ui_context.particles.size * 6, &whole_screen_scissor);

    render_end_draw(&ui_context.render.render);
}

internal void render_overlay_ui(const u32 index_offset, const u32 index_count)
{
    AABB whole_screen_scissor = { .size = ui_context.dimensions };
    render_begin_draw(&ui_context.render.render,
                      ui_context.render.render.shader_properties.shader,
                      &ui_context.mvp);
    for (u32 i = 0, j = 0; i < ui_context.last_frame_overlay_windows.size; ++i)
    {
        UiWindow* window =
            ui_window_get(ui_context.last_frame_overlay_windows.data[i]);

        AABB scissor = { 0 };
        if (ui_context.dimensions.y)
        {
            scissor.min.x = window->position.x;
            scissor.min.y = ui_context.dimensions.y -
                            (window->position.y + window->size.height);
            scissor.size = window->size;
        }

        if (window->frosted_background)
        {
            const u32 shader =
                ui_context.frosted_render.render.shader_properties.shader;
            render_begin_draw(&ui_context.frosted_render.render, shader,
                              &ui_context.mvp);
            render_draw(j++ * 6, 6, &whole_screen_scissor);
            render_end_draw(&ui_context.frosted_render.render);

            render_begin_draw(&ui_context.render.render,
                              ui_context.render.render.shader_properties.shader,
                              &ui_context.mvp);
        }

        render_draw(window->rendering_index_offset,
                    window->rendering_index_count, &scissor);

        render_draw(index_offset + (index_count * i), index_count, &scissor);
    }
    render_end_draw(&ui_context.render.render);
}

internal void handle_tab_change_or_close(TabChange tab_change)
{
    u32 focused_window_id = 0;
    b8 change_window_to_focus = false;
    if (tab_change.close_tab)
    {
        ui_window_close(tab_change.window_to_hide->id);
    }
    else
    {
        tab_change.dock_space->window_in_focus = tab_change.window_focus_index;
        tab_change.window_to_hide->hide = true;
        tab_change.window_to_show->position =
            tab_change.window_to_hide->position;
        tab_change.window_to_show->size = tab_change.window_to_hide->size;
        tab_change.window_to_show->hide = false;
        focused_window_id = tab_change.window_to_show->id;
    }
    tab_change.window_to_hide->release_from_dock_space = false;
    ui_context.window_in_focus = ui_context.id_to_index.data[focused_window_id];
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
        check_if_window_should_be_docked();
        if (!ui_context.non_docked_window_hover)
        {
            check_dock_space_resize();
        }
    }

#endif
    check_and_display_mouse_drag_box();

    ui_context.current_index_offset =
        ui_context.extra_index_offset + ui_context.extra_index_count;

    sync_current_frame_windows(&ui_context.current_frame_windows,
                               &ui_context.last_frame_windows);

    sync_current_frame_windows(&ui_context.current_frame_overlay_windows,
                               &ui_context.last_frame_overlay_windows);

    DockNodePtrArray dock_spaces = get_active_dock_spaces();

    UU32Array dock_spaces_index_offsets_and_counts = { 0 };
    array_create(&dock_spaces_index_offsets_and_counts, dock_spaces.size);
    TabChange tab_change =
        update_tabs(&dock_spaces, &dock_spaces_index_offsets_and_counts);

    ui_context.particles_index_offset = ui_context.current_index_offset;
    particle_buffer_update(&ui_context.particles, ui_context.delta_time);
    for (u32 i = 0; i < ui_context.particles.size; ++i)
    {
        Particle* particle = ui_context.particles.data + i;
        quad(&ui_context.render.vertices, particle->position,
             particle->dimension, particle->color, UI_CIRCLE_TEXTURE);
    }
    ui_context.current_index_offset += ui_context.particles.size * 6;

    u32 overlay_index_offset = ui_context.current_index_offset;
    u32 overlay_index_count = 0;
    for (u32 i = 0; i < ui_context.last_frame_overlay_windows.size; ++i)
    {
        UiWindow* window =
            ui_window_get(ui_context.last_frame_overlay_windows.data[i]);

        // const f32 roundness = 15.0f / max(window->size.height, 1.0f);

        overlay_index_count = 0;
        quad_border(&ui_context.render.vertices, &overlay_index_count,
                    window->position, window->size,
                    global_get_secondary_color(), 1.0f, 0.0f);
    }

    ui_context.current_index_offset +=
        overlay_index_count * ui_context.last_frame_overlay_windows.size;

    rendering_properties_check_and_grow_vertex_buffer(&ui_context.render);
    rendering_properties_check_and_grow_index_buffer(
        &ui_context.render, ui_context.current_index_offset);

    buffer_set_sub_data(ui_context.render.render.vertex_buffer_id,
                        GL_ARRAY_BUFFER, 0,
                        sizeof(Vertex) * ui_context.render.vertices.size,
                        ui_context.render.vertices.data);

    ui_context.frosted_render.vertices.size = 0;
    ui_context.frosted_render.render.textures.size = 0;
    u32 fbo = 0;
    u32 fbo_texture = 0;
    if (ui_context.last_frame_overlay_windows.size)
    {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        glGenTextures(1, &fbo_texture);
        glBindTexture(GL_TEXTURE_2D, fbo_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                     (GLsizei)ui_context.dimensions.width,
                     (GLsizei)ui_context.dimensions.height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, fbo_texture, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
        {
            render_ui(&dock_spaces, &dock_spaces_index_offsets_and_counts);
        }
        else
        {
            texture_delete(fbo_texture);
            fbo_texture = 0;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (fbo_texture)
        {
            ui_context.frosted_render.render.textures.size = 1;
            ui_context.frosted_render.render.textures.data[0] = fbo_texture;
            for (u32 i = 0; i < ui_context.last_frame_overlay_windows.size; ++i)
            {
                UiWindow* window = ui_window_get(
                    ui_context.last_frame_overlay_windows.data[i]);
                if (window->frosted_background)
                {
                    add_frosted_background(window->position, window->size, 0);
                }
            }
            buffer_set_sub_data(
                ui_context.frosted_render.render.vertex_buffer_id,
                GL_ARRAY_BUFFER, 0,
                sizeof(Vertex) * ui_context.frosted_render.vertices.size,
                ui_context.frosted_render.vertices.data);
        }
    }
    render_ui(&dock_spaces, &dock_spaces_index_offsets_and_counts);

    AABB whole_screen_scissor = { .size = ui_context.dimensions };

    if (ui_context.last_frame_overlay_windows.size)
    {
#if 0
        if (event_is_key_pressed_once(FTIC_KEY_I))
        {
            ui_context.frosted_blur_amount += 0.0001f;
            log_f32("BlurAmount: ", ui_context.frosted_blur_amount);
        }
        if (event_is_key_pressed_once(FTIC_KEY_U))
        {
            ui_context.frosted_blur_amount -= 0.0001f;
            log_f32("BlurAmount: ", ui_context.frosted_blur_amount);
        }
        if (event_is_key_pressed_once(FTIC_KEY_J))
        {
            ui_context.frosted_samples += 1;
            log_u64("Samples: ", (u64)ui_context.frosted_samples);
        }
        if (event_is_key_pressed_once(FTIC_KEY_K))
        {
            ui_context.frosted_samples -= 1;
            log_u64("Samples: ", (u64)ui_context.frosted_samples);
        }
#endif
        shader_bind(ui_context.frosted_render.render.shader_properties.shader);
        glUniform1f(ui_context.frosted_blur_amount_location,
                    ui_context.frosted_blur_amount);
        glUniform1i(ui_context.frosted_samples_location,
                    ui_context.frosted_samples);
        shader_unbind();
        render_overlay_ui(overlay_index_offset, overlay_index_count);
    }

    if (ui_context.last_frame_overlay_windows.size)
    {
        if (fbo_texture)
        {
            texture_delete(fbo_texture);
        }
        glDeleteFramebuffers(1, &fbo);
    }

    if (!ui_context.any_window_hold && ui_context.dock_resize)
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
        if (closed_interval(0.1f, new_ratio, 0.9f))
        {
            ui_context.dock_hit_node->size_ratio = new_ratio;
            dock_node_resize_traverse(ui_context.dock_hit_node);
        }
        ui_context.dock_resize_aabb =
            dock_node_set_resize_aabb(ui_context.dock_hit_node);
    }

    if (tab_change.dock_space)
    {
        handle_tab_change_or_close(tab_change);
    }
    array_free(&dock_spaces);
    array_free(&dock_spaces_index_offsets_and_counts);

    ui_context.current_frame_top_bar_windows.size = 0;
    ui_context.current_frame_top_bar_windows.size = 0;
}

void ui_context_destroy()
{
    save_layout();
}

void ui_context_set_animation(b8 on)
{
    ui_context.animation_off = !on;
}

void ui_context_set_highlight_focused_window(b8 on)
{
    ui_context.highlight_fucused_window_off = !on;
}

u32 ui_window_create()
{
    u32 id = generate_id(ui_context.windows.size, &ui_context.free_indices,
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

UiWindow* ui_window_get(const u32 window_id)
{
    return ui_context.windows.data + ui_context.id_to_index.data[window_id];
}

void ui_window_set_end_scroll_offset(const u32 window_id, const f32 offset)
{
    ui_window_get(window_id)->end_scroll_offset = offset;
}

void ui_window_set_current_scroll_offset(const u32 window_id, const f32 offset)
{
    ui_window_get(window_id)->current_scroll_offset = offset;
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
        window->size_animation_precent += 1.0f * ui_context.animation_off;
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
        window->position_animation_precent += 1.0f * ui_context.animation_off;
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

internal V2 directory_item_animate_position(DirectoryItem* item)
{
    const f32 animation_speed = 5.0f;
    V2 position = v2d();
    item->animation_precent += 1.0f * ui_context.animation_off;
    if (item->animation_precent >= 1.0f)
    {
        position = item->after_animation;
    }
    else
    {
        position = v2_lerp(item->before_animation, item->after_animation,
                           ease_out_cubic(item->animation_precent));

        item->animation_precent +=
            clampf32_low((f32)ui_context.delta_time, 0.0f) * animation_speed;
    }
    return position;
}

internal void handle_window_top_bar_events(UiWindow* window)
{
    const f32 top_bar_height = ui_context.font.pixel_height + 6.0f;
    if (window->top_bar_hold)
    {
        window->position =
            v2_add(event_get_mouse_position(), window->top_bar_offset);

        window->position.y =
            clampf32_high(window->position.y,
                          ui_context.dimensions.height - window->size.height);
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
        if (v2_distance(window->top_bar_offset, event_get_mouse_position()) >=
            10.0f)
        {
            const V2 mouse_position = event_get_mouse_position();
            if (window->release_from_dock_space)
            {
                DockNode* dock_node = window->dock_node;
                if (dock_node->windows.size > 1)
                {
                    u32 i = 0;
                    for (; i < dock_node->windows.size; ++i)
                    {
                        if (window->id == dock_node->windows.data[i]) break;
                    }
                    remove_window_from_shared_dock_space(i, window, dock_node);
                    window->closing =
                        false; // Function above sets closing = true

                    window->position.y =
                        mouse_position.y - (top_bar_height * 0.5f);
                    ui_window_start_size_animation(window, window->size,
                                                   v2i(200.0f));
                }
                window->release_from_dock_space = false;
            }

            if (window->docked)
            {
                DockNode* dock_node = window->dock_node;
                for (u32 i = 0; i < dock_node->windows.size; ++i)
                {
                    u32 id = dock_node->windows.data[i];
                    UiWindow* window_to_undock = ui_window_get(id);
                    window_to_undock->docked = false;
                    for (u32 j = 0; j < ui_context.last_frame_windows.size; ++j)
                    {
                        if (ui_context.last_frame_windows.data[j] ==
                            window_to_undock->id)
                        {
                            push_window_to_back(&ui_context.last_frame_windows,
                                                j);
                            break;
                        }
                    }
                }

                dock_node_remove_node(ui_context.dock_tree, window->dock_node);

                window->position.y = mouse_position.y - (top_bar_height * 0.5f);
                ui_window_start_size_animation(window, window->size,
                                               v2i(200.0f));
            }
            window->top_bar_hold = true;
            window->top_bar_offset = v2_sub(window->position, mouse_position);
            window->any_holding = true;
        }
    }
}

internal void resize_where_position_change(const f32 mouse_position,
                                           const f32 size_offset,
                                           const f32 position_offset,
                                           f32* position, f32* size)
{
    *size = size_offset + position_offset;
    if (*size > 200.0f)
    {
        *position = mouse_position;
    }
    else
    {
        *position = mouse_position - (200.0f - (*size));
        *size = 200.0f;
    }
}

internal void resize_where_only_size_change(const f32 size_offset,
                                            const f32 position_offset,
                                            f32* size)
{
    *size = size_offset - position_offset;
    if (*size < 200.0f)
    {
        *size = 200.0f;
    }
}

internal void handle_window_resize_events(UiWindow* window)
{
    const V2 mouse_position = event_get_mouse_position();
    const V2 offset = v2_sub(window->resize_pointer_offset, mouse_position);
    if (check_bit(window->resize_dragging, RESIZE_LEFT))
    {
        resize_where_position_change(mouse_position.x,
                                     window->resize_size_offset.width, offset.x,
                                     &window->position.x, &window->size.width);
    }
    else if (check_bit(window->resize_dragging, RESIZE_RIGHT))
    {
        resize_where_only_size_change(window->resize_size_offset.width,
                                      offset.x, &window->size.width);
    }

    if (check_bit(window->resize_dragging, RESIZE_TOP))
    {
        resize_where_position_change(
            mouse_position.y, window->resize_size_offset.height, offset.y,
            &window->position.y, &window->size.height);
    }
    else if (check_bit(window->resize_dragging, RESIZE_BOTTOM))
    {
        resize_where_only_size_change(window->resize_size_offset.height,
                                      offset.y, &window->size.height);
    }
    set_resize_cursor(window->resize_dragging);

    window->size_animation_on = false;
    window->position_animation_on = false;

    window->any_holding = true;
    window->top_bar_hold = false;
    window->top_bar_pressed = false;
    window->top_bar_offset = mouse_position;
}

b8 ui_window_begin(u32 window_id, const char* title, u32 flags)
{
    ui_context.current_window_id = window_id;

    const u32 window_index = ui_context.id_to_index.data[window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;
    aabbs->size = 0;

    window->resizeable = check_bit(flags, UI_WINDOW_RESIZEABLE);
    window->top_bar = check_bit(flags, UI_WINDOW_TOP_BAR);
    window->frosted_background = check_bit(flags, UI_WINDOW_FROSTED_GLASS);
    window->overlay = check_bit(flags, UI_WINDOW_OVERLAY);

    if (event_get_mouse_button_event()->action == FTIC_RELEASE)
    {
        window->top_bar_pressed = false;
        window->top_bar_hold = false;
        window->any_holding = false;
        window->resize_dragging = RESIZE_NONE;
        window->scroll_bar.dragging = false;
        window->scroll_bar_width.dragging = false;
        window->release_from_dock_space = false;
    }

    if (!window->closing || window->size_animation_on ||
        window->position_animation_on)
    {
        if (window->overlay)
        {
            array_push(&ui_context.current_frame_overlay_windows, window->id);
        }
        else
        {
            array_push(&ui_context.current_frame_windows, window->id);
        }
        if (window->top_bar)
        {
            array_push(&ui_context.current_frame_top_bar_windows, window->id);
        }
    }

    if (title)
    {
        const u32 title_length = (u32)strlen(title);
        if (!window->title.data)
        {
            array_create(&window->title, (u32)strlen(title));
        }
        window->title.size = 0;
        for (u32 i = 0; i < title_length; ++i)
        {
            array_push(&window->title, title[i]);
        }
        array_push(&window->title, '\0');
        array_push(&window->title, '\0');
        array_push(&window->title, '\0');
    }
    else
    {
        window->title.size = 0;
    }
    window->rendering_index_offset = ui_context.current_index_offset;
    window->rendering_index_count = 0;

    if (window->resize_dragging)
    {
        handle_window_resize_events(window);
    }
    else if (window->top_bar && !ui_context.mouse_box_is_dragging)
    {
        handle_window_top_bar_events(window);
    }
    window_animate(window);

    const f32 top_bar_height = ui_context.font.pixel_height + 6.0f;
    window->first_item_position = window->position;
    window->first_item_position.y += top_bar_height * window->top_bar;

    window->total_height = 0.0f;
    window->total_width = 0.0f;
    window->top_color =
        v4a(global_get_clear_color(), window->back_ground_alpha);
    window->bottom_color =
        v4a(global_get_clear_color(), window->back_ground_alpha);

    if (window->hide) return false;

    if (window->frosted_background)
    {
        window->top_color.a = 0.8f;
        window->bottom_color.a = 0.8f;
    }
    array_push(aabbs,
               quad_gradiant_t_b(&ui_context.render.vertices, window->position,
                                 window->size, window->top_color,
                                 window->bottom_color, 0.0f));
    window->rendering_index_count += 6;

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
             v2f(scroll_bar_width, area_height), global_get_highlight_color(),
             0.0f);
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
                 global_get_bright_color(), 0.0f);
            window->rendering_index_count += 6;
        }
        else
        {
            quad_gradiant_t_b(&ui_context.render.vertices, position,
                              scroll_bar_dimensions, global_get_lighter_color(),
                              v4ic(0.45f), 0.0f);
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
             v2f(area_width, scroll_bar_height), global_get_highlight_color(),
             0.0f);
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
                 global_get_bright_color(), 0.0f);
            window->rendering_index_count += 6;
        }
        else
        {
            quad_gradiant_t_b(&ui_context.render.vertices, position,
                              scroll_bar_dimensions, global_get_lighter_color(),
                              v4ic(0.45f), 0.0f);
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
                              12.0f, window->current_scroll_offset);
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
    if (window->overlay)
    {
        return !window->area_hit &&
               (event_is_mouse_button_pressed_once(FTIC_MOUSE_BUTTON_LEFT) ||
                event_is_mouse_button_pressed_once(FTIC_MOUSE_BUTTON_RIGHT));
    }
    return false;
}

void ui_window_row_begin(const f32 padding)
{
    ui_context.row = true;
    ui_context.row_current = 0;
    ui_context.row_padding = padding;
}

f32 ui_window_row_end()
{
    const f32 result = ui_context.row_current;
    ui_context.row = false;
    ui_context.row_current = 0;
    ui_context.row_padding = 0.0f;
    return result;
}

void ui_window_column_begin(const f32 padding)
{
    ui_context.column = true;
    ui_context.column_current = 0;
    ui_context.column_padding = padding;
}

f32 ui_window_column_end()
{
    const f32 result = ui_context.column_current;
    ui_context.column = false;
    ui_context.column_current = 0;
    ui_context.column_padding = 0.0f;
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

    if (ui_context.column || ui_context.row)
    {
        if (ui_context.row)
        {
            position.x += ui_context.row_current;
            ui_context.row_current += size.width + ui_context.row_padding;
        }
        else
        {
            position.y += ui_context.column_current;
            ui_context.column_current +=
                size.height + ui_context.column_padding;
        }
    }

    V4 button_color = v4ic(1.0f);
    b8 hover = false;
    b8 clicked = false;
    if (disable)
    {
        button_color = global_get_border_color();
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
                v2f(1.0f, ui_context.font.pixel_height), v4i(1.0f), 0.0f);
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

    if (ui_context.column || ui_context.row)
    {
        if (ui_context.row)
        {
            position.x += ui_context.row_current;
            ui_context.row_current += size.width + ui_context.row_padding;
        }
        else
        {
            position.y += ui_context.column_current;
            ui_context.column_current +=
                size.height + ui_context.column_padding;
        }
    }

    if (event_is_mouse_button_pressed_once(FTIC_MOUSE_BUTTON_LEFT) ||
        event_is_mouse_button_pressed_once(FTIC_MOUSE_BUTTON_RIGHT))
    {
        input->active = hover_clicked_index.index == (i32)aabbs->size;
    }
    array_push(aabbs, quad(&ui_context.render.vertices, position, size,
                           global_get_clear_color(), 0.0f));
    window->rendering_index_count += 6;

    b8 typed = false;
    if (input->active)
    {
        if (event_is_key_pressed_once(FTIC_KEY_ESCAPE))
        {
            input->active = false;
        }
        else
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
                        input->buffer.data[input->input_index++] =
                            clip_board[i];
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
    }

    V2 text_position =
        v2f(position.x + 5.0f,
            position.y + (middle(size.y, ui_context.font.pixel_height) * 0.8f));
    window->rendering_index_count +=
        render_input(ui_context.delta_time, text_position, input);

    quad_border_rounded(&ui_context.render.vertices,
                        &window->rendering_index_count, position, size,
                        global_get_border_color(), 1.0f, 0.4f, 3, 0.0f);

    return typed;
}

internal b8 item_in_view(const f32 position_y, const f32 height,
                         const f32 window_position_y, const f32 window_height)
{
    const f32 value = position_y + height;
    return closed_interval(window_position_y, value,
                           window_position_y + window_height + height);
}

internal b8 check_directory_item_collision(V2 starting_position,
                                           V2 item_dimensions,
                                           DirectoryItem* item, b8* selected,
                                           List* list)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;
    HoverClickedIndex hover_clicked_index =
        ui_context.window_hover_clicked_indices.data[window_index];

    char** check_if_selected = NULL;
    if (list)
    {
        check_if_selected = hash_table_get_guid(
            &list->selected_item_values.selected_items, item->id);

        *selected = check_if_selected ? true : false;
    }

    const b8 alt_pressed = event_get_key_event()->alt_pressed;

    AABB aabb = { .min = starting_position, .size = item_dimensions };
    const b8 hit = (hover_clicked_index.index == (i32)aabbs->size) ||
                   (collision_aabb_in_aabb(&ui_context.mouse_drag_box, &aabb) &&
                    alt_pressed);

    const b8 mouse_button_clicked_right =
        event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_RIGHT);
    const b8 mouse_button_clicked =
        event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_LEFT) ||
        mouse_button_clicked_right;

    b8 clicked_on_same = false;
    if (mouse_button_clicked && list)
    {
        if (hit)
        {
            const u32 path_length = (u32)strlen(item->path);
            const u32 name_length = (u32)strlen(item->name);

            b8 removed_item = false;
            b8 ctrl_pressed = event_get_key_event()->ctrl_pressed;
            if (!check_if_selected)
            {
                if (!ctrl_pressed && !alt_pressed)
                {
                    directory_clear_selected_items(&list->selected_item_values);
                }
                char* path = string_copy(item->path, path_length, 2);
                hash_table_insert_guid(
                    &list->selected_item_values.selected_items, item->id, path);
                array_push(&list->selected_item_values.paths, path);
            }
            else if (!ctrl_pressed)
            {
                if (!mouse_button_clicked_right)
                {
                    directory_clear_selected_items(&list->selected_item_values);
                    char* path = string_copy(item->path, path_length, 2);
                    hash_table_insert_guid(
                        &list->selected_item_values.selected_items, item->id,
                        path);
                    array_push(&list->selected_item_values.paths, path);
                }
            }
            else
            {
                directory_remove_selected_item(&list->selected_item_values,
                                               item->id);
                removed_item = true;
            }

            if (list->selected_item_values.last_selected)
            {
                if (strcmp(list->selected_item_values.last_selected,
                           item->name) == 0)
                {
                    if (removed_item)
                    {
                        free(list->selected_item_values.last_selected);
                        list->selected_item_values.last_selected = NULL;
                        item->rename = false;
                    }
                    else if (!mouse_button_clicked_right)
                    {
                        list->input_pressed = window_get_time();
                        item->rename = true;
                        clicked_on_same = true;
                    }
                }
                else
                {
                    free(list->selected_item_values.last_selected);
                    list->selected_item_values.last_selected =
                        string_copy(item->name, name_length, 0);
                    item->rename = false;
                }
            }
            else
            {
                list->selected_item_values.last_selected =
                    string_copy(item->name, name_length, 0);
                item->rename = false;
            }

            list->item_selected = true;
            *selected = true;
        }
        else if (!list->input.active)
        {
            item->rename = false;
        }
    }

    if (item->rename && list && !hover_clicked_index.double_clicked)
    {
        if (!list->input.active)
        {
            if (window_get_time() - list->input_pressed >= 1.0f)
            {
                list->input.active = true;
                list->input.buffer.size = 0;
                const u32 name_length = (u32)strlen(item->name);
                for (u32 i = 0; i < name_length; ++i)
                {
                    array_push(&list->input.buffer, item->name[i]);
                }
                array_push(&list->input.buffer, '\0');
            }
        }
    }
    else
    {
        item->rename = false;
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
            if (strcmp(extension, "png") == 0)
            {
                return small ? UI_FILE_PNG_ICON_TEXTURE
                             : UI_FILE_PNG_ICON_BIG_TEXTURE;
            }
            else if (strcmp(extension, "jpg") == 0)
            {
                return small ? UI_FILE_JPG_ICON_TEXTURE
                             : UI_FILE_JPG_ICON_BIG_TEXTURE;
            }
            else if (strcmp(extension, "pdf") == 0)
            {
                return small ? UI_FILE_PDF_ICON_TEXTURE
                             : UI_FILE_PDF_ICON_BIG_TEXTURE;
            }
            else if (strcmp(extension, "cpp") == 0)
            {
                return small ? UI_FILE_CPP_ICON_TEXTURE
                             : UI_FILE_CPP_ICON_BIG_TEXTURE;
            }
            else if (strcmp(extension, "c") == 0 || strcmp(extension, "h") == 0)
            {
                return small ? UI_FILE_C_ICON_TEXTURE
                             : UI_FILE_C_ICON_BIG_TEXTURE;
            }
            else if (strcmp(extension, "java") == 0)
            {
                return small ? UI_FILE_JAVA_ICON_TEXTURE
                             : UI_FILE_JAVA_ICON_BIG_TEXTURE;
            }
        }
    }
    return icon_index;
}

internal void animate_based_on_selection(const b8 selected, const b8 hit,
                                         const V2 before, DirectoryItem* item)
{
    if (selected || hit)
    {
        if (!item->animation_on)
        {
            item->animation_on = true;
            item->before_animation = before;
            item->after_animation = v2i(10.0f);
            item->animation_precent = 0.0f;
        }
    }
    else
    {
        if (item->animation_on)
        {
            item->animation_on = false;
            item->before_animation = before;
            item->after_animation = v2i(0.0f);
            item->animation_precent = 0.0f;
        }
    }
}

internal b8 directory_item(V2 starting_position, V2 item_dimensions,
                           f32 icon_index, DirectoryItem* item, List* list)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;
    HoverClickedIndex hover_clicked_index =
        ui_context.window_hover_clicked_indices.data[window_index];

    b8 selected = false;
    b8 hit = check_directory_item_collision(starting_position, item_dimensions,
                                            item, &selected, list);

    AABB back_drop_aabb = {
        .min = starting_position,
        .size = item_dimensions,
    };
    array_push(aabbs, back_drop_aabb);

    if (hit || selected)
    {
        V4 color = selected ? v4ic(0.45f) : v4ic(0.3f);
        color = v4a(color, window->alpha);
#if 1
        quad_gradiant_l_r(&ui_context.render.vertices, back_drop_aabb.min,
                          back_drop_aabb.size, v4a(color, window->alpha),
                          global_get_clear_color(), 0.0f);
#else
        if (selected)
        {
            emit_aabb_particles(&back_drop_aabb, color, v2f(1.0f, 2.0f), 5);
        }
        quad(&ui_context.render.vertices, back_drop_aabb.min,
             back_drop_aabb.size, color, 0.0f);

#endif
        window->rendering_index_count += 6;
    }

    const V2 animated = directory_item_animate_position(item);
    animate_based_on_selection(selected, hit, v2i(animated.x), item);
    starting_position.x += animated.x;
    item_dimensions.width -= animated.x;

    icon_index = get_file_icon_based_on_extension(icon_index, item->name);

    const V2 icon_size = v2i(24.0f);
    AABB icon_aabb =
        quad(&ui_context.render.vertices,
             v2f(starting_position.x + 5.0f,
                 starting_position.y +
                     middle(item_dimensions.height, icon_size.height)),
             icon_size, v4a(v4i(1.0f), window->alpha), icon_index);
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
        window->rendering_index_count += text_generation_color(
            ui_context.font.chars, buffer, UI_FONT_TEXTURE, size_text_position,
            1.0f, ui_context.font.pixel_height,
            v4a(global_get_text_color(), window->alpha), NULL, NULL, NULL,
            &ui_context.render.vertices);
    }
    window->rendering_index_count += display_text_and_truncate_if_necissary(
        text_position,
        (item_dimensions.width - icon_aabb.size.x - 20.0f - x_advance),
        window->alpha, item->name);

    return hit && hover_clicked_index.double_clicked;
}

internal b8 directory_item_grid(V2 starting_position, V2 item_dimensions,
                                f32 icon_index, ThreadTaskQueue* task_queue,
                                SafeIdTexturePropertiesArray* textures,
                                DirectoryItem* item, List* list)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;
    HoverClickedIndex hover_clicked_index =
        ui_context.window_hover_clicked_indices.data[window_index];

    b8 selected = false;
    b8 hit = check_directory_item_collision(starting_position, item_dimensions,
                                            item, &selected, list);

    const V2 animated = directory_item_animate_position(item);
    animate_based_on_selection(selected, hit, animated, item);
    v2_sub_equal(&starting_position, animated);
    v2_add_equal(&item_dimensions, animated);

    AABB back_drop_aabb = {
        .min = starting_position,
        .size = item_dimensions,
    };
    array_push(aabbs, back_drop_aabb);

    if (hit || selected)
    {
        const V4 color = selected ? v4ic(0.45f) : v4ic(0.3f);
        quad(&ui_context.render.vertices, back_drop_aabb.min,
             back_drop_aabb.size, v4a(color, window->alpha), 0.0f);
        window->rendering_index_count += 6;
    }

    V2 icon_size = v2i(ui_big_icon_size);
    if (item->texture_id)
    {
        icon_index = (f32)ui_context.render.render.textures.size;
        array_push(&ui_context.render.render.textures, item->texture_id);
        i32 new_width = (i32)icon_size.width;
        i32 new_height = (i32)icon_size.height;
        if (item->texture_width > icon_size.width ||
            item->texture_height > icon_size.height)
        {
            texture_scale_down(item->texture_width, item->texture_height,
                               &new_width, &new_height);
        }
        else
        {
            new_width = item->texture_width;
            new_height = item->texture_height;
        }
        icon_size.width = (f32)new_width;
        icon_size.height = (f32)new_height;
    }
    else
    {
        icon_index = get_file_icon_based_on_extension(icon_index, item->name);
        if (!item->reload_thumbnail &&
            (icon_index == UI_FILE_PNG_ICON_BIG_TEXTURE ||
             icon_index == UI_FILE_JPG_ICON_BIG_TEXTURE))
        {
            LoadThumpnailData* thump_nail_data =
                (LoadThumpnailData*)calloc(1, sizeof(LoadThumpnailData));
            thump_nail_data->file_id = guid_copy(&item->id);
            thump_nail_data->array = textures;
            thump_nail_data->file_path = item->path;
            thump_nail_data->size = 128;
            ThreadTask task = {
                .data = thump_nail_data,
                .task_callback = load_thumpnails,
            };
            thread_tasks_push(task_queue, &task, 1, NULL);
            item->reload_thumbnail = true;
        }
    }

    AABB icon_aabb = quad(
        &ui_context.render.vertices,
        v2f(starting_position.x +
                middle(item_dimensions.width, icon_size.width),
            starting_position.y + 3.0f + (ui_big_icon_size - icon_size.height)),
        icon_size, v4a(v4i(1.0f), window->alpha), icon_index);
    window->rendering_index_count += 6;

    const f32 total_available_width_for_text = item_dimensions.width;

    f32 x_advance = text_x_advance(ui_context.font.chars, item->name,
                                   (u32)strlen(item->name), 1.0f);

    x_advance = min(x_advance, total_available_width_for_text);

    V2 text_position = starting_position;
    text_position.x += middle(total_available_width_for_text, x_advance);
    text_position.y = icon_aabb.min.y + icon_aabb.size.height;
    text_position.y += ui_context.font.pixel_height;

    window->rendering_index_count += display_text_and_truncate_if_necissary(
        text_position, total_available_width_for_text, window->alpha,
        item->name);

    return hit && hover_clicked_index.double_clicked;
}

i32 ui_window_add_directory_item_list(V2 position, const f32 icon_index,
                                      const f32 item_height,
                                      DirectoryItemArray* items, List* list)
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

    i32 double_clicked_index = -1;
    for (i32 i = 0; i < (i32)items->size; ++i)
    {
        if (item_in_view(position.y, item_dimensions.height, window->position.y,
                         window->size.height))
        {
            DirectoryItem* item = items->data + i;
            if (directory_item(position, item_dimensions, icon_index, item,
                               list))
            {
                if (list)
                {
                    list->item_to_change = item;
                }
                double_clicked_index = i;
            }
            if (item->rename && list && list->input.active)
            {
                const f32 icon_size = 24.0f;
                V2 input_field_position =
                    v2f(relative_position.x + icon_size + 20.0f,
                        relative_position.y + window->current_scroll_offset);
                ui_window_add_input_field(
                    input_field_position,
                    v2f((item_dimensions.width * 0.9f) - (icon_size + 20.0f),
                        item_dimensions.height),
                    &list->input);

                item->rename = list->input.active;

                if (event_is_key_pressed_once(FTIC_KEY_ENTER))
                {
                    list->item_to_change = item;
                    list->reload = true;
                    list->input.active = false;
                    item->rename = false;
                }
            }
        }
        position.y += item_height;
        relative_position.y += item_height;
    }
    // TODO: add this for all components.
    window->total_height =
        max(window->total_height, relative_position.y + item_height);
    return double_clicked_index;
}

internal void display_grid_item(const V2 position, const i32 index,
                                const DirectoryItemArray* folders,
                                const DirectoryItemArray* files,
                                const V2 item_dimensions,
                                ThreadTaskQueue* task_queue,
                                SafeIdTexturePropertiesArray* textures,
                                II32* hit_index, List* list)
{
    if (index < (i32)folders->size)
    {
        DirectoryItem* item = folders->data + index;
        if (directory_item_grid(position, item_dimensions,
                                UI_FOLDER_ICON_BIG_TEXTURE, task_queue,
                                textures, item, list))
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
                                UI_FILE_ICON_BIG_TEXTURE, task_queue, textures,
                                item, list))
        {
            hit_index->first = 1;
            hit_index->second = new_index;
        }
    }
}

II32 ui_window_add_directory_item_grid(V2 position,
                                       const DirectoryItemArray* folders,
                                       const DirectoryItemArray* files,
                                       ThreadTaskQueue* task_queue,
                                       SafeIdTexturePropertiesArray* textures,
                                       List* list)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    V2 relative_position = position;
    v2_add_equal(&position, window->first_item_position);

    position.y += window->current_scroll_offset;
    position.x += window->current_scroll_offset_width;

    V2 item_dimensions = v2i(ui_big_icon_size);
    item_dimensions.height += (ui_context.font.pixel_height + 5.0f);

    const u32 item_count = folders->size + files->size;
    const f32 grid_padding = 10.0f;
    const f32 total_area_space = window->size.width - relative_position.x;
    const i32 columns = max(
        (i32)(total_area_space / (item_dimensions.width + grid_padding)), 1);
    const i32 rows = item_count / columns;
    const i32 last_row = item_count % columns;

    const f32 total_items_width = columns * item_dimensions.width;
    const f32 remaining_space = total_area_space - total_items_width;
    const f32 grid_padding_width = max(remaining_space / (columns + 1), 0.0f);

    position.x += grid_padding_width;
    const f32 start_x = position.x;

    II32 hit_index = { .first = -1, .second = -1 };
    for (i32 row = 0; row < rows; ++row)
    {
        if (item_in_view(position.y, item_dimensions.height, window->position.y,
                         window->size.height))
        {
            for (i32 column = 0; column < columns; ++column)
            {
                const i32 index = (row * columns) + column;
                display_grid_item(position, index, folders, files,
                                  item_dimensions, task_queue, textures,
                                  &hit_index, list);
                position.x += item_dimensions.width + grid_padding_width;
            }
            position.x = start_x;
        }
        position.y += item_dimensions.height + grid_padding;
        relative_position.y += item_dimensions.height + grid_padding;
    }
    if (item_in_view(position.y, item_dimensions.height, window->position.y,
                     window->size.height))
    {
        for (i32 column = 0; column < last_row; ++column)
        {
            const i32 index = (rows * columns) + column;
            display_grid_item(position, index, folders, files, item_dimensions,
                              task_queue, textures, &hit_index, list);
            position.x += item_dimensions.width + grid_padding_width;
        }
    }
    relative_position.y +=
        (item_dimensions.height + grid_padding) * (last_row > 0);

    window->total_height =
        max(window->total_height,
            relative_position.y + ui_big_icon_size + grid_padding);

    return hit_index;
}

b8 ui_window_add_folder_list(V2 position, const f32 item_height,
                             DirectoryItemArray* items, List* list,
                             i32* double_clicked_index)
{
    i32 result = ui_window_add_directory_item_list(
        position, UI_FOLDER_ICON_TEXTURE, item_height, items, list);
    if (double_clicked_index) *double_clicked_index = result;
    return result != -1;
}

b8 ui_window_add_file_list(V2 position, const f32 item_height,
                           DirectoryItemArray* items, List* list,
                           i32* double_clicked_index)
{
    i32 result = ui_window_add_directory_item_list(
        position, UI_FILE_ICON_TEXTURE, item_height, items, list);
    if (double_clicked_index) *double_clicked_index = result;
    return result != -1;
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

    quad_border_gradiant(&ui_context.render.vertices,
                         &window->rendering_index_count,
                         v2f(position.x - drop_down_border_width,
                             position.y - drop_down_border_width),
                         v2f(drop_down_width + border_extra_padding,
                             current_y + border_extra_padding),
                         global_get_lighter_color(), global_get_lighter_color(),
                         drop_down_border_width, 0.0f);

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

        const V4 drop_down_color = drop_down_item_hit
                                       ? global_get_border_color()
                                       : global_get_highlight_color();
        V2 size = v2f(drop_down_width, drop_down_item_height * precent);
        array_push(aabbs, quad(&ui_context.render.vertices, promt_item_position,
                               size, drop_down_color, 0.0f));
        window->rendering_index_count += 6;

        V2 promt_item_text_position = promt_item_position;
        promt_item_text_position.y +=
            ui_context.font.pixel_height + promt_item_text_padding + 3.0f;
        promt_item_text_position.x += promt_item_text_padding;

        V4 text_color = global_get_text_color();
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

void ui_window_add_text_c(V2 position, V4 color, const char* text, b8 scrolling)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;

    V2 relative_position = position;
    v2_add_equal(&position, window->first_item_position);

    if (ui_context.column || ui_context.row)
    {
        if (ui_context.row)
        {
            position.x += ui_context.row_current;
        }
        else
        {
            position.y += ui_context.column_current;
        }
    }

    position.y += window->current_scroll_offset + ui_context.font.pixel_height;
    position.x += window->current_scroll_offset_width;

    u32 new_lines = 0;
    f32 x_advance = 0.0f;
    window->rendering_index_count += text_generation_color(
        ui_context.font.chars, text, UI_FONT_TEXTURE, position, 1.0f,
        ui_context.font.line_height, color, &new_lines, &x_advance, NULL,
        &ui_context.render.vertices);

    if (ui_context.column || ui_context.row)
    {
        if (ui_context.row)
        {
            ui_context.row_current += x_advance + ui_context.row_padding;
        }
        else
        {
            ui_context.column_current +=
                ((new_lines + 1) * ui_context.font.line_height) +
                ui_context.column_padding;
        }
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

void ui_window_add_text(V2 position, const char* text, b8 scrolling)
{
    ui_window_add_text_c(position, v4ic(1.0f), text, scrolling);
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
        ui_context.row_current += x_advance + ui_context.row_padding;
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

void ui_window_add_image(V2 position, V2 image_dimensions, u32 image)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;

    V2 relative_position = position;
    v2_add_equal(&position, window->first_item_position);

    position.x += window->current_scroll_offset_width;
    position.y += window->current_scroll_offset;

    f32 texture_index = (f32)ui_context.render.render.textures.size;
    array_push(&ui_context.render.render.textures, image);

    quad(&ui_context.render.vertices, position, image_dimensions, v4i(1.0f),
         texture_index);
    window->rendering_index_count += 6;

    window->total_height = max(window->total_height,
                               relative_position.y + image_dimensions.height);

    window->total_width =
        max(window->total_width, relative_position.x + image_dimensions.width);
}

V2 ui_window_get_button_dimensions(V2 dimensions, const char* text,
                                   f32* x_advance_out)
{
    f32 x_advance = 20.0f;
    f32 pixel_height = 10.0f;
    if (text)
    {
        x_advance += text_x_advance(ui_context.font.chars, text,
                                    (u32)strlen(text), 1.0f);
        pixel_height += ui_context.font.pixel_height;

        if (x_advance_out)
        {
            *x_advance_out = x_advance - 20.0f;
        }
    }

    return v2f(max(dimensions.width, x_advance),
               max(dimensions.height, pixel_height));
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

    V2 end_dimensions;
    f32 x_advance;
    if (dimensions)
    {
        end_dimensions =
            ui_window_get_button_dimensions(*dimensions, text, &x_advance);
        *dimensions = end_dimensions;
    }
    else
    {
        end_dimensions =
            ui_window_get_button_dimensions(v2d(), text, &x_advance);
    }

    if (ui_context.column || ui_context.row)
    {
        if (ui_context.row)
        {
            position.x += ui_context.row_current;
            ui_context.row_current +=
                end_dimensions.width + ui_context.row_padding;
        }
        else
        {
            position.y += ui_context.column_current;
            ui_context.column_current +=
                end_dimensions.height + ui_context.column_padding;
        }
    }

    b8 collided = hover_clicked_index.index == (i32)aabbs->size;
    const V4 col = v4a(v4_s_multi(global_get_clear_color(), 1.5f), 1.0f);
    V4 button_color = color ? *color : col;
    if (collided && event_get_mouse_button_event()->action != FTIC_PRESS)
    {
        button_color = v4_s_multi(button_color, 1.5f);
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
            v2f(position.x + middle(end_dimensions.width, x_advance),
                position.y + ui_context.font.pixel_height + 2.0f);
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
         v2f(ui_context.dimensions.width, pixel_height),
         global_get_clear_color(), 0.0f);
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

    if (ui_context.column || ui_context.row)
    {
        if (ui_context.row)
        {
            position.x += ui_context.row_current;
            ui_context.row_current += size.width + ui_context.row_padding;
        }
        else
        {
            position.y += ui_context.column_current;
            ui_context.column_current +=
                size.height + ui_context.column_padding;
        }
    }

    quad_co(&ui_context.render.vertices, position, size, v4ic(1.0f),
            texture_coordinates, texture_index);
    window->rendering_index_count += 6;
}

V2 ui_window_get_switch_size()
{
    return v2_s_add(v2f(24.0f, 4.0f), ui_context.font.pixel_height);
}

void ui_window_add_switch(V2 position, b8* selected, f32* x)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;
    HoverClickedIndex hover_clicked_index =
        ui_context.window_hover_clicked_indices.data[window_index];

    v2_add_equal(&position, window->first_item_position);

    V2 size = ui_window_get_switch_size();
    if (ui_context.row)
    {
        position.x += ui_context.row_current;
        ui_context.row_current += size.width + ui_context.row_padding;
    }

    b8 hover = hover_clicked_index.index == (i32)aabbs->size;

    if (hover && hover_clicked_index.clicked)
    {
        *selected ^= true;
        *x = 0.0f;
    }

    AABB aabb = quad(&ui_context.render.vertices, position, size,
                     global_get_highlight_color(), 0.0f);
    array_push(aabbs, aabb);
    window->rendering_index_count += 6;

    V4 color =
        *selected ? global_get_secondary_color() : global_get_border_color();
    V2 t_position = aabb.min;

    if (*selected)
    {
        t_position = v2_lerp(
            aabb.min, v2f(aabb.min.x + (aabb.size.width * 0.5f), aabb.min.y),
            ease_out_cubic(*x));
        *x = clampf32_high(*x + (f32)ui_context.delta_time * 5.0f, 1.0f);
    }
    else
    {
        t_position =
            v2_lerp(v2f(aabb.min.x + (aabb.size.width * 0.5f), aabb.min.y),
                    aabb.min, ease_out_cubic(*x));
        *x = clampf32_high(*x + (f32)ui_context.delta_time * 5.0f, 1.0f);
    }
    quad(&ui_context.render.vertices, t_position,
         v2f(aabb.size.width * 0.5f, aabb.size.height), color, 0.0f);
    window->rendering_index_count += 6;
}

b8 ui_window_add_drop_down(V2 position, b8* open)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;
    HoverClickedIndex hover_clicked_index =
        ui_context.window_hover_clicked_indices.data[window_index];

    v2_add_equal(&position, window->first_item_position);
    return *open;
}

void ui_window_close(u32 window_id)
{
    const u32 window_index = ui_context.id_to_index.data[window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    DockNode* dock_space = window->dock_node;
    if (dock_space->windows.size == 1)
    {
        if (window->docked)
        {
            dock_node_remove_node(ui_context.dock_tree, dock_space);
        }
        window->closing = true;
        window->hide = false;
        window->docked = false;
        V2 end_position =
            v2_add(window->position, v2_s_multi(window->size, 0.5f));
        ui_window_start_position_animation(window, window->position,
                                           end_position);
        ui_window_start_size_animation(window, window->size, v2d());
    }
    else
    {
        u32 i = 0;
        for (; i < dock_space->windows.size; ++i)
        {
            if (dock_space->windows.data[i] == window_id)
            {
                break;
            }
        }
        ftic_assert(i != dock_space->windows.size);
        u32 focused_window_id =
            remove_window_from_shared_dock_space(i, window, dock_space);
        ui_context.window_in_focus =
            ui_context.id_to_index.data[focused_window_id];

        window->release_from_dock_space = false;
    }
}

f32 ui_window_add_slider(V2 position, V2 size, const f32 min_value,
                         const f32 max_value, f32 value, b8* pressed)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;
    HoverClickedIndex hover_clicked_index =
        ui_context.window_hover_clicked_indices.data[window_index];

    v2_add_equal(&position, window->first_item_position);

    b8 hit = hover_clicked_index.index == (i32)aabbs->size;

    V4 color = global_get_bright_color();
    if (hit || *pressed)
    {
        color = v4a(v4_s_multi(color, 1.4f), 1.0f);
    }
    quad(&ui_context.render.vertices, position, size, color, 0.0f);
    window->rendering_index_count += 6;

    const V2 slider_size = v2f(15.0f, size.height + 20.0f);

    if (hit && hover_clicked_index.pressed)
    {
        *pressed = true;
    }
    if (event_get_mouse_button_event()->action == FTIC_RELEASE)
    {
        *pressed = false;
    }

    V2 slider_position = position;
    slider_position.y -= 10.0f;

    const f32 start_x = position.x;
    const f32 end_x = position.x + (size.width - slider_size.width);

    if (*pressed)
    {
        f32 position_x =
            event_get_mouse_position().x - (slider_size.width * 0.5f);
        position_x = clampf32_low(position_x, start_x);
        position_x = clampf32_high(position_x, end_x);

        const f32 p = (position_x - start_x) / (end_x - start_x);
        value = lerp_f32(min_value, max_value, p);
    }

    value = clampf32_low(value, min_value);
    value = clampf32_high(value, max_value);

    const f32 p = (value - min_value) / (max_value - min_value);
    slider_position.x = lerp_f32(start_x, end_x, p);

    array_push(aabbs, quad(&ui_context.render.vertices, slider_position,
                           slider_size, global_get_secondary_color(), 0.0f));
    window->rendering_index_count += 6;

    return value;
}
