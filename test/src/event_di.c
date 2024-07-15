#include "event_di.h"

global KeyEvent g_key_event = { 0 };
global MouseMoveEvent g_mouse_move_event = { 0 };
global MouseButtonEvent g_mouse_button_event = { 0 };
global MouseWheelEvent g_mouse_wheel_event = { 0 };
global V2 g_mouse_position = { 0 };
global CharArray g_key_buffer = { 0 };
global CharPtrArray g_drop_buffer = { 0 };

void event_inject_key_event(KeyEvent key_event)
{
    g_key_event = key_event;
}

void event_inject_mouse_move_event(MouseMoveEvent mouse_move_event)
{
    g_mouse_move_event = mouse_move_event;
}

void event_inject_mouse_button_event(MouseButtonEvent mouse_button_event)
{
    g_mouse_button_event = mouse_button_event;
}

void event_inject_mouse_wheel_event(MouseWheelEvent mouse_wheel_event)
{
    g_mouse_wheel_event = mouse_wheel_event;
}

void event_inject_mouse_position(V2 mouse_position)
{
    g_mouse_position = mouse_position;
}

void event_inject_key_buffer(CharArray key_buffer)
{
    if (g_key_buffer.data)
    {
        array_free(&g_key_buffer);
    }
    g_key_buffer = key_buffer;
}

void event_inject_drop_buffer(CharPtrArray drop_buffer)
{
    if (g_drop_buffer.data)
    {
        for (u32 i = 0; i < g_drop_buffer.size; ++i)
        {
            free(g_drop_buffer.data[i]);
        }
        array_free(&g_drop_buffer);
    }
    g_drop_buffer = drop_buffer;
}

const KeyEvent* event_get_key_event()
{
    return &g_key_event;
}

const MouseMoveEvent* event_get_mouse_move_event()
{
    return &g_mouse_move_event;
}

const MouseButtonEvent* event_get_mouse_button_event()
{
    return &g_mouse_button_event;
}

const MouseWheelEvent* event_get_mouse_wheel_event()
{
    return &g_mouse_wheel_event;
}

V2 event_get_mouse_position()
{
    return g_mouse_position;
}

const CharArray* event_get_key_buffer()
{
    return &g_key_buffer;
}

const CharPtrArray* event_get_drop_buffer()
{
    return &g_drop_buffer;
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
    return true; 
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

