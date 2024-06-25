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

typedef struct EventArray
{
    u32 size;
    u32 capacity;
    Event* data;
} EventArray;

typedef struct EventContextInternal
{
    EventArray events;

    CharArray key_buffer;

    V2 position;

    double last_click_time;
    int last_button;
    V2 last_position;

} EventContextInternal;

EventContextInternal event_context = { .last_button = -1 };

internal void on_key_event(void* window, int key, int scancode, int action,
                           int mods)
{
    for (u32 i = 0; i < event_context.events.size; ++i)
    {
        Event* event = event_context.events.data + i;
        if (event->type == KEY)
        {
            event->key_event.key = key;
            event->key_event.ctrl_pressed = mods & FTIC_MOD_CONTROL;
            event->key_event.alt_pressed = mods & FTIC_MOD_ALT;
            event->key_event.shift_pressed = mods & FTIC_MOD_SHIFT;
            event->key_event.action = action;
            event->activated = true;
        }
    }
}

internal void on_mouse_move_event(void* window, double x_pos, double y_pos)
{
    event_context.position = v2f((f32)x_pos, (f32)y_pos);
    for (u32 i = 0; i < event_context.events.size; ++i)
    {
        Event* event = event_context.events.data + i;
        if (event->type == MOUSE_MOVE)
        {
            event->mouse_move_event.position_x = (f32)x_pos;
            event->mouse_move_event.position_y = (f32)y_pos;
            event->activated = true;
        }
    }
}

internal void on_mouse_button_event(void* window, int button, int action,
                                    int mods)
{
    const f64 time = window_get_time();
    const f64 time_since_last = time - event_context.last_click_time;
    const b8 double_clicked =
        (action == FTIC_PRESS) && (button == event_context.last_button) &&
        (time_since_last <= DOUBLE_CLICK_THRESHOLD) &&
        v2_equal(event_context.last_position, event_context.position);
    for (u32 i = 0; i < event_context.events.size; ++i)
    {
        Event* event = event_context.events.data + i;
        if (event->type == MOUSE_BUTTON)
        {
            event->mouse_button_event.button = button;
            event->mouse_button_event.action = action;
            event->mouse_button_event.double_clicked = double_clicked;
            event->activated = true;
        }
    }
    event_context.last_click_time = time;
    event_context.last_button = button;
    event_context.last_position = event_context.position;
}

internal void on_mouse_wheel_event(void* window, double x_offset,
                                   double y_offset)
{
    for (u32 i = 0; i < event_context.events.size; ++i)
    {
        Event* event = event_context.events.data + i;
        if (event->type == MOUSE_WHEEL)
        {
            event->mouse_wheel_event.x_offset = (f32)x_offset;
            event->mouse_wheel_event.y_offset = (f32)y_offset;
            event->activated = true;
        }
    }
}

internal void on_key_stroke_event(void* window, unsigned int codepoint)
{
    array_push(&event_context.key_buffer, (char)codepoint);
}

void event_init(FTicWindow* window)
{
    array_create(&event_context.events, 20);
    array_create(&event_context.key_buffer, 20);

    window_set_on_key_event(window, on_key_event);
    window_set_on_button_event(window, on_mouse_button_event);
    window_set_on_mouse_move_event(window, on_mouse_move_event);
    window_set_on_mouse_wheel_event(window, on_mouse_wheel_event);
    window_set_on_key_stroke_event(window, on_key_stroke_event);
}

void event_poll()
{
    for (u32 i = 0; i < event_context.events.size; ++i)
    {
        Event* event = event_context.events.data + i;
        event->activated = false;
        if (event->type == MOUSE_BUTTON)
        {
            event->mouse_button_event.double_clicked = false;
        }
    }
    if (event_context.key_buffer.size)
    {
        memset(event_context.key_buffer.data, 0, event_context.key_buffer.size);
        event_context.key_buffer.size = 0;
    }

    window_poll_event();
}

Event* event_subscribe(EventType type)
{
    Event event = { .type = type };

    array_push(&event_context.events, event);
    return array_back(&event_context.events);
}

void event_unsubscribe(Event* event)
{
}

const CharArray* event_get_key_buffer()
{
    return &event_context.key_buffer;
}

