#include "event.h"

typedef struct EventArray
{
    u32 size;
    u32 capacity;
    Event* data;
} EventArray;

typedef struct EventContextInternal
{
    EventArray events;
} EventContextInternal;

EventContextInternal event_context = { 0 };

internal void _on_key_event(u16 key, u8 action)
{
    for (u32 i = 0; i < event_context.events.size; ++i)
    {
        Event* event = event_context.events.data + i;
        if (event->type == KEY)
        {
            event->key_event.key = key;
            event->key_event.action = action;
            event->activated = true;
        }
    }
}

internal void on_key_pressed_event(u16 key)
{
    _on_key_event(key, 1);
}

internal void on_key_released_event(u16 key)
{
    _on_key_event(key, 0);
}

internal void on_mouse_move_event(i16 x, i16 y)
{
    for (u32 i = 0; i < event_context.events.size; ++i)
    {
        Event* event = event_context.events.data + i;
        if (event->type == MOUSE_MOVE)
        {
            event->mouse_move_event.position_x = x;
            event->mouse_move_event.position_y = y;
            event->activated = true;
        }
    }
}

void event_init(Platform* platform)
{
    platform_event_set_on_key_pressed(platform, on_key_pressed_event);
    platform_event_set_on_key_released(platform, on_key_released_event);
    platform_event_set_on_mouse_move(platform, on_mouse_move_event);

    array_create(&event_context.events, 20);
}

void poll_event(Platform* platform)
{
    for (u32 i = 0; i < event_context.events.size; ++i)
    {
        Event* event = event_context.events.data + i;
        event->activated = false;
    }
    platform_event_fire(platform);
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

