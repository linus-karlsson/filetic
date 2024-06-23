#pragma once

typedef void FTicWindow;
typedef void (*OnKeyCallback)(void* window, int key, int scancode, int action, int mods);
typedef void (*OnButtonCallback)(void* window, int button, int action, int mods);
typedef void (*OnMouseMovedCallback)(void* window, double x_pos, double y_pos);
typedef void (*OnMouseWheelCallback)(void* window, double x_offset, double y_offset);
typedef void (*OnKeyStrokeCallback)(void* window, unsigned int codepoint);

FTicWindow* window_init(const char* title, int width, int height);
void window_set_on_key_event(FTicWindow* window, OnKeyCallback callback);
void window_set_on_button_event(FTicWindow* window, OnButtonCallback callback);
void window_set_on_mouse_move_event(FTicWindow* window, OnMouseMovedCallback callback);
void window_set_on_mouse_wheel_event(FTicWindow* window, OnMouseWheelCallback callback);
void window_set_on_key_stroke_event(FTicWindow* window, OnKeyStrokeCallback callback);

void window_get_mouse_position(FTicWindow* window, double* x, double* y);

int window_should_close(FTicWindow* window);
void window_poll_event();
void window_swap(FTicWindow* window);
void window_get_size(FTicWindow* window, int* width, int* height);
double window_get_time();
