#include "event_di.h"

global KeyEvent g_key_event = { 0 };
global MouseMoveEvent g_mouse_move_event = { 0 };
global MouseButtonEvent g_mouse_button_event = { 0 };
global MouseWheelEvent g_mouse_wheel_event = { 0 };
global V2 g_mouse_position = { 0 };
global CharArray g_key_buffer = { 0 };
global CharPtrArray g_drop_buffer = { 0 };
global b8 g_mouse_button_clicked = false;
global b8 g_mouse_button_pressed = false;
global b8 g_mouse_button_pressed_once = false;
global b8 g_ctrl_and_key_pressed = false;
global b8 g_ctrl_and_key_range_pressed = false;
global b8 g_key_clicked = false;
global b8 g_key_pressed = false;
global b8 g_key_pressed_once = false;
global b8 g_key_pressed_repeat = false;

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

void event_inject_mouse_button_clicked(b8 mouse_button_clicked)
{
    g_mouse_button_clicked = mouse_button_clicked;
}

void event_inject_mouse_button_pressed(b8 mouse_button_pressed)
{
    g_mouse_button_pressed = mouse_button_pressed;
}

void event_inject_mouse_button_pressed_once(b8 mouse_button_pressed_once)
{
    g_mouse_button_pressed_once = mouse_button_pressed_once;
}

void event_inject_ctrl_and_key_pressed(b8 ctrl_and_key_pressed)
{
    g_ctrl_and_key_pressed = ctrl_and_key_pressed;
}

void event_inject_ctrl_and_key_range_pressed(b8 ctrl_and_key_range_pressed)
{
    g_ctrl_and_key_range_pressed = ctrl_and_key_range_pressed;
}

void event_inject_key_clicked(b8 key_clicked)
{
    g_key_clicked = key_clicked;
}

void event_inject_key_pressed(b8 key_pressed)
{
    g_key_pressed = key_pressed;
}

void event_inject_key_pressed_once(b8 key_pressed_once)
{
    g_key_pressed_once = key_pressed_once;
}

void event_inject_key_pressed_repeat(b8 key_pressed_repeat)
{
    g_key_pressed_repeat = key_pressed_repeat;
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
    return g_mouse_button_clicked;
}

b8 event_is_mouse_button_pressed(i32 button)
{
    return g_mouse_button_pressed;
}

b8 event_is_mouse_button_pressed_once(i32 button)
{
    return g_mouse_button_pressed_once;
}

b8 event_is_ctrl_and_key_pressed(i32 key)
{
    return g_ctrl_and_key_pressed;
}

b8 event_is_ctrl_and_key_range_pressed(i32 key_low, i32 key_high)
{
    return g_ctrl_and_key_range_pressed;
}

b8 event_is_key_clicked(i32 key)
{
    return g_key_clicked;
}

b8 event_is_key_pressed(i32 key)
{
    return g_key_pressed;
}

b8 event_is_key_pressed_once(i32 key)
{
    return g_key_pressed_once;
}

b8 event_is_key_pressed_repeat(i32 key)
{
    return g_key_pressed_repeat;
}

