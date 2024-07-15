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
    U32Array windows = {0};

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

    UiWindow window = {0};
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


    U32Array windows = {0};
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
    U32Array free_ids = {0};
    U32Array id_to_index = {0};

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
