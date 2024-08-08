#include "event.h"
#include <string.h>
#include <stdio.h>
#include "logging.h"

#define FTIC_MOD_SHIFT 0x0001
#define FTIC_MOD_CONTROL 0x0002
#define FTIC_MOD_ALT 0x0004
#define FTIC_MOD_SUPER 0x0008
#define FTIC_MOD_CAPS_LOCK 0x0010
#define FTIC_MOD_NUM_LOCK 0x0020

#define DOUBLE_CLICK_THRESHOLD 0.3

typedef struct EventContext
{
    CharArray key_buffer;
    CharPtrArray drop_paths;

    KeyEvent key_event;
    MouseMoveEvent mouse_move_event;
    MouseButtonEvent mouse_button_event;
    MouseWheelEvent mouse_wheel_event;

    V2 position;

    double last_click_time;
    int last_button;
    V2 last_position;

    f64 last_event_time;

    b8 key_pressed[FTIC_KEY_LAST];

} EventContext;

EventContext event_context = { .last_button = -1 };

internal void on_key_event(void* window, int key, int scancode, int action,
                           int mods)
{
    event_context.key_event.key = key;
    event_context.key_event.ctrl_pressed = mods & FTIC_MOD_CONTROL;
    event_context.key_event.alt_pressed = mods & FTIC_MOD_ALT;
    event_context.key_event.shift_pressed = mods & FTIC_MOD_SHIFT;
    event_context.key_event.action = action;
    event_context.key_event.activated = true;

    if (action == FTIC_RELEASE)
    {
        event_context.key_pressed[key] = false;
    }
    else
    {
        event_context.key_pressed[key] = true;
    }

    event_context.last_event_time = window_get_time();
}

internal void on_mouse_move_event(void* window, double x_pos, double y_pos)
{
    event_context.mouse_move_event.position_x = (f32)x_pos;
    event_context.mouse_move_event.position_y = (f32)y_pos;
    event_context.mouse_move_event.activated = true;

    event_context.last_event_time = window_get_time();
}

internal void on_mouse_button_event(void* window, int button, int action,
                                    int mods)
{
    const f64 time = window_get_time();
    const f64 time_since_last = time - event_context.last_click_time;
    const b8 double_clicked =
        (action == FTIC_RELEASE) && (button == event_context.last_button) &&
        (time_since_last <= DOUBLE_CLICK_THRESHOLD) &&
        v2_equal(event_context.last_position, event_context.position);

    event_context.mouse_button_event.button = button;
    event_context.mouse_button_event.action = action;
    event_context.mouse_button_event.double_clicked = double_clicked;
    event_context.mouse_button_event.activated = true;

    if (action == FTIC_RELEASE)
    {
        event_context.last_click_time = time;
        event_context.last_button = button;
        event_context.last_position = event_context.position;
    }
    event_context.last_event_time = time;
}

internal void on_mouse_wheel_event(void* window, double x_offset,
                                   double y_offset)
{
    event_context.mouse_wheel_event.x_offset = (f32)x_offset;
    event_context.mouse_wheel_event.y_offset = (f32)y_offset;
    event_context.mouse_wheel_event.activated = true;

    event_context.last_event_time = window_get_time();
}

internal void on_key_stroke_event(void* window, unsigned int codepoint)
{
    array_push(&event_context.key_buffer, (char)codepoint);
}

internal void on_drop_event(void* window, int count, const char** paths)
{
    for (u32 i = 0; i < (u32)count; ++i)
    {
        array_push(&event_context.drop_paths,
                   string_copy(paths[i], (u32)strlen(paths[i]), 3));
    }
}

void event_initialize(FTicWindow* window)
{
    array_create(&event_context.key_buffer, 20);
    array_create(&event_context.drop_paths, 20);

    window_set_on_key_event(window, on_key_event);
    window_set_on_button_event(window, on_mouse_button_event);
    window_set_on_mouse_move_event(window, on_mouse_move_event);
    window_set_on_mouse_wheel_event(window, on_mouse_wheel_event);
    window_set_on_key_stroke_event(window, on_key_stroke_event);
    window_set_on_drop_event(window, on_drop_event);

    event_context.last_event_time = window_get_time();
}

void event_uninitialize()
{
    free(event_context.key_buffer.data);
    free(event_context.drop_paths.data);
}

void event_poll(V2 mouse_position)
{
    event_context.key_event.activated = false;
    event_context.mouse_move_event.activated = false;
    event_context.mouse_button_event.activated = false;
    event_context.mouse_wheel_event.activated = false;

    event_context.mouse_button_event.double_clicked = false;

    event_context.position = mouse_position;

    if (event_context.key_buffer.size)
    {
        memset(event_context.key_buffer.data, 0, event_context.key_buffer.size);
        event_context.key_buffer.size = 0;
    }

    for (u32 i = 0; i < event_context.drop_paths.size; ++i)
    {
        free(event_context.drop_paths.data[i]);
    }
    event_context.drop_paths.size = 0;

    /*
     * NOTE: after 8 seconds of no events the main thread will go to sleep.
     */
#if 0
    if (window_get_time() - event_context.last_event_time >= 8.0f)
    {
        window_wait_event();
    }
    else
    {
        window_poll_event();
    }
#else
    window_poll_event();
#endif
}

void event_update_position(V2 mouse_position)
{
    event_context.position = mouse_position;
}

const KeyEvent* event_get_key_event()
{
    return &event_context.key_event;
}

const MouseMoveEvent* event_get_mouse_move_event()
{
    return &event_context.mouse_move_event;
}

const MouseButtonEvent* event_get_mouse_button_event()
{
    return &event_context.mouse_button_event;
}

const MouseWheelEvent* event_get_mouse_wheel_event()
{
    return &event_context.mouse_wheel_event;
}

V2 event_get_mouse_position()
{
    return event_context.position;
          
}

const CharArray* event_get_key_buffer()
{
    return &event_context.key_buffer;
}

const CharPtrArray* event_get_drop_buffer()
{
    return &event_context.drop_paths;
}

b8 event_is_mouse_button_clicked(i32 button)
{
    const MouseButtonEvent* event = event_get_mouse_button_event();
    return event->activated && event->action == FTIC_RELEASE &&
           event->button == button;
}

b8 event_is_mouse_button_pressed_once(i32 button)
{
    const MouseButtonEvent* event = event_get_mouse_button_event();
    return event->activated && event->action == FTIC_PRESS &&
           event->button == button;
}

b8 event_is_mouse_button_pressed(i32 button)
{
    const MouseButtonEvent* event = event_get_mouse_button_event();
    return event->action == FTIC_PRESS && event->button == button;
}

b8 event_is_ctrl_and_key_pressed(i32 key)
{
    const KeyEvent* event = event_get_key_event();
    return event->activated && event->action == 1 && event->ctrl_pressed &&
           event->key == key;
}

b8 event_is_ctrl_and_key_range_pressed(i32 key_low, i32 key_high)
{
    const KeyEvent* event = event_get_key_event();
    return event->activated && event->action == 1 && event->ctrl_pressed &&
           (closed_interval(key_low, event->key, key_high));
}

b8 event_is_key_clicked(i32 key)
{
    const KeyEvent* event = event_get_key_event();
    return event->activated && event->action == FTIC_RELEASE &&
           event->key == key;
}

b8 event_is_key_pressed(i32 key)
{
    return event_context.key_pressed[key];
}

b8 event_is_key_pressed_once(i32 key)
{
    const KeyEvent* event = event_get_key_event();
    return event->activated && event->action == FTIC_PRESS && event->key == key;
}

b8 event_is_key_pressed_repeat(i32 key)
{
    const KeyEvent* event = event_get_key_event();
    return event->activated &&
           (event->action == FTIC_PRESS || event->action == FTIC_REPEAT) &&
           event->key == key;
}

