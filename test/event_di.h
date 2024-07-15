#pragma once
#include "define.h"
#include "event.h"

void event_inject_key_event(KeyEvent key_event);
void event_inject_mouse_move_event(MouseMoveEvent mouse_move_event);
void event_inject_mouse_button_event(MouseButtonEvent mouse_button_event);
void event_inject_mouse_wheel_event(MouseWheelEvent mouse_wheel_event);
void event_inject_mouse_position(V2 mouse_position);
void event_inject_key_buffer(CharArray key_buffer);
void event_inject_drop_buffer(CharPtrArray drop_buffer);
