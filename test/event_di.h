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
void event_inject_mouse_button_clicked(b8 mouse_button_clicked);
void event_inject_mouse_button_pressed(b8 mouse_button_pressed);
void event_inject_mouse_button_pressed_once(b8 mouse_button_pressed_once);
void event_inject_ctrl_and_key_pressed(b8 ctrl_and_key_pressed);
void event_inject_ctrl_and_key_range_pressed(b8 ctrl_and_key_range_pressed);
void event_inject_key_clicked(b8 key_clicked);
void event_inject_key_pressed(b8 key_pressed);
void event_inject_key_pressed_once(b8 key_pressed_once);
void event_inject_key_pressed_repeat(b8 key_pressed_repeat);
