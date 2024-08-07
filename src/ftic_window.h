#pragma once

#define FTIC_NORMAL_CURSOR 0
#define FTIC_HAND_CURSOR 1
#define FTIC_RESIZE_H_CURSOR 2
#define FTIC_RESIZE_V_CURSOR 3
#define FTIC_RESIZE_NWSE_CURSOR 4
#define FTIC_RESIZE_NESW_CURSOR 5
#define FTIC_MOVE_CURSOR 6

#define TOTAL_CURSORS 7

#define FTIC_MODE_CURSOR 0x00033001
#define FTIC_MODE_CURSOR_NORMAL 0x00034001
#define FTIC_MODE_CURSOR_HIDDEN 0x00034002
#define FTIC_MODE_CURSOR_DISABLED 0x00034003
#define FTIC_MODE_CURSOR_CAPTURED 0x00034004


typedef void FTicWindow;
typedef void (*OnKeyCallback)(void* window, int key, int scancode, int action, int mods);
typedef void (*OnButtonCallback)(void* window, int button, int action, int mods);
typedef void (*OnMouseMovedCallback)(void* window, double x_pos, double y_pos);
typedef void (*OnMouseWheelCallback)(void* window, double x_offset, double y_offset);
typedef void (*OnKeyStrokeCallback)(void* window, unsigned int codepoint);
typedef void (*OnDropCallback)(void* window, int count, const char** paths);

FTicWindow* window_create(const char* title, int width, int height);
FTicWindow* window_get_current();
void window_destroy(FTicWindow* window);
void window_set_on_key_event(FTicWindow* window, OnKeyCallback callback);
void window_set_on_button_event(FTicWindow* window, OnButtonCallback callback);
void window_set_on_mouse_move_event(FTicWindow* window, OnMouseMovedCallback callback);
void window_set_on_mouse_wheel_event(FTicWindow* window, OnMouseWheelCallback callback);
void window_set_on_key_stroke_event(FTicWindow* window, OnKeyStrokeCallback callback);
void window_set_on_drop_event(FTicWindow* window, OnDropCallback callback);

void window_get_mouse_position(FTicWindow* window, double* x, double* y);

void window_set_cursor(FTicWindow* window, int cursor);
void window_set_last_cursor(int cursor);
int window_get_cursor();
void window_set_cursor_position(FTicWindow* window, double x, double y);
void window_set_input_mode(FTicWindow* window, int mode, int value);

int window_should_close(FTicWindow* window);
void window_wait_event();
void window_poll_event();
void window_swap(FTicWindow* window);
void window_get_size(FTicWindow* window, int* width, int* height);
double window_get_time();

void window_set_clipboard(const char* text);
const char* window_get_clipboard();
