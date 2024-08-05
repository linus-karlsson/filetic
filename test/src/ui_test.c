#include "ui_test.h"
#include "ui.c"
#include "asserts.h"
#include "event_di.h"

global u32 g_total_test_failed_count = 0;

void ui_test_begin()
{
    printf("Ui tests:\n");
}

void ui_test_end()
{
    if (g_total_test_failed_count)
    {
        printf("\tTotal failed tests: %u\n", g_total_test_failed_count);
    }
    else
    {
        printf("\tNo failed tests\n");
    }
}

internal void free_nodes(DockNode* root)
{
    if (root == NULL)
    {
        return;
    }
    free_nodes(root->children[0]);
    free_nodes(root->children[1]);

    if (root->type == NODE_LEAF)
    {
        array_free(&root->windows);
    }
    free(root);
}

void ui_test_init_context()
{
    ui_context.check_collisions = true;
    ui_context.dock_tree = dock_node_create(NODE_ROOT, SPLIT_NONE, -1);
    ui_context.dock_tree->aabb.size = v2f(1000.0f, 800.0f);
    ui_context.animation_off = true;

    array_create(&ui_context.id_to_index, 10);

    array_create(&ui_context.windows, 10);
    array_create(&ui_context.window_aabbs, 10);
    array_create(&ui_context.window_hover_clicked_indices, 10);
    array_create(&ui_context.animation_x, 10);

    for (u32 i = 0; i < 10; ++i)
    {
        DockNode* node = dock_node_create(NODE_LEAF, SPLIT_NONE, i);
        insert_window(node, i, false);
        array_push(&ui_context.id_to_index, i);
    }
}

void ui_test_uninit_context()
{
    for (u32 i = 0; i < 10; ++i)
    {
        array_free(&ui_context.window_aabbs.data[i]);
        if (!check_bit(ui_context.windows.data[i].flags, UI_WINDOW_DOCKED))
        {
            array_free(&ui_context.windows.data[i].dock_node->windows);
            free(ui_context.windows.data[i].dock_node);
        }
    }
    free_nodes(ui_context.dock_tree);
    array_free(&ui_context.windows);
    array_free(&ui_context.window_aabbs);
    array_free(&ui_context.window_hover_clicked_indices);
    array_free(&ui_context.animation_x);
}

void ui_test_set_scroll_offset()
{
    // Normal
    MouseWheelEvent mouse_wheel_event = { .y_offset = -10.0f };
    event_inject_mouse_wheel_event(mouse_wheel_event);
    f32 actual = set_scroll_offset(2500.0f, 1000.0f, 0.0f);
    ASSERT_EQUALS(-1000.0f, actual, EQUALS_FORMAT_FLOAT);

    mouse_wheel_event.y_offset = 15.0f;
    event_inject_mouse_wheel_event(mouse_wheel_event);
    actual = set_scroll_offset(1000.0f, 500.0f, -2000.0f);
    ASSERT_EQUALS(-500.0f, actual, EQUALS_FORMAT_FLOAT);

    // clamp high
    mouse_wheel_event.y_offset = -3.0f;
    event_inject_mouse_wheel_event(mouse_wheel_event);
    actual = set_scroll_offset(2500.0f, 1000.0f, 301.0f);
    ASSERT_EQUALS(0.0f, actual, EQUALS_FORMAT_FLOAT);

    // clamp low
    actual = set_scroll_offset(234.0f, 554.0f, 0.0f);
    ASSERT_EQUALS(0.0f, actual, EQUALS_FORMAT_FLOAT);
}

void ui_test_smooth_scroll()
{
    f32 actual = smooth_scroll(0.0166f, -134.5f, 12.0f, -10.0f);

    ASSERT_EQUALS_WITHIN(-34.8f, actual, 0.01f, EQUALS_FORMAT_FLOAT);
}

void ui_test_push_window_to_front()
{
    U32Array windows = { 0 };

    // Array is empty
    array_create(&windows, 10);
    push_window_to_front(&windows, 0);
    ASSERT_EQUALS(0, windows.size, EQUALS_FORMAT_U32);
    array_free(&windows);

    // Value already at the front
    array_create(&windows, 10);
    array_push(&windows, 1);
    array_push(&windows, 2);
    array_push(&windows, 3);
    push_window_to_front(&windows, 0);
    ASSERT_EQUALS(1, windows.data[0], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(2, windows.data[1], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(3, windows.data[2], EQUALS_FORMAT_U32);
    array_free(&windows);

    // Value at the back
    array_create(&windows, 10);
    array_push(&windows, 1);
    array_push(&windows, 2);
    array_push(&windows, 3);
    push_window_to_front(&windows, 2);
    ASSERT_EQUALS(3, windows.data[0], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(1, windows.data[1], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(2, windows.data[2], EQUALS_FORMAT_U32);
    array_free(&windows);

    // Value in the middle
    array_create(&windows, 10);
    array_push(&windows, 1);
    array_push(&windows, 2);
    array_push(&windows, 3);
    array_push(&windows, 4);
    push_window_to_front(&windows, 2);
    ASSERT_EQUALS(3, windows.data[0], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(1, windows.data[1], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(2, windows.data[2], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(4, windows.data[3], EQUALS_FORMAT_U32);
    array_free(&windows);

    // Single element array
    array_create(&windows, 10);
    array_push(&windows, 1);
    push_window_to_front(&windows, 0);
    ASSERT_EQUALS(1, windows.data[0], EQUALS_FORMAT_U32);
    array_free(&windows);

    // Index out of bounds
    array_create(&windows, 10);
    array_push(&windows, 1);
    array_push(&windows, 2);
    array_push(&windows, 3);
    push_window_to_front(&windows, 3);
    ASSERT_EQUALS(1, windows.data[0], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(2, windows.data[1], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(3, windows.data[2], EQUALS_FORMAT_U32);
    array_free(&windows);
}

void ui_test_push_window_to_back()
{
    U32Array windows = { 0 };

    // Array is empty
    array_create(&windows, 10);
    push_window_to_back(&windows, 0);
    ASSERT_EQUALS(0, windows.size, EQUALS_FORMAT_U32);
    array_free(&windows);

    // Value already at the back
    array_create(&windows, 10);
    array_push(&windows, 1);
    array_push(&windows, 2);
    array_push(&windows, 3);
    push_window_to_back(&windows, 2);
    ASSERT_EQUALS(1, windows.data[0], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(2, windows.data[1], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(3, windows.data[2], EQUALS_FORMAT_U32);
    array_free(&windows);

    // Value at the front
    array_create(&windows, 10);
    array_push(&windows, 1);
    array_push(&windows, 2);
    array_push(&windows, 3);
    push_window_to_back(&windows, 0);
    ASSERT_EQUALS(2, windows.data[0], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(3, windows.data[1], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(1, windows.data[2], EQUALS_FORMAT_U32);
    array_free(&windows);

    // Value in the middle
    array_create(&windows, 10);
    array_push(&windows, 1);
    array_push(&windows, 2);
    array_push(&windows, 3);
    array_push(&windows, 4);
    push_window_to_back(&windows, 2);
    ASSERT_EQUALS(1, windows.data[0], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(2, windows.data[1], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(4, windows.data[2], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(3, windows.data[3], EQUALS_FORMAT_U32);
    array_free(&windows);

    // Single element array
    array_create(&windows, 10);
    array_push(&windows, 1);
    push_window_to_back(&windows, 0);
    ASSERT_EQUALS(1, windows.data[0], EQUALS_FORMAT_U32);
    array_free(&windows);

    // Index out of bounds
    array_create(&windows, 10);
    array_push(&windows, 1);
    array_push(&windows, 2);
    array_push(&windows, 3);
    push_window_to_back(&windows, 3);
    ASSERT_EQUALS(1, windows.data[0], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(2, windows.data[1], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(3, windows.data[2], EQUALS_FORMAT_U32);
    array_free(&windows);
}

void ui_test_generate_id()
{
    U32Array free_ids = { 0 };
    U32Array id_to_index = { 0 };

    // No free ids
    array_create(&free_ids, 10);
    array_create(&id_to_index, 10);

    generate_id(0, &free_ids, &id_to_index);
    generate_id(1, &free_ids, &id_to_index);
    generate_id(2, &free_ids, &id_to_index);
    ASSERT_EQUALS(3, id_to_index.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, id_to_index.data[0], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(1, id_to_index.data[1], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(2, id_to_index.data[2], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, free_ids.size, EQUALS_FORMAT_U32);

    // Values in free ids
    array_push(&free_ids, 2);
    array_push(&free_ids, 0);

    generate_id(3, &free_ids, &id_to_index);
    generate_id(4, &free_ids, &id_to_index);

    ASSERT_EQUALS(3, id_to_index.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(3, id_to_index.data[0], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(1, id_to_index.data[1], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(4, id_to_index.data[2], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, free_ids.size, EQUALS_FORMAT_U32);
}

void ui_test_dock_node_create_()
{
    DockNode* node = dock_node_create_(NODE_LEAF, SPLIT_NONE);
    ASSERT_EQUALS(NODE_LEAF, node->type, EQUALS_FORMAT_I32);
    ASSERT_EQUALS(SPLIT_NONE, node->split_axis, EQUALS_FORMAT_I32);
    ASSERT_EQUALS(0.5f, node->size_ratio, EQUALS_FORMAT_FLOAT);
    free(node);
}

void ui_test_dock_node_create()
{
    DockNode* node = dock_node_create(NODE_LEAF, SPLIT_HORIZONTAL, -1);
    ASSERT_EQUALS(NODE_LEAF, node->type, EQUALS_FORMAT_I32);
    ASSERT_EQUALS(SPLIT_HORIZONTAL, node->split_axis, EQUALS_FORMAT_I32);
    ASSERT_EQUALS(0, node->windows.size, EQUALS_FORMAT_U32);
    free(node);

    i32 window = 42;
    node = dock_node_create(NODE_LEAF, SPLIT_VERTICAL, window);
    ASSERT_EQUALS(1, node->windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS((u32)window, node->windows.data[0], EQUALS_FORMAT_U32);
    array_free(&node->windows);
    free(node);
}

void ui_test_dock_node_create_multiple_windows()
{
    U32Array windows;
    array_create(&windows, 2);
    array_push(&windows, 10);
    array_push(&windows, 20);

    DockNode* node = dock_node_create_multiple_windows(NODE_PARENT, SPLIT_VERTICAL, windows);
    ASSERT_EQUALS(2, node->windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(10, node->windows.data[0], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(20, node->windows.data[1], EQUALS_FORMAT_U32);
    array_free(&node->windows);
    free(node);
}

void ui_test_dock_node_set_split()
{
    ui_test_init_context();

    DockNode* parent = dock_node_create(NODE_PARENT, SPLIT_NONE, -1);
    parent->aabb.size.width = 100;
    parent->aabb.size.height = 200;

    DockNode* child1 = dock_node_create(NODE_LEAF, SPLIT_NONE, 0);
    DockNode* child2 = dock_node_create(NODE_LEAF, SPLIT_NONE, 1);
    parent->children[0] = child1;
    parent->children[1] = child2;

    dock_node_set_split(parent, parent, SPLIT_HORIZONTAL, false);
    ASSERT_EQUALS(100.0f, child1->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(100.0f, child2->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(100.0f, child1->aabb.size.height, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(100.0f, child2->aabb.size.height, EQUALS_FORMAT_FLOAT);

    dock_node_set_split(parent, parent, SPLIT_VERTICAL, false);
    ASSERT_EQUALS(50.0f, child1->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(50.0f, child2->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(200.0f, child1->aabb.size.height, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(200.0f, child2->aabb.size.height, EQUALS_FORMAT_FLOAT);

    parent->size_ratio = 0.88f;
    dock_node_set_split(parent, parent, SPLIT_HORIZONTAL, false);
    ASSERT_EQUALS(100.0f, child1->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(100.0f, child2->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(200.0f * 0.88f, child1->aabb.size.height, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(200.0f - (200.0f * 0.88f), child2->aabb.size.height, EQUALS_FORMAT_FLOAT);

    dock_node_set_split(parent, parent, SPLIT_VERTICAL, false);
    ASSERT_EQUALS(100.0f * 0.88f, child1->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(100.0f - (100.0f * 0.88f), child2->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(200.0f, child1->aabb.size.height, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(200.0f, child2->aabb.size.height, EQUALS_FORMAT_FLOAT);

    array_free(&child1->windows);
    array_free(&child2->windows);
    free(child1);
    free(child2);
    free(parent);

    ui_test_uninit_context();
}

void ui_test_dock_node_resize_traverse()
{
    ui_test_init_context();

    DockNode* parent = dock_node_create(NODE_PARENT, SPLIT_NONE, -1);
    parent->aabb.size.width = 150;
    parent->aabb.size.height = 470;

    DockNode* child1 = dock_node_create(NODE_LEAF, SPLIT_NONE, 0);
    DockNode* child2 = dock_node_create(NODE_LEAF, SPLIT_NONE, 1);
    parent->children[0] = child1;
    parent->children[1] = child2;

    parent->split_axis = SPLIT_VERTICAL;
    dock_node_resize_traverse(parent, false);
    ASSERT_EQUALS(75.0f, child1->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(75.0f, child2->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(470.0f, child1->aabb.size.height, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(470.0f, child2->aabb.size.height, EQUALS_FORMAT_FLOAT);

    ASSERT_EQUALS(0.0f, child1->aabb.min.x, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, child1->aabb.min.y, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(75.0f, child2->aabb.min.x, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, child2->aabb.min.y, EQUALS_FORMAT_FLOAT);

    parent->split_axis = SPLIT_HORIZONTAL;
    dock_node_resize_traverse(parent, false);
    ASSERT_EQUALS(150.0f, child1->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(150.0f, child2->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(235.0f, child1->aabb.size.height, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(235.0f, child2->aabb.size.height, EQUALS_FORMAT_FLOAT);

    ASSERT_EQUALS(0.0f, child1->aabb.min.x, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, child1->aabb.min.y, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, child2->aabb.min.x, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(235.0f, child2->aabb.min.y, EQUALS_FORMAT_FLOAT);

    child1->type = NODE_PARENT;
    child1->split_axis = SPLIT_VERTICAL;
    child1->size_ratio = 0.63f;
    child1->children[0] = dock_node_create(NODE_LEAF, SPLIT_NONE, 0);
    child1->children[1] = dock_node_create(NODE_LEAF, SPLIT_NONE, 2);

    parent->size_ratio = 0.35f;
    dock_node_resize_traverse(parent, false);
    ASSERT_EQUALS(150.0f, child2->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(470.0f - (470.0f * parent->size_ratio), child2->aabb.size.height,
                  EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, child2->aabb.min.x, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(470.0f * parent->size_ratio, child2->aabb.min.y, EQUALS_FORMAT_FLOAT);

    DockNode* child3 = child1->children[0];
    DockNode* child4 = child1->children[1];
    ASSERT_EQUALS(150.0f * child1->size_ratio, child3->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(470.0f * parent->size_ratio, child3->aabb.size.height, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, child3->aabb.min.x, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, child3->aabb.min.y, EQUALS_FORMAT_FLOAT);

    ASSERT_EQUALS(150.0f - (150.0f * child1->size_ratio), child4->aabb.size.width,
                  EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(470.0f * parent->size_ratio, child4->aabb.size.height, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(150.0f * child1->size_ratio, child4->aabb.min.x, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, child4->aabb.min.y, EQUALS_FORMAT_FLOAT);

    array_free(&child1->windows);
    array_free(&child1->children[0]->windows);
    array_free(&child1->children[1]->windows);
    array_free(&child2->windows);
    free(child1->children[0]);
    free(child1->children[1]);
    free(child1);
    free(child2);
    free(parent);

    ui_test_uninit_context();
}

internal void test_docked_leaf(const DockNode* node, const u32 window_index, const u32 window_size,
                               const char* file, const i32 line)
{
    ASSERT_EQUALS_MSG(NULL, node->children[0], EQUALS_FORMAT_PTR, FILE_LINE_FORMAT);
    ASSERT_EQUALS_MSG(NULL, node->children[1], EQUALS_FORMAT_PTR, FILE_LINE_FORMAT);
    ASSERT_EQUALS_MSG(SPLIT_NONE, node->split_axis, EQUALS_FORMAT_U32, FILE_LINE_FORMAT);
    ASSERT_EQUALS_MSG(NODE_LEAF, node->type, EQUALS_FORMAT_U32, FILE_LINE_FORMAT);
    ASSERT_EQUALS_MSG(window_size, node->windows.size, EQUALS_FORMAT_U32, FILE_LINE_FORMAT);
    ASSERT_EQUALS_MSG(ui_context.windows.data[window_index].id, node->windows.data[0],
                      EQUALS_FORMAT_U32, FILE_LINE_FORMAT);
    for (u32 i = 0; i < node->windows.size; ++i)
    {
        ASSERT_EQUALS_MSG(true,
                          check_bit(ui_window_get(node->windows.data[i])->flags, UI_WINDOW_DOCKED),
                          EQUALS_FORMAT_U32, FILE_LINE_FORMAT);
    }
}

internal void test_docked_parent(const DockNode* node, const SplitAxis split_axis, const char* file,
                                 const i32 line)
{
    ASSERT_NOT_EQUALS_MSG(NULL, node->children[0], EQUALS_FORMAT_PTR, FILE_LINE_FORMAT);
    ASSERT_NOT_EQUALS_MSG(NULL, node->children[1], EQUALS_FORMAT_PTR, FILE_LINE_FORMAT);
    ASSERT_EQUALS_MSG(split_axis, node->split_axis, EQUALS_FORMAT_U32, FILE_LINE_FORMAT);
    ASSERT_EQUALS_MSG(NODE_PARENT, node->type, EQUALS_FORMAT_U32, FILE_LINE_FORMAT);
    ASSERT_EQUALS_MSG(0, node->windows.size, EQUALS_FORMAT_U32, FILE_LINE_FORMAT);
}

internal void test_root()
{
    ASSERT_EQUALS(-1, ui_context.dock_side_hit, EQUALS_FORMAT_I32);

    ASSERT_NOT_EQUALS(NULL, ui_context.dock_tree->children[0], EQUALS_FORMAT_PTR);
    ASSERT_EQUALS(NULL, ui_context.dock_tree->children[1], EQUALS_FORMAT_PTR);
}

// NOTE: testing size in ui_test_dock_node_resize_from_root
void ui_test_dock_node_dock_window()
{
    ui_test_init_context();
    ui_context.dock_side_hit = -1;

    for (u32 i = 0; i < ui_context.windows.size; ++i)
    {
        ASSERT_EQUALS(false, check_bit(ui_context.windows.data[i].flags, UI_WINDOW_DOCKED),
                      EQUALS_FORMAT_U32);
    }

    ///////////////////////////////////

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(0)->dock_node, SPLIT_VERTICAL,
                          DOCK_SIDE_LEFT);
    set_bit(ui_window_get_(0)->flags, UI_WINDOW_DOCKED);
    test_root();
    window_animate(ui_window_get_(0));

    DockNode* left = ui_context.dock_tree->children[0];
    test_docked_leaf(left, 0, 1, __FILE__, __LINE__);

    ///////////////////////////////////

    ///////////////////////////////////

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(1)->dock_node, SPLIT_HORIZONTAL,
                          DOCK_SIDE_TOP);
    set_bit(ui_window_get_(1)->flags, UI_WINDOW_DOCKED);
    test_root();
    window_animate(ui_window_get_(0));
    window_animate(ui_window_get_(1));

    left = ui_context.dock_tree->children[0];
    test_docked_parent(left, SPLIT_HORIZONTAL, __FILE__, __LINE__);

    DockNode* right = left->children[1];
    left = left->children[0];
    test_docked_leaf(left, 1, 1, __FILE__, __LINE__);
    test_docked_leaf(right, 0, 1, __FILE__, __LINE__);

    ///////////////////////////////////

    ///////////////////////////////////

    dock_node_dock_window(ui_context.windows.data[1].dock_node, ui_window_get(2)->dock_node,
                          SPLIT_VERTICAL, DOCK_SIDE_LEFT);
    set_bit(ui_window_get_(2)->flags, UI_WINDOW_DOCKED);
    test_root();
    window_animate(ui_window_get_(0));
    window_animate(ui_window_get_(1));
    window_animate(ui_window_get_(2));

    left = ui_context.dock_tree->children[0];
    test_docked_parent(left, SPLIT_HORIZONTAL, __FILE__, __LINE__);

    right = left->children[1];
    left = left->children[0];

    test_docked_leaf(right, 0, 1, __FILE__, __LINE__);

    test_docked_parent(left, SPLIT_VERTICAL, __FILE__, __LINE__);
    right = left->children[1];
    left = left->children[0];
    test_docked_leaf(left, 2, 1, __FILE__, __LINE__);
    test_docked_leaf(right, 1, 1, __FILE__, __LINE__);

    ///////////////////////////////////

    ui_test_uninit_context();
}

void ui_test_dock_node_resize_from_root()
{
    ui_test_init_context();
    ui_context.dock_tree->aabb.size = v2f(1200.0f, 850.0f);
    ui_context.dock_side_hit = -1;

    for (u32 i = 0; i < ui_context.windows.size; ++i)
    {
        ASSERT_EQUALS(false, check_bit(ui_context.windows.data[i].flags, UI_WINDOW_DOCKED),
                      EQUALS_FORMAT_U32);
    }

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(0)->dock_node, SPLIT_HORIZONTAL,
                          DOCK_SIDE_TOP);
    set_bit(ui_window_get_(0)->flags, UI_WINDOW_DOCKED);
    dock_node_dock_window(ui_context.dock_tree, ui_window_get(1)->dock_node, SPLIT_HORIZONTAL,
                          DOCK_SIDE_TOP);
    set_bit(ui_window_get_(1)->flags, UI_WINDOW_DOCKED);
    dock_node_dock_window(ui_context.dock_tree, ui_window_get(2)->dock_node, SPLIT_VERTICAL,
                          DOCK_SIDE_RIGHT);
    set_bit(ui_window_get_(2)->flags, UI_WINDOW_DOCKED);
    dock_node_dock_window(ui_window_get(1)->dock_node, ui_window_get(3)->dock_node, SPLIT_VERTICAL,
                          DOCK_SIDE_RIGHT);
    set_bit(ui_window_get_(3)->flags, UI_WINDOW_DOCKED);

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(4)->dock_node, SPLIT_VERTICAL,
                          DOCK_SIDE_RIGHT);
    set_bit(ui_window_get_(4)->flags, UI_WINDOW_DOCKED);

    window_animate(ui_window_get_(0));
    window_animate(ui_window_get_(1));
    window_animate(ui_window_get_(2));
    window_animate(ui_window_get_(3));
    window_animate(ui_window_get_(4));

    DockNode* c = ui_context.dock_tree->children[0];
    DockNode* c0 = c->children[1];
    ASSERT_EQUALS(600.0f, c0->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(850.0f, c0->aabb.size.height, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(600.0f, c0->aabb.min.x, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, c0->aabb.min.y, EQUALS_FORMAT_FLOAT);

    c = c->children[0];
    c0 = c->children[1];
    ASSERT_EQUALS(300.0f, c0->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(850.0f, c0->aabb.size.height, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(300.0f, c0->aabb.min.x, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, c0->aabb.min.y, EQUALS_FORMAT_FLOAT);

    c = c->children[0];
    c0 = c->children[1];
    ASSERT_EQUALS(300.0f, c0->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(425.0f, c0->aabb.size.height, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, c0->aabb.min.x, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(425.0f, c0->aabb.min.y, EQUALS_FORMAT_FLOAT);

    c = c->children[0];
    c0 = c->children[1];
    ASSERT_EQUALS(150.0f, c0->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(425.0f, c0->aabb.size.height, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(150.0f, c0->aabb.min.x, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, c0->aabb.min.y, EQUALS_FORMAT_FLOAT);

    c0 = c->children[0];
    ASSERT_EQUALS(150.0f, c0->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(425.0f, c0->aabb.size.height, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, c0->aabb.min.x, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, c0->aabb.min.y, EQUALS_FORMAT_FLOAT);

    AABB new_dock_space = {
        .size = v2f(838.0f, 450.0f),
    };
    dock_node_resize_from_root(ui_context.dock_tree, &new_dock_space, false);

    c = ui_context.dock_tree->children[0];
    c0 = c->children[1];
    ASSERT_EQUALS(419.0f, c0->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(450.0f, c0->aabb.size.height, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(419.0f, c0->aabb.min.x, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, c0->aabb.min.y, EQUALS_FORMAT_FLOAT);

    c = c->children[0];
    c0 = c->children[1];
    ASSERT_EQUALS(209.5f, c0->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(450.0f, c0->aabb.size.height, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(209.5f, c0->aabb.min.x, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, c0->aabb.min.y, EQUALS_FORMAT_FLOAT);

    c = c->children[0];
    c0 = c->children[1];
    ASSERT_EQUALS(209.5f, c0->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(225.0f, c0->aabb.size.height, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, c0->aabb.min.x, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(225.0f, c0->aabb.min.y, EQUALS_FORMAT_FLOAT);

    c = c->children[0];
    c0 = c->children[1];
    ASSERT_EQUALS(104.75f, c0->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(225.0f, c0->aabb.size.height, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(104.75f, c0->aabb.min.x, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, c0->aabb.min.y, EQUALS_FORMAT_FLOAT);

    c0 = c->children[0];
    ASSERT_EQUALS(104.75f, c0->aabb.size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(225.0f, c0->aabb.size.height, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, c0->aabb.min.x, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(0.0f, c0->aabb.min.y, EQUALS_FORMAT_FLOAT);

    ui_test_uninit_context();
}

void ui_test_find_node()
{
    ui_test_init_context();
    ui_context.dock_tree->aabb.size = v2f(1200.0f, 850.0f);
    ui_context.dock_side_hit = -1;

    for (u32 i = 0; i < ui_context.windows.size; ++i)
    {
        ASSERT_EQUALS(false, check_bit(ui_context.windows.data[i].flags, UI_WINDOW_DOCKED),
                      EQUALS_FORMAT_U32);
    }

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(0)->dock_node, SPLIT_HORIZONTAL,
                          DOCK_SIDE_TOP);
    set_bit(ui_window_get_(0)->flags, UI_WINDOW_DOCKED);

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(1)->dock_node, SPLIT_HORIZONTAL,
                          DOCK_SIDE_TOP);
    set_bit(ui_window_get_(1)->flags, UI_WINDOW_DOCKED);

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(2)->dock_node, SPLIT_HORIZONTAL,
                          DOCK_SIDE_TOP);
    set_bit(ui_window_get_(2)->flags, UI_WINDOW_DOCKED);

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(3)->dock_node, SPLIT_VERTICAL,
                          DOCK_SIDE_RIGHT);
    set_bit(ui_window_get_(3)->flags, UI_WINDOW_DOCKED);

    dock_node_dock_window(ui_window_get(1)->dock_node, ui_window_get(4)->dock_node, SPLIT_VERTICAL,
                          DOCK_SIDE_RIGHT);
    set_bit(ui_window_get_(4)->flags, UI_WINDOW_DOCKED);

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(5)->dock_node, SPLIT_VERTICAL,
                          DOCK_SIDE_RIGHT);
    set_bit(ui_window_get_(5)->flags, UI_WINDOW_DOCKED);

    DockNode* node_before = find_node(ui_context.dock_tree->children[0], ui_context.dock_tree,
                                      ui_window_get(3)->dock_node);

    DockNode* c = ui_context.dock_tree;
    c = c->children[0];
    c = c->children[0];

    ASSERT_EQUALS(c, node_before, EQUALS_FORMAT_PTR);

    node_before = find_node(ui_context.dock_tree->children[0], ui_context.dock_tree,
                            ui_window_get(0)->dock_node);

    c = ui_context.dock_tree;
    c = c->children[0];
    c = c->children[0];
    c = c->children[0];
    c = c->children[1];

    ASSERT_EQUALS(c, node_before, EQUALS_FORMAT_PTR);

    node_before = find_node(ui_context.dock_tree->children[0], ui_context.dock_tree,
                            ui_window_get(1)->dock_node);

    c = c->children[0];

    ASSERT_EQUALS(c, node_before, EQUALS_FORMAT_PTR);

    node_before = find_node(ui_context.dock_tree->children[0], ui_context.dock_tree,
                            ui_window_get(2)->dock_node);

    c = ui_context.dock_tree;
    c = c->children[0];
    c = c->children[0];
    c = c->children[0];

    ASSERT_EQUALS(c, node_before, EQUALS_FORMAT_PTR);

    ui_test_uninit_context();
}

void ui_test_dock_node_remove_node()
{
    ui_test_init_context();
    ui_context.dock_tree->aabb.size = v2f(1200.0f, 850.0f);
    ui_context.dock_side_hit = -1;

    for (u32 i = 0; i < ui_context.windows.size; ++i)
    {
        ASSERT_EQUALS(false, check_bit(ui_context.windows.data[i].flags, UI_WINDOW_DOCKED),
                      EQUALS_FORMAT_U32);
    }

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(0)->dock_node, SPLIT_HORIZONTAL,
                          DOCK_SIDE_TOP);
    set_bit(ui_window_get_(0)->flags, UI_WINDOW_DOCKED);

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(1)->dock_node, SPLIT_HORIZONTAL,
                          DOCK_SIDE_TOP);
    set_bit(ui_window_get_(1)->flags, UI_WINDOW_DOCKED);

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(2)->dock_node, SPLIT_HORIZONTAL,
                          DOCK_SIDE_TOP);
    set_bit(ui_window_get_(2)->flags, UI_WINDOW_DOCKED);

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(3)->dock_node, SPLIT_VERTICAL,
                          DOCK_SIDE_RIGHT);
    set_bit(ui_window_get_(3)->flags, UI_WINDOW_DOCKED);

    dock_node_dock_window(ui_window_get(1)->dock_node, ui_window_get(4)->dock_node, SPLIT_VERTICAL,
                          DOCK_SIDE_RIGHT);
    set_bit(ui_window_get_(4)->flags, UI_WINDOW_DOCKED);

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(5)->dock_node, SPLIT_VERTICAL,
                          DOCK_SIDE_RIGHT);
    set_bit(ui_window_get_(5)->flags, UI_WINDOW_DOCKED);

    DockNode* c = ui_context.dock_tree;
    c = c->children[0];
    c = c->children[0];
    DockNode c1 = *c->children[0];

    DockNode* node = find_node(ui_context.dock_tree->children[0], ui_context.dock_tree,
                               ui_window_get(3)->dock_node);
    ASSERT_NOT_EQUALS(NULL, node, EQUALS_FORMAT_PTR);

    dock_node_remove_node(ui_context.dock_tree, ui_window_get(3)->dock_node);
    node = find_node(ui_context.dock_tree->children[0], ui_context.dock_tree,
                     ui_window_get(3)->dock_node);
    ASSERT_EQUALS(NULL, node, EQUALS_FORMAT_PTR);

    DockNode* c0 = ui_context.dock_tree;
    c0 = c0->children[0];
    c0 = c0->children[0];

    ASSERT_EQUALS(c1.type, c0->type, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(c1.split_axis, c0->split_axis, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(c1.window_in_focus, c0->window_in_focus, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(c1.size_ratio, c0->size_ratio, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(c1.children[0], c0->children[0], EQUALS_FORMAT_PTR);
    ASSERT_EQUALS(c1.children[1], c0->children[1], EQUALS_FORMAT_PTR);

    node = find_node(ui_context.dock_tree->children[0], ui_context.dock_tree,
                     ui_window_get(1)->dock_node);
    ASSERT_NOT_EQUALS(NULL, node, EQUALS_FORMAT_PTR);
    dock_node_remove_node(ui_context.dock_tree, ui_window_get(1)->dock_node);
    node = find_node(ui_context.dock_tree->children[0], ui_context.dock_tree,
                     ui_window_get(1)->dock_node);
    ASSERT_EQUALS(NULL, node, EQUALS_FORMAT_PTR);

    c0 = ui_context.dock_tree;
    c0 = c0->children[0];
    c0 = c0->children[0];
    c0 = c0->children[1];
    c0 = c0->children[0];
    ASSERT_EQUALS(c0, ui_window_get(4)->dock_node, EQUALS_FORMAT_PTR);

    ui_test_uninit_context();
}

void ui_test_set_docking()
{
    event_inject_mouse_position(v2f(135.0f, 148.0f));

    array_create(&ui_context.render.vertices, 10);

    u32 index_count0 = 0;
    b8 result = set_docking(v2f(100.0f, 100.0f), v2f(45.0f, 59.0f), v2d(), v2d(), &index_count0);
    ASSERT_EQUALS(true, result, EQUALS_FORMAT_U32);

    u32 index_count1 = 0;
    result = set_docking(v2f(100.0f, 100.0f), v2f(20.0f, 20.0f), v2d(), v2d(), &index_count1);
    ASSERT_EQUALS(false, result, EQUALS_FORMAT_U32);

    ASSERT_EQUALS(index_count0, (index_count1 + 6), EQUALS_FORMAT_U32);

    array_free(&ui_context.render.vertices);
    ui_context.render.vertices.size = 0;
    ui_context.render.vertices.capacity = 0;
}

void ui_test_display_docking()
{
    array_create(&ui_context.render.vertices, 10);

    const V2 docking_size_top_bottom = v2f(50.0f, 30.0f);
    const V2 docking_size_left_right = v2f(30.0f, 50.0f);

    const AABB aabb = {
        .min = v2i(134.0f),
        .size = v2i(450.0f),
    };

    const V2 middle_top_bottom =
        v2f(aabb.min.x + middle(aabb.size.width, docking_size_top_bottom.width),
            aabb.min.y + middle(aabb.size.height, docking_size_top_bottom.height));

    const V2 middle_left_right =
        v2f(aabb.min.x + middle(aabb.size.width, docking_size_left_right.width),
            aabb.min.y + middle(aabb.size.height, docking_size_left_right.height));

    const V2 top_position = v2f(middle_top_bottom.x, aabb.min.y + 10.0f);

    const V2 right_position =
        v2f(aabb.size.width - (docking_size_left_right.width + 10.0f), middle_left_right.y);

    const V2 bottom_position =
        v2f(middle_top_bottom.x,
            (aabb.min.y + aabb.size.height) - (docking_size_top_bottom.height + 10.0f));

    const V2 left_position = v2f(aabb.min.x + 10.0f, middle_left_right.y);

    const V2 docking_size_middle = v2i(50.0f);
    const f32 x = middle((right_position.x + docking_size_left_right.width) - left_position.x,
                         docking_size_middle.width);
    const f32 y = middle((bottom_position.y + docking_size_top_bottom.height) - top_position.y,
                         docking_size_middle.height);

    const V2 middle_position = v2f(left_position.x + x, top_position.y + y);

    // None
    event_inject_mouse_position(v2i(0.0f));
    i32 actual =
        display_docking(&aabb, top_position, right_position, bottom_position, left_position, true);
    ASSERT_EQUALS(-1, actual, EQUALS_FORMAT_I32);

    // Top
    event_inject_mouse_position(v2_s_add(top_position, 5.0f));
    actual =
        display_docking(&aabb, top_position, right_position, bottom_position, left_position, true);
    ASSERT_EQUALS(0, actual, EQUALS_FORMAT_I32);

    // Right
    event_inject_mouse_position(v2_s_add(right_position, 5.0f));
    actual =
        display_docking(&aabb, top_position, right_position, bottom_position, left_position, true);
    ASSERT_EQUALS(1, actual, EQUALS_FORMAT_I32);

    // Bottom
    event_inject_mouse_position(v2_s_add(bottom_position, 5.0f));
    actual =
        display_docking(&aabb, top_position, right_position, bottom_position, left_position, true);
    ASSERT_EQUALS(2, actual, EQUALS_FORMAT_I32);

    // Left
    event_inject_mouse_position(v2_s_add(left_position, 5.0f));
    actual =
        display_docking(&aabb, top_position, right_position, bottom_position, left_position, true);
    ASSERT_EQUALS(3, actual, EQUALS_FORMAT_I32);

    // Middle
    event_inject_mouse_position(v2_s_add(middle_position, 5.0f));
    actual =
        display_docking(&aabb, top_position, right_position, bottom_position, left_position, false);
    ASSERT_EQUALS(-1, actual, EQUALS_FORMAT_I32);

    actual =
        display_docking(&aabb, top_position, right_position, bottom_position, left_position, true);
    ASSERT_EQUALS(4, actual, EQUALS_FORMAT_I32);

    array_free(&ui_context.render.vertices);
    ui_context.render.vertices.size = 0;
    ui_context.render.vertices.capacity = 0;
}

void ui_test_remove_window_from_shared_dock_space()
{
    array_create(&ui_context.windows, 10);
    array_create(&ui_context.id_to_index, 10);

    UiWindow window = { 0 };
    array_push(&ui_context.windows, window);
    array_push(&ui_context.windows, window);
    array_push(&ui_context.windows, window);
    array_push(&ui_context.windows, window);

    array_push(&ui_context.windows, window);
    array_push(&ui_context.windows, window);

    array_push(&ui_context.id_to_index, 0);
    array_push(&ui_context.id_to_index, 1);
    array_push(&ui_context.id_to_index, 2);
    array_push(&ui_context.id_to_index, 3);
    array_push(&ui_context.id_to_index, 4);
    array_push(&ui_context.id_to_index, 5);

    DockNode* dock_space0 = dock_node_create(NODE_LEAF, SPLIT_NONE, 0);
    UiWindow* w = ui_window_get_(0);
    w->dock_node = dock_space0;
    set_bit(w->flags, UI_WINDOW_DOCKED);
    array_push(&dock_space0->windows, 1);
    w = ui_window_get_(1);
    w->dock_node = dock_space0;
    set_bit(w->flags, UI_WINDOW_DOCKED);
    array_push(&dock_space0->windows, 2);
    w = ui_window_get_(2);
    w->dock_node = dock_space0;
    set_bit(w->flags, UI_WINDOW_DOCKED);

    dock_space0->window_in_focus = 1;

    UiWindow* window_to_remove = ui_window_get_(1);
    window_to_remove->position = v2i(138.64f);
    window_to_remove->size = v2i(188.94f);
    u32 actual = remove_window_from_shared_dock_space(1, window_to_remove, dock_space0);

    ASSERT_EQUALS(2, dock_space0->windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, dock_space0->windows.data[0], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(2, dock_space0->windows.data[1], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, dock_space0->window_in_focus, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, actual, EQUALS_FORMAT_U32);

    UiWindow* window_to_show =
        ui_window_get_(dock_space0->windows.data[dock_space0->window_in_focus]);

    ASSERT_EQUALS(window_to_remove->position.x, window_to_show->position.x, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(window_to_remove->position.y, window_to_show->position.y, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(window_to_remove->size.width, window_to_show->size.width, EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(window_to_remove->size.height, window_to_show->size.height, EQUALS_FORMAT_FLOAT);

    ASSERT_EQUALS(false, check_bit(window_to_show->flags, UI_WINDOW_CLOSING), EQUALS_FORMAT_U32);
    ASSERT_EQUALS(false, check_bit(window_to_show->flags, UI_WINDOW_HIDE), EQUALS_FORMAT_U32);
    ASSERT_EQUALS(true, check_bit(window_to_show->flags, UI_WINDOW_DOCKED), EQUALS_FORMAT_U32);

    ASSERT_EQUALS(true, check_bit(window_to_remove->flags, UI_WINDOW_CLOSING), EQUALS_FORMAT_U32);
    ASSERT_EQUALS(false, check_bit(window_to_remove->flags, UI_WINDOW_HIDE), EQUALS_FORMAT_U32);
    ASSERT_EQUALS(false, check_bit(window_to_remove->flags, UI_WINDOW_DOCKED), EQUALS_FORMAT_U32);
    ASSERT_TRUE(window_to_remove->dock_node != dock_space0);

    array_free(&window_to_remove->dock_node->windows);
    free(window_to_remove->dock_node);
    array_free(&dock_space0->windows);
    free(dock_space0);
    array_free(&ui_context.windows);
    ui_context.windows.capacity = 0;
    ui_context.windows.size = 0;
    array_free(&ui_context.id_to_index);
    ui_context.id_to_index.capacity = 0;
    ui_context.id_to_index.size = 0;
}

void ui_test_set_resize_cursor()
{
    u8 resize = RESIZE_NONE;
    set_resize_cursor(resize);
    ASSERT_EQUALS(FTIC_NORMAL_CURSOR, window_get_cursor(), EQUALS_FORMAT_I32);

    resize = RESIZE_RIGHT;
    set_resize_cursor(resize);
    ASSERT_EQUALS(FTIC_RESIZE_H_CURSOR, window_get_cursor(), EQUALS_FORMAT_I32);

    resize = RESIZE_LEFT;
    set_resize_cursor(resize);
    ASSERT_EQUALS(FTIC_RESIZE_H_CURSOR, window_get_cursor(), EQUALS_FORMAT_I32);

    resize = RESIZE_TOP;
    set_resize_cursor(resize);
    ASSERT_EQUALS(FTIC_RESIZE_V_CURSOR, window_get_cursor(), EQUALS_FORMAT_I32);

    resize = RESIZE_BOTTOM;
    set_resize_cursor(resize);
    ASSERT_EQUALS(FTIC_RESIZE_V_CURSOR, window_get_cursor(), EQUALS_FORMAT_I32);

    resize = RESIZE_BOTTOM | RESIZE_LEFT;
    set_resize_cursor(resize);
    ASSERT_EQUALS(FTIC_RESIZE_NESW_CURSOR, window_get_cursor(), EQUALS_FORMAT_I32);

    resize = RESIZE_BOTTOM | RESIZE_RIGHT;
    set_resize_cursor(resize);
    ASSERT_EQUALS(FTIC_RESIZE_NWSE_CURSOR, window_get_cursor(), EQUALS_FORMAT_I32);

    resize = RESIZE_TOP | RESIZE_LEFT;
    set_resize_cursor(resize);
    ASSERT_EQUALS(FTIC_RESIZE_NWSE_CURSOR, window_get_cursor(), EQUALS_FORMAT_I32);

    resize = RESIZE_TOP | RESIZE_RIGHT;
    set_resize_cursor(resize);
    ASSERT_EQUALS(FTIC_RESIZE_NESW_CURSOR, window_get_cursor(), EQUALS_FORMAT_I32);
}

void ui_test_look_for_window_resize()
{
    UiWindow window = { 0 };
    window.position = v2i(345.0f);
    window.size = v2f(200.0f, 448.8f);

    u8 actual = RESIZE_NONE;

    event_inject_mouse_position(v2i(340.0f));
    u8 resize = look_for_window_resize(&window);
    actual = resize;
    ASSERT_EQUALS(RESIZE_NONE, actual, EQUALS_FORMAT_U32);

    event_inject_mouse_position(v2f(345.0f, 365.0f));
    resize = look_for_window_resize(&window);
    actual = resize;
    ASSERT_EQUALS(RESIZE_LEFT, actual & RESIZE_LEFT, EQUALS_FORMAT_U32);
    actual ^= RESIZE_LEFT;
    ASSERT_EQUALS(RESIZE_NONE, actual, EQUALS_FORMAT_U32);

    event_inject_mouse_position(v2f(window.position.x + window.size.width, 365.0f));
    resize = look_for_window_resize(&window);
    actual = resize;
    ASSERT_EQUALS(RESIZE_RIGHT, actual & RESIZE_RIGHT, EQUALS_FORMAT_U32);
    actual ^= RESIZE_RIGHT;
    ASSERT_EQUALS(RESIZE_NONE, actual, EQUALS_FORMAT_U32);

    event_inject_mouse_position(v2f(window.position.x + (window.size.width * 0.5f), 345.0f));
    resize = look_for_window_resize(&window);
    actual = resize;
    ASSERT_EQUALS(RESIZE_TOP, actual & RESIZE_TOP, EQUALS_FORMAT_U32);
    actual ^= RESIZE_TOP;
    ASSERT_EQUALS(RESIZE_NONE, actual, EQUALS_FORMAT_U32);

    event_inject_mouse_position(v2f(window.position.x + (window.size.width * 0.5f),
                                    window.position.y + window.size.height));
    resize = look_for_window_resize(&window);
    actual = resize;
    ASSERT_EQUALS(RESIZE_BOTTOM, actual & RESIZE_BOTTOM, EQUALS_FORMAT_U32);
    actual ^= RESIZE_BOTTOM;
    ASSERT_EQUALS(RESIZE_NONE, actual, EQUALS_FORMAT_U32);

    event_inject_mouse_position(window.position);
    resize = look_for_window_resize(&window);
    actual = resize;
    ASSERT_EQUALS(RESIZE_LEFT, actual & RESIZE_LEFT, EQUALS_FORMAT_U32);
    actual ^= RESIZE_LEFT;
    ASSERT_EQUALS(RESIZE_TOP, actual & RESIZE_TOP, EQUALS_FORMAT_U32);
    actual ^= RESIZE_TOP;
    ASSERT_EQUALS(RESIZE_NONE, actual, EQUALS_FORMAT_U32);

    event_inject_mouse_position(v2_add(window.position, window.size));
    resize = look_for_window_resize(&window);
    actual = resize;
    ASSERT_EQUALS(RESIZE_RIGHT, actual & RESIZE_RIGHT, EQUALS_FORMAT_U32);
    actual ^= RESIZE_RIGHT;
    ASSERT_EQUALS(RESIZE_BOTTOM, actual & RESIZE_BOTTOM, EQUALS_FORMAT_U32);
    actual ^= RESIZE_BOTTOM;
    ASSERT_EQUALS(RESIZE_NONE, actual, EQUALS_FORMAT_U32);

    event_inject_mouse_position(v2f(window.position.x, window.position.y + window.size.height));
    resize = look_for_window_resize(&window);
    actual = resize;
    ASSERT_EQUALS(RESIZE_LEFT, actual & RESIZE_LEFT, EQUALS_FORMAT_U32);
    actual ^= RESIZE_LEFT;
    ASSERT_EQUALS(RESIZE_BOTTOM, actual & RESIZE_BOTTOM, EQUALS_FORMAT_U32);
    actual ^= RESIZE_BOTTOM;
    ASSERT_EQUALS(RESIZE_NONE, actual, EQUALS_FORMAT_U32);

    event_inject_mouse_position(v2f(window.position.x + window.size.width, window.position.y));
    resize = look_for_window_resize(&window);
    actual = resize;
    ASSERT_EQUALS(RESIZE_RIGHT, actual & RESIZE_RIGHT, EQUALS_FORMAT_U32);
    actual ^= RESIZE_RIGHT;
    ASSERT_EQUALS(RESIZE_TOP, actual & RESIZE_TOP, EQUALS_FORMAT_U32);
    actual ^= RESIZE_TOP;
    ASSERT_EQUALS(RESIZE_NONE, actual, EQUALS_FORMAT_U32);
}

void ui_test_check_window_collisions()
{
    UiWindow window = { .flags = UI_WINDOW_DOCKED };
    array_create(&ui_context.windows, 10);
    array_create(&ui_context.window_aabbs, 10);
    array_create(&ui_context.window_hover_clicked_indices, 10);
    array_create(&ui_context.id_to_index, 10);
    for (u32 i = 0; i < 10; ++i)
    {
        array_push(&ui_context.id_to_index, i);
    }

    array_push(&ui_context.windows, window);
    WindowRenderData render_data = render_data_create(&window);
    UiWindow* window_to_check = array_back(&ui_context.windows);
    MouseButtonEvent mouse_button_event = {
        .button = FTIC_MOUSE_BUTTON_LEFT,
        .action = FTIC_PRESS,
        .activated = true,
    };
    event_inject_mouse_button_event(mouse_button_event);

    AABBArray temp_aabbs = { 0 };
    array_create(&temp_aabbs, 10);

    array_push(&ui_context.window_aabbs, temp_aabbs);

    AABBArray* aabbs = array_back(&ui_context.window_aabbs);
    AABB aabb = {
        .min = window.position,
        .size = window.size,
    };
    array_push(aabbs, aabb);

    // Area is not hit
    mouse_button_event.activated = false;
    event_inject_mouse_button_event(mouse_button_event);
    event_inject_mouse_position(v2i(200.0f));
    render_data = render_data_create(window_to_check);
    b8 result = check_window_collisions(&render_data);
    ASSERT_EQUALS(false, result, EQUALS_FORMAT_U32);

    array_push(aabbs, aabb);

    V2 half_size = v2_s_multi(window.size, 0.5f);
    event_inject_mouse_position(v2_add(window.position, half_size));
    render_data = render_data_create(window_to_check);
    result = check_window_collisions(&render_data);
    ASSERT_EQUALS(true, result, EQUALS_FORMAT_U32);
    HoverClickedIndex* hover_clicked_index = ui_context.window_hover_clicked_indices.data;
    ASSERT_EQUALS(1, hover_clicked_index->index, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(true, hover_clicked_index->hover, EQUALS_FORMAT_U32);

    array_free(aabbs);
    array_free(&ui_context.windows);
    array_free(&ui_context.window_aabbs);
    array_free(&ui_context.window_hover_clicked_indices);
    array_free(&ui_context.id_to_index);
}

void ui_test_check_if_window_should_be_docked()
{
    ui_test_init_context();
    ui_context.dock_tree->aabb.size = v2f(1000.0f, 800.0f);
    ui_context.dock_side_hit = -1;

    for (u32 i = 0; i < ui_context.windows.size; ++i)
    {
        ASSERT_EQUALS(false, check_bit(ui_context.windows.data[i].flags, UI_WINDOW_DOCKED),
                      EQUALS_FORMAT_U32);
    }
    check_if_window_should_be_docked();

    ASSERT_EQUALS(NULL, ui_context.dock_tree->children[0], EQUALS_FORMAT_PTR);
    ASSERT_EQUALS(NULL, ui_context.dock_tree->children[1], EQUALS_FORMAT_PTR);

    {
        ui_context.dock_side_hit = 0;
        ui_context.dock_hit_node = ui_context.dock_tree;
        ui_context.pressed_window = ui_window_get_(0);

        check_if_window_should_be_docked();
        test_root();
        window_animate(ui_window_get_(0));

        DockNode* left = ui_context.dock_tree->children[0];
        test_docked_leaf(left, 0, 1, __FILE__, __LINE__);
    }

    {
        ui_context.dock_side_hit = 1;
        ui_context.dock_hit_node = ui_context.dock_tree;
        ui_context.pressed_window = ui_window_get_(1);

        check_if_window_should_be_docked();
        test_root();
        window_animate(ui_window_get_(0));
        window_animate(ui_window_get_(1));

        DockNode* left = ui_context.dock_tree->children[0];
        test_docked_parent(left, SPLIT_VERTICAL, __FILE__, __LINE__);

        DockNode* right = left->children[1];
        left = left->children[0];
        test_docked_leaf(left, 0, 1, __FILE__, __LINE__);
        test_docked_leaf(right, 1, 1, __FILE__, __LINE__);
    }

    {
        ui_context.dock_side_hit = 2;
        ui_context.dock_hit_node = ui_context.dock_tree;
        ui_context.pressed_window = ui_window_get_(2);

        check_if_window_should_be_docked();
        test_root();
        window_animate(ui_window_get_(0));
        window_animate(ui_window_get_(1));
        window_animate(ui_window_get_(2));

        DockNode* left = ui_context.dock_tree->children[0];
        test_docked_parent(left, SPLIT_HORIZONTAL, __FILE__, __LINE__);

        DockNode* right = left->children[1];
        left = left->children[0];

        test_docked_leaf(right, 2, 1, __FILE__, __LINE__);

        test_docked_parent(left, SPLIT_VERTICAL, __FILE__, __LINE__);
        right = left->children[1];
        left = left->children[0];
        test_docked_leaf(left, 0, 1, __FILE__, __LINE__);
        test_docked_leaf(right, 1, 1, __FILE__, __LINE__);
    }

    {
        ui_context.dock_side_hit = 3;
        ui_context.dock_hit_node = ui_context.windows.data[2].dock_node;
        ui_context.pressed_window = ui_window_get_(3);

        check_if_window_should_be_docked();
        test_root();
        window_animate(ui_window_get_(0));
        window_animate(ui_window_get_(1));
        window_animate(ui_window_get_(2));
        window_animate(ui_window_get_(3));

        DockNode* left = ui_context.dock_tree->children[0];
        test_docked_parent(left, SPLIT_HORIZONTAL, __FILE__, __LINE__);

        DockNode* right = left->children[1];
        left = left->children[0];
        test_docked_parent(left, SPLIT_VERTICAL, __FILE__, __LINE__);
        test_docked_parent(right, SPLIT_VERTICAL, __FILE__, __LINE__);

        DockNode* left_left = left->children[0];
        DockNode* left_right = left->children[1];
        test_docked_leaf(left_left, 0, 1, __FILE__, __LINE__);
        test_docked_leaf(left_right, 1, 1, __FILE__, __LINE__);

        DockNode* right_left = right->children[0];
        DockNode* right_right = right->children[1];
        test_docked_leaf(right_left, 3, 1, __FILE__, __LINE__);
        test_docked_leaf(right_right, 2, 1, __FILE__, __LINE__);
    }

    {
        ui_context.dock_side_hit = 4;
        ui_context.dock_hit_node = ui_context.windows.data[1].dock_node;
        ui_context.pressed_window = ui_window_get_(4);

        check_if_window_should_be_docked();
        window_animate(ui_window_get_(0));
        window_animate(ui_window_get_(1));
        window_animate(ui_window_get_(2));
        window_animate(ui_window_get_(3));
        window_animate(ui_window_get_(4));
        test_root();

        DockNode* left = ui_context.dock_tree->children[0];
        test_docked_parent(left, SPLIT_HORIZONTAL, __FILE__, __LINE__);

        DockNode* right = left->children[1];
        left = left->children[0];
        test_docked_parent(left, SPLIT_VERTICAL, __FILE__, __LINE__);
        test_docked_parent(right, SPLIT_VERTICAL, __FILE__, __LINE__);

        DockNode* left_left = left->children[0];
        DockNode* left_right = left->children[1];
        test_docked_leaf(left_left, 0, 1, __FILE__, __LINE__);
        test_docked_leaf(left_right, 1, 2, __FILE__, __LINE__);
        ASSERT_EQUALS(ui_context.windows.data[4].id, left_right->windows.data[1],
                      EQUALS_FORMAT_U32);
        ASSERT_EQUALS(1, left_right->window_in_focus, EQUALS_FORMAT_U32);
        ASSERT_EQUALS(true, check_bit(ui_context.windows.data[1].flags, UI_WINDOW_HIDE),
                      EQUALS_FORMAT_U32);
        ASSERT_EQUALS(false, check_bit(ui_context.windows.data[4].flags, UI_WINDOW_HIDE),
                      EQUALS_FORMAT_U32);
        ASSERT_EQUALS(ui_context.windows.data[1].position.x, ui_context.windows.data[4].position.x,
                      EQUALS_FORMAT_FLOAT);
        ASSERT_EQUALS(ui_context.windows.data[1].position.y, ui_context.windows.data[4].position.y,
                      EQUALS_FORMAT_FLOAT);
        ASSERT_EQUALS(ui_context.windows.data[1].position.width,
                      ui_context.windows.data[4].position.width, EQUALS_FORMAT_FLOAT);
        ASSERT_EQUALS(ui_context.windows.data[1].position.height,
                      ui_context.windows.data[4].position.height, EQUALS_FORMAT_FLOAT);

        ASSERT_EQUALS(ui_context.window_in_focus, 4, EQUALS_FORMAT_U32);

        ASSERT_EQUALS(ui_context.windows.data[1].dock_node, ui_context.windows.data[4].dock_node,
                      EQUALS_FORMAT_PTR);

        DockNode* right_left = right->children[0];
        DockNode* right_right = right->children[1];
        test_docked_leaf(right_left, 3, 1, __FILE__, __LINE__);
        test_docked_leaf(right_right, 2, 1, __FILE__, __LINE__);
    }

    ui_test_uninit_context();
}

void ui_test_check_dock_space_resize()
{
    ui_context.check_collisions = true;
    ui_context.dock_tree = dock_node_create(NODE_ROOT, SPLIT_NONE, -1);
    ui_context.dock_tree->aabb.size = v2f(1000.0f, 800.0f);
    ui_context.animation_off = true;
    particle_buffer_create(&ui_context.particles, 1000);

    array_create(&ui_context.id_to_index, 10);

    UiWindow window = { .flags = UI_WINDOW_HIDE };
    array_create(&ui_context.windows, 10);
    array_create(&ui_context.window_aabbs, 10);
    array_create(&ui_context.window_hover_clicked_indices, 10);
    array_create(&ui_context.render.vertices, 10);
    array_create(&ui_context.animation_x, 10);

    for (u32 i = 0; i < 10; ++i)
    {
        DockNode* node = dock_node_create(NODE_LEAF, SPLIT_NONE, i);
        insert_window(node, i, false);
        array_push(&ui_context.id_to_index, i);
    }

    MouseButtonEvent mouse_button_event = { .action = FTIC_RELEASE };

    ///////////////////////////////////

    window_set_last_cursor(FTIC_NORMAL_CURSOR);
    check_dock_space_resize();
    ASSERT_EQUALS(false, ui_context.dock_resize, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, ui_context.render.vertices.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(FTIC_NORMAL_CURSOR, window_get_cursor(), EQUALS_FORMAT_I32);

    ///////////////////////////////////

    ///////////////////////////////////

    window_set_last_cursor(FTIC_NORMAL_CURSOR);

    ui_context.dock_resize = false;
    ui_context.dock_side_hit = 0;
    ui_context.dock_hit_node = ui_context.dock_tree;
    ui_context.pressed_window = ui_window_get_(0);
    check_if_window_should_be_docked();

    check_dock_space_resize();
    ASSERT_EQUALS(false, ui_context.dock_resize, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, ui_context.render.vertices.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(FTIC_NORMAL_CURSOR, window_get_cursor(), EQUALS_FORMAT_I32);

    ///////////////////////////////////

    ///////////////////////////////////

    window_set_last_cursor(FTIC_NORMAL_CURSOR);

    ui_context.dock_resize = false;
    ui_context.dock_side_hit = 2;
    ui_context.dock_hit_node = ui_context.dock_tree;
    ui_context.pressed_window = ui_window_get_(1);
    check_if_window_should_be_docked();

    event_inject_mouse_position(v2f(800.0f, ui_context.dock_tree->aabb.size.height * 0.5f));

    check_dock_space_resize();
    ASSERT_EQUALS(false, ui_context.dock_resize, EQUALS_FORMAT_U32);
    ASSERT_NOT_EQUALS(0, ui_context.render.vertices.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(FTIC_RESIZE_V_CURSOR, window_get_cursor(), EQUALS_FORMAT_I32);

    mouse_button_event.activated = true;
    mouse_button_event.action = FTIC_PRESS;
    mouse_button_event.button = FTIC_MOUSE_BUTTON_LEFT;
    event_inject_mouse_button_event(mouse_button_event);

    check_dock_space_resize();
    ASSERT_EQUALS(true, ui_context.dock_resize, EQUALS_FORMAT_U32);
    ASSERT_NOT_EQUALS(0, ui_context.render.vertices.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(FTIC_RESIZE_V_CURSOR, window_get_cursor(), EQUALS_FORMAT_I32);

    ///////////////////////////////////

    ///////////////////////////////////

    window_set_last_cursor(FTIC_NORMAL_CURSOR);

    ui_context.dock_resize = false;
    ui_context.dock_side_hit = 3;
    ui_context.dock_hit_node = ui_context.dock_tree;
    ui_context.pressed_window = ui_window_get_(2);
    check_if_window_should_be_docked();

    event_inject_mouse_position(v2f(ui_context.dock_tree->aabb.size.width * 0.5f, 100.0f));
    mouse_button_event.action = FTIC_RELEASE;
    event_inject_mouse_button_event(mouse_button_event);

    check_dock_space_resize();
    ASSERT_EQUALS(false, ui_context.dock_resize, EQUALS_FORMAT_U32);
    ASSERT_NOT_EQUALS(0, ui_context.render.vertices.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(FTIC_RESIZE_H_CURSOR, window_get_cursor(), EQUALS_FORMAT_I32);

    mouse_button_event.activated = true;
    mouse_button_event.action = FTIC_PRESS;
    mouse_button_event.button = FTIC_MOUSE_BUTTON_LEFT;
    event_inject_mouse_button_event(mouse_button_event);

    check_dock_space_resize();
    ASSERT_EQUALS(true, ui_context.dock_resize, EQUALS_FORMAT_U32);
    ASSERT_NOT_EQUALS(0, ui_context.render.vertices.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(FTIC_RESIZE_H_CURSOR, window_get_cursor(), EQUALS_FORMAT_I32);

    ///////////////////////////////////

    for (u32 i = 0; i < 10; ++i)
    {
        array_free(&ui_context.window_aabbs.data[i]);
        if (!check_bit(ui_context.windows.data[i].flags, UI_WINDOW_DOCKED))
        {
            array_free(&ui_context.windows.data[i].dock_node->windows);
            free(ui_context.windows.data[i].dock_node);
        }
    }

    free_nodes(ui_context.dock_tree);
    array_free(&ui_context.windows);
    array_free(&ui_context.window_aabbs);
    array_free(&ui_context.window_hover_clicked_indices);
    array_free(&ui_context.render.vertices);
    array_free(&ui_context.animation_x);
    free(ui_context.particles.data);
}

void ui_test_sync_current_frame_windows()
{
    array_create(&ui_context.current_frame_windows, 10);
    array_create(&ui_context.last_frame_windows, 10);

    // No windows

    sync_current_frame_windows(&ui_context.current_frame_windows, &ui_context.last_frame_windows);
    ASSERT_EQUALS(0, ui_context.current_frame_windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, ui_context.last_frame_windows.size, EQUALS_FORMAT_U32);

    /////////////////////////////////////////////////////

    // One new window

    WindowRenderData render_data = { .id = 0 };
    array_push(&ui_context.current_frame_windows, render_data);
    sync_current_frame_windows(&ui_context.current_frame_windows, &ui_context.last_frame_windows);
    ASSERT_EQUALS(0, ui_context.current_frame_windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(1, ui_context.last_frame_windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, ui_context.last_frame_windows.data[0].id, EQUALS_FORMAT_U32);

    /////////////////////////////////////////////////////

    // Two new windows

    array_push(&ui_context.current_frame_windows, render_data);
    render_data.id = 5;
    array_push(&ui_context.current_frame_windows, render_data);
    render_data.id = 3;
    array_push(&ui_context.current_frame_windows, render_data);
    sync_current_frame_windows(&ui_context.current_frame_windows, &ui_context.last_frame_windows);
    ASSERT_EQUALS(0, ui_context.current_frame_windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(3, ui_context.last_frame_windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, ui_context.last_frame_windows.data[0].id, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(5, ui_context.last_frame_windows.data[1].id, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(3, ui_context.last_frame_windows.data[2].id, EQUALS_FORMAT_U32);

    /////////////////////////////////////////////////////

    // Last frame has windows and no new window

    render_data.id = 5;
    array_push(&ui_context.current_frame_windows, render_data);
    render_data.id = 0;
    array_push(&ui_context.current_frame_windows, render_data);
    render_data.id = 3;
    array_push(&ui_context.current_frame_windows, render_data);
    sync_current_frame_windows(&ui_context.current_frame_windows, &ui_context.last_frame_windows);
    ASSERT_EQUALS(0, ui_context.current_frame_windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(3, ui_context.last_frame_windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, ui_context.last_frame_windows.data[0].id, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(5, ui_context.last_frame_windows.data[1].id, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(3, ui_context.last_frame_windows.data[2].id, EQUALS_FORMAT_U32);

    /////////////////////////////////////////////////////

    // Last frame has windows and one window should be removed

    render_data.id = 0;
    array_push(&ui_context.current_frame_windows, render_data);
    render_data.id = 3;
    array_push(&ui_context.current_frame_windows, render_data);
    sync_current_frame_windows(&ui_context.current_frame_windows, &ui_context.last_frame_windows);
    ASSERT_EQUALS(0, ui_context.current_frame_windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(2, ui_context.last_frame_windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, ui_context.last_frame_windows.data[0].id, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(3, ui_context.last_frame_windows.data[1].id, EQUALS_FORMAT_U32);

    /////////////////////////////////////////////////////

    // Last frame has windows and two window should be removed

    render_data.id = 5;
    array_push(&ui_context.current_frame_windows, render_data);
    render_data.id = 0;
    array_push(&ui_context.current_frame_windows, render_data);
    render_data.id = 3;
    array_push(&ui_context.current_frame_windows, render_data);
    sync_current_frame_windows(&ui_context.current_frame_windows, &ui_context.last_frame_windows);
    ASSERT_EQUALS(0, ui_context.current_frame_windows.size, EQUALS_FORMAT_U32);

    render_data.id = 3;
    array_push(&ui_context.current_frame_windows, render_data);
    sync_current_frame_windows(&ui_context.current_frame_windows, &ui_context.last_frame_windows);
    ASSERT_EQUALS(0, ui_context.current_frame_windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(1, ui_context.last_frame_windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(3, ui_context.last_frame_windows.data[0].id, EQUALS_FORMAT_U32);

    /////////////////////////////////////////////////////

    // Last frame has windows and one window sould be added;

    render_data.id = 6;
    array_push(&ui_context.current_frame_windows, render_data);
    render_data.id = 3;
    array_push(&ui_context.current_frame_windows, render_data);
    sync_current_frame_windows(&ui_context.current_frame_windows, &ui_context.last_frame_windows);
    ASSERT_EQUALS(0, ui_context.current_frame_windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(2, ui_context.last_frame_windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(3, ui_context.last_frame_windows.data[0].id, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(6, ui_context.last_frame_windows.data[1].id, EQUALS_FORMAT_U32);

    /////////////////////////////////////////////////////

    // Last frame has windows and two window sould be added;

    render_data.id = 4;
    array_push(&ui_context.current_frame_windows, render_data);
    render_data.id = 6;
    array_push(&ui_context.current_frame_windows, render_data);
    render_data.id = 3;
    array_push(&ui_context.current_frame_windows, render_data);
    render_data.id = 7;
    array_push(&ui_context.current_frame_windows, render_data);
    sync_current_frame_windows(&ui_context.current_frame_windows, &ui_context.last_frame_windows);
    ASSERT_EQUALS(0, ui_context.current_frame_windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(4, ui_context.last_frame_windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(3, ui_context.last_frame_windows.data[0].id, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(6, ui_context.last_frame_windows.data[1].id, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(4, ui_context.last_frame_windows.data[2].id, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(7, ui_context.last_frame_windows.data[3].id, EQUALS_FORMAT_U32);

    /////////////////////////////////////////////////////

    array_free(&ui_context.current_frame_windows);
    array_free(&ui_context.last_frame_windows);
}


void ui_test_ui_window_close()
{
    ui_test_init_context();

    ui_window_close(0);
    const UiWindow* window = ui_window_get(0);
    ASSERT_EQUALS(true, check_bit(window->flags, UI_WINDOW_CLOSING), EQUALS_FORMAT_U32);
    ASSERT_EQUALS(false, check_bit(window->flags, UI_WINDOW_HIDE), EQUALS_FORMAT_U32);
    ASSERT_EQUALS(false, check_bit(window->flags, UI_WINDOW_DOCKED), EQUALS_FORMAT_U32);

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(0)->dock_node, SPLIT_HORIZONTAL,
                          DOCK_SIDE_TOP);
    set_bit(ui_window_get_(0)->flags, UI_WINDOW_DOCKED);

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(1)->dock_node, SPLIT_HORIZONTAL,
                          DOCK_SIDE_TOP);
    set_bit(ui_window_get_(1)->flags, UI_WINDOW_DOCKED);

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(2)->dock_node, SPLIT_HORIZONTAL,
                          DOCK_SIDE_TOP);
    set_bit(ui_window_get_(2)->flags, UI_WINDOW_DOCKED);

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(3)->dock_node, SPLIT_VERTICAL,
                          DOCK_SIDE_RIGHT);
    set_bit(ui_window_get_(3)->flags, UI_WINDOW_DOCKED);

    dock_node_dock_window(ui_window_get(1)->dock_node, ui_window_get(4)->dock_node, SPLIT_VERTICAL,
                          DOCK_SIDE_RIGHT);
    set_bit(ui_window_get_(4)->flags, UI_WINDOW_DOCKED);

    dock_node_dock_window(ui_context.dock_tree, ui_window_get(5)->dock_node, SPLIT_VERTICAL,
                          DOCK_SIDE_RIGHT);
    set_bit(ui_window_get_(5)->flags, UI_WINDOW_DOCKED);

    window_animate(ui_window_get_(0));
    window_animate(ui_window_get_(1));
    window_animate(ui_window_get_(2));
    window_animate(ui_window_get_(3));
    window_animate(ui_window_get_(4));
    window_animate(ui_window_get_(5));

    DockNode* node_before =
        find_node(ui_context.dock_tree->children[0], ui_context.dock_tree, window->dock_node);
    ASSERT_NOT_EQUALS(NULL, node_before, EQUALS_FORMAT_PTR);

    window = ui_window_get(2);
    ASSERT_EQUALS(false, check_bit(window->flags, UI_WINDOW_CLOSING), EQUALS_FORMAT_U32);
    ASSERT_EQUALS(true, check_bit(window->flags, UI_WINDOW_DOCKED), EQUALS_FORMAT_U32);
    ui_window_close(2);
    window = ui_window_get(2);
    ASSERT_EQUALS(true, check_bit(window->flags, UI_WINDOW_CLOSING), EQUALS_FORMAT_U32);
    ASSERT_EQUALS(false, check_bit(window->flags, UI_WINDOW_HIDE), EQUALS_FORMAT_U32);
    ASSERT_EQUALS(false, check_bit(window->flags, UI_WINDOW_DOCKED), EQUALS_FORMAT_U32);

    node_before =
        find_node(ui_context.dock_tree->children[0], ui_context.dock_tree, window->dock_node);
    ASSERT_EQUALS(NULL, node_before, EQUALS_FORMAT_PTR);

    window = ui_window_get(3);
    array_push(&window->dock_node->windows, 6);
    ui_window_get_(6)->dock_node = window->dock_node;
    set_bit(ui_window_get_(6)->flags, UI_WINDOW_DOCKED);

    ui_window_close(3);
    window = ui_window_get(3);

    ASSERT_EQUALS(true, check_bit(window->flags, UI_WINDOW_CLOSING), EQUALS_FORMAT_U32);
    ASSERT_EQUALS(false, check_bit(window->flags, UI_WINDOW_HIDE), EQUALS_FORMAT_U32);
    ASSERT_EQUALS(false, check_bit(window->flags, UI_WINDOW_DOCKED), EQUALS_FORMAT_U32);
    ASSERT_NOT_EQUALS(window->dock_node, ui_window_get(6)->dock_node, EQUALS_FORMAT_PTR);

    ui_test_uninit_context();
}

void ui_test_ui_window_close_current()
{
    ui_test_init_context();


    ui_test_uninit_context();
}
