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

void ui_test_push_window_to_first_docked()
{
    array_create(&ui_context.windows, 10);
    array_create(&ui_context.id_to_index, 10);

    UiWindow window = { 0 };
    array_push(&ui_context.windows, window);
    array_push(&ui_context.windows, window);
    array_push(&ui_context.windows, window);
    array_push(&ui_context.windows, window);

    window.docked = true;
    array_push(&ui_context.windows, window);
    array_push(&ui_context.windows, window);

    array_push(&ui_context.id_to_index, 0);
    array_push(&ui_context.id_to_index, 1);
    array_push(&ui_context.id_to_index, 2);
    array_push(&ui_context.id_to_index, 3);
    array_push(&ui_context.id_to_index, 4);
    array_push(&ui_context.id_to_index, 5);

    U32Array windows = { 0 };
    // Array is empty
    array_create(&windows, 10);
    push_window_to_first_docked(&windows, 0);
    ASSERT_EQUALS(0, windows.size, EQUALS_FORMAT_U32);
    array_free(&windows);

    // Value already at first docked
    array_create(&windows, 10);
    array_push(&windows, 4);
    array_push(&windows, 5);
    array_push(&windows, 0);
    array_push(&windows, 1);
    array_push(&windows, 2);
    push_window_to_first_docked(&windows, 1);
    ASSERT_EQUALS(4, windows.data[0], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(5, windows.data[1], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, windows.data[2], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(1, windows.data[3], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(2, windows.data[4], EQUALS_FORMAT_U32);
    array_free(&windows);

    // Value at the back
    array_create(&windows, 10);
    array_push(&windows, 5);
    array_push(&windows, 0);
    array_push(&windows, 1);
    array_push(&windows, 2);
    array_push(&windows, 4);
    push_window_to_first_docked(&windows, 4);
    ASSERT_EQUALS(5, windows.data[0], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(4, windows.data[1], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, windows.data[2], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(1, windows.data[3], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(2, windows.data[4], EQUALS_FORMAT_U32);
    array_free(&windows);

    // Value in the middle
    array_create(&windows, 10);
    array_push(&windows, 4);
    array_push(&windows, 0);
    array_push(&windows, 1);
    array_push(&windows, 5);
    array_push(&windows, 2);
    array_push(&windows, 3);
    push_window_to_first_docked(&windows, 3);
    ASSERT_EQUALS(4, windows.data[0], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(5, windows.data[1], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, windows.data[2], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(1, windows.data[3], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(2, windows.data[4], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(3, windows.data[5], EQUALS_FORMAT_U32);
    array_free(&windows);

    // Single element array
    array_create(&windows, 10);
    array_push(&windows, 1);
    push_window_to_first_docked(&windows, 0);
    ASSERT_EQUALS(1, windows.data[0], EQUALS_FORMAT_U32);
    array_free(&windows);

    // Index out of bounds
    array_create(&windows, 10);
    array_push(&windows, 1);
    array_push(&windows, 2);
    array_push(&windows, 3);
    push_window_to_first_docked(&windows, 4);
    ASSERT_EQUALS(1, windows.data[0], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(2, windows.data[1], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(3, windows.data[2], EQUALS_FORMAT_U32);
    array_free(&windows);

    array_free(&ui_context.windows);
    ui_context.windows.capacity = 0;
    ui_context.windows.size = 0;
    array_free(&ui_context.id_to_index);
    ui_context.id_to_index.capacity = 0;
    ui_context.id_to_index.size = 0;
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

void ui_test_get_window_index()
{
    array_create(&ui_context.id_to_index, 10);
    array_create(&ui_context.last_frame_windows, 10);

    array_push(&ui_context.id_to_index, 23);
    array_push(&ui_context.last_frame_windows, 0);

    u32 actual = get_window_index(0);

    ASSERT_EQUALS(23, actual, EQUALS_FORMAT_U32);

    array_free(&ui_context.id_to_index);
    ui_context.id_to_index.capacity = 0;
    ui_context.id_to_index.size = 0;
    array_free(&ui_context.last_frame_windows);
    ui_context.last_frame_windows.capacity = 0;
    ui_context.last_frame_windows.size = 0;
}

void ui_test_set_docking()
{
    event_inject_mouse_position(v2f(135.0f, 148.0f));

    array_create(&ui_context.render.vertices, 10);

    u32 index_count0 = 0;
    b8 result = set_docking(v2f(100.0f, 100.0f), v2f(45.0f, 59.0f), v2d(),
                            v2d(), &index_count0);
    ASSERT_EQUALS(true, result, EQUALS_FORMAT_U32);

    u32 index_count1 = 0;
    result = set_docking(v2f(100.0f, 100.0f), v2f(20.0f, 20.0f), v2d(), v2d(),
                         &index_count1);
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

    const V2 middle_top_bottom = v2f(
        aabb.min.x + middle(aabb.size.width, docking_size_top_bottom.width),
        aabb.min.y + middle(aabb.size.height, docking_size_top_bottom.height));

    const V2 middle_left_right = v2f(
        aabb.min.x + middle(aabb.size.width, docking_size_left_right.width),
        aabb.min.y + middle(aabb.size.height, docking_size_left_right.height));

    const V2 top_position = v2f(middle_top_bottom.x, aabb.min.y + 10.0f);

    const V2 right_position =
        v2f(aabb.size.width - (docking_size_left_right.width + 10.0f),
            middle_left_right.y);

    const V2 bottom_position =
        v2f(middle_top_bottom.x, (aabb.min.y + aabb.size.height) -
                                     (docking_size_top_bottom.height + 10.0f));

    const V2 left_position = v2f(aabb.min.x + 10.0f, middle_left_right.y);

    const V2 docking_size_middle = v2i(50.0f);
    const f32 x = middle((right_position.x + docking_size_left_right.width) -
                             left_position.x,
                         docking_size_middle.width);
    const f32 y = middle((bottom_position.y + docking_size_top_bottom.height) -
                             top_position.y,
                         docking_size_middle.height);

    const V2 middle_position = v2f(left_position.x + x, top_position.y + y);

    // None
    event_inject_mouse_position(v2i(0.0f));
    i32 actual = display_docking(&aabb, top_position, right_position,
                                 bottom_position, left_position, true);
    ASSERT_EQUALS(-1, actual, EQUALS_FORMAT_I32);

    // Top
    event_inject_mouse_position(v2_s_add(top_position, 5.0f));
    actual = display_docking(&aabb, top_position, right_position,
                             bottom_position, left_position, true);
    ASSERT_EQUALS(0, actual, EQUALS_FORMAT_I32);

    // Right
    event_inject_mouse_position(v2_s_add(right_position, 5.0f));
    actual = display_docking(&aabb, top_position, right_position,
                             bottom_position, left_position, true);
    ASSERT_EQUALS(1, actual, EQUALS_FORMAT_I32);

    // Bottom
    event_inject_mouse_position(v2_s_add(bottom_position, 5.0f));
    actual = display_docking(&aabb, top_position, right_position,
                             bottom_position, left_position, true);
    ASSERT_EQUALS(2, actual, EQUALS_FORMAT_I32);

    // Left
    event_inject_mouse_position(v2_s_add(left_position, 5.0f));
    actual = display_docking(&aabb, top_position, right_position,
                             bottom_position, left_position, true);
    ASSERT_EQUALS(3, actual, EQUALS_FORMAT_I32);

    // Middle
    event_inject_mouse_position(v2_s_add(middle_position, 5.0f));
    actual = display_docking(&aabb, top_position, right_position,
                             bottom_position, left_position, false);
    ASSERT_EQUALS(-1, actual, EQUALS_FORMAT_I32);

    actual = display_docking(&aabb, top_position, right_position,
                             bottom_position, left_position, true);
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
    UiWindow* w = ui_window_get(0);
    w->dock_node = dock_space0;
    w->docked = true;
    array_push(&dock_space0->windows, 1);
    w = ui_window_get(1);
    w->dock_node = dock_space0;
    w->docked = true;
    array_push(&dock_space0->windows, 2);
    w = ui_window_get(2);
    w->dock_node = dock_space0;
    w->docked = true;

    dock_space0->window_in_focus = 1;

    UiWindow* window_to_remove = ui_window_get(1);
    window_to_remove->position = v2i(138.64f);
    window_to_remove->size = v2i(188.94f);
    u32 actual =
        remove_window_from_shared_dock_space(1, window_to_remove, dock_space0);

    ASSERT_EQUALS(2, dock_space0->windows.size, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, dock_space0->windows.data[0], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(2, dock_space0->windows.data[1], EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, dock_space0->window_in_focus, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(0, actual, EQUALS_FORMAT_U32);

    UiWindow* window_to_show =
        ui_window_get(dock_space0->windows.data[dock_space0->window_in_focus]);

    ASSERT_EQUALS(window_to_remove->position.x, window_to_show->position.x,
                  EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(window_to_remove->position.y, window_to_show->position.y,
                  EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(window_to_remove->size.width, window_to_show->size.width,
                  EQUALS_FORMAT_FLOAT);
    ASSERT_EQUALS(window_to_remove->size.height, window_to_show->size.height,
                  EQUALS_FORMAT_FLOAT);

    ASSERT_EQUALS(false, window_to_show->closing, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(false, window_to_show->hide, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(true, window_to_show->docked, EQUALS_FORMAT_U32);

    ASSERT_EQUALS(true, window_to_remove->closing, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(false, window_to_remove->hide, EQUALS_FORMAT_U32);
    ASSERT_EQUALS(false, window_to_remove->docked, EQUALS_FORMAT_U32);
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

void ui_test_look_for_window_resize();
