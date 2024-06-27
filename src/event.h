#pragma once

#include "define.h"
#include "ftic_window.h"
#include "util.h"

// Taken from GLFW
#define FTIC_KEY_SPACE 32
#define FTIC_KEY_APOSTROPHE 39
#define FTIC_KEY_COMMA 44
#define FTIC_KEY_MINUS 45
#define FTIC_KEY_PERIOD 46
#define FTIC_KEY_SLASH 47
#define FTIC_KEY_0 48
#define FTIC_KEY_1 49
#define FTIC_KEY_2 50
#define FTIC_KEY_3 51
#define FTIC_KEY_4 52
#define FTIC_KEY_5 53
#define FTIC_KEY_6 54
#define FTIC_KEY_7 55
#define FTIC_KEY_8 56
#define FTIC_KEY_9 57
#define FTIC_KEY_SEMICOLON 59
#define FTIC_KEY_EQUAL 61
#define FTIC_KEY_A 65
#define FTIC_KEY_B 66
#define FTIC_KEY_C 67
#define FTIC_KEY_D 68
#define FTIC_KEY_E 69
#define FTIC_KEY_F 70
#define FTIC_KEY_G 71
#define FTIC_KEY_H 72
#define FTIC_KEY_I 73
#define FTIC_KEY_J 74
#define FTIC_KEY_K 75
#define FTIC_KEY_L 76
#define FTIC_KEY_M 77
#define FTIC_KEY_N 78
#define FTIC_KEY_O 79
#define FTIC_KEY_P 80
#define FTIC_KEY_Q 81
#define FTIC_KEY_R 82
#define FTIC_KEY_S 83
#define FTIC_KEY_T 84
#define FTIC_KEY_U 85
#define FTIC_KEY_V 86
#define FTIC_KEY_W 87
#define FTIC_KEY_X 88
#define FTIC_KEY_Y 89
#define FTIC_KEY_Z 90
#define FTIC_KEY_LEFT_BRACKET 91
#define FTIC_KEY_BACKSLASH 92
#define FTIC_KEY_RIGHT_BRACKET 93
#define FTIC_KEY_GRAVE_ACCENT 96
#define FTIC_KEY_WORLD_1 161
#define FTIC_KEY_WORLD_2 162

#define FTIC_KEY_ESCAPE 256
#define FTIC_KEY_ENTER 257
#define FTIC_KEY_TAB 258
#define FTIC_KEY_BACKSPACE 259
#define FTIC_KEY_INSERT 260
#define FTIC_KEY_DELETE 261
#define FTIC_KEY_RIGHT 262
#define FTIC_KEY_LEFT 263
#define FTIC_KEY_DOWN 264
#define FTIC_KEY_UP 265
#define FTIC_KEY_PAGE_UP 266
#define FTIC_KEY_PAGE_DOWN 267
#define FTIC_KEY_HOME 268
#define FTIC_KEY_END 269
#define FTIC_KEY_CAPS_LOCK 280
#define FTIC_KEY_SCROLL_LOCK 281
#define FTIC_KEY_NUM_LOCK 282
#define FTIC_KEY_PRINT_SCREEN 283
#define FTIC_KEY_PAUSE 284
#define FTIC_KEY_F1 290
#define FTIC_KEY_F2 291
#define FTIC_KEY_F3 292
#define FTIC_KEY_F4 293
#define FTIC_KEY_F5 294
#define FTIC_KEY_F6 295
#define FTIC_KEY_F7 296
#define FTIC_KEY_F8 297
#define FTIC_KEY_F9 298
#define FTIC_KEY_F10 299
#define FTIC_KEY_F11 300
#define FTIC_KEY_F12 301
#define FTIC_KEY_F13 302
#define FTIC_KEY_F14 303
#define FTIC_KEY_F15 304
#define FTIC_KEY_F16 305
#define FTIC_KEY_F17 306
#define FTIC_KEY_F18 307
#define FTIC_KEY_F19 308
#define FTIC_KEY_F20 309
#define FTIC_KEY_F21 310
#define FTIC_KEY_F22 311
#define FTIC_KEY_F23 312
#define FTIC_KEY_F24 313
#define FTIC_KEY_F25 314
#define FTIC_KEY_KP_0 320
#define FTIC_KEY_KP_1 321
#define FTIC_KEY_KP_2 322
#define FTIC_KEY_KP_3 323
#define FTIC_KEY_KP_4 324
#define FTIC_KEY_KP_5 325
#define FTIC_KEY_KP_6 326
#define FTIC_KEY_KP_7 327
#define FTIC_KEY_KP_8 328
#define FTIC_KEY_KP_9 329
#define FTIC_KEY_KP_DECIMAL 330
#define FTIC_KEY_KP_DIVIDE 331
#define FTIC_KEY_KP_MULTIPLY 332
#define FTIC_KEY_KP_SUBTRACT 333
#define FTIC_KEY_KP_ADD 334
#define FTIC_KEY_KP_ENTER 335
#define FTIC_KEY_KP_EQUAL 336
#define FTIC_KEY_LEFT_SHIFT 340
#define FTIC_KEY_LEFT_CONTROL 341
#define FTIC_KEY_LEFT_ALT 342
#define FTIC_KEY_LEFT_SUPER 343
#define FTIC_KEY_RIGHT_SHIFT 344
#define FTIC_KEY_RIGHT_CONTROL 345
#define FTIC_KEY_RIGHT_ALT 346
#define FTIC_KEY_RIGHT_SUPER 347
#define FTIC_KEY_MENU 348

#define FTIC_KEY_LAST GLFW_KEY_MENU

#define FTIC_MOUSE_BUTTON_1 0
#define FTIC_MOUSE_BUTTON_2 1
#define FTIC_MOUSE_BUTTON_3 2
#define FTIC_MOUSE_BUTTON_4 3
#define FTIC_MOUSE_BUTTON_5 4
#define FTIC_MOUSE_BUTTON_6 5
#define FTIC_MOUSE_BUTTON_7 6
#define FTIC_MOUSE_BUTTON_8 7
#define FTIC_MOUSE_BUTTON_LAST FTIC_MOUSE_BUTTON_8
#define FTIC_MOUSE_BUTTON_LEFT FTIC_MOUSE_BUTTON_1
#define FTIC_MOUSE_BUTTON_RIGHT FTIC_MOUSE_BUTTON_2
#define FTIC_MOUSE_BUTTON_MIDDLE FTIC_MOUSE_BUTTON_3

#define FTIC_RELEASE 0
#define FTIC_PRESS 1
#define FTIC_REPEAT 2

#define KEY_BUFFER_CAPACITY 50

typedef enum EventType
{
    KEY,
    MOUSE_MOVE,
    MOUSE_BUTTON,
    MOUSE_WHEEL,
} EventType;

typedef struct KeyEvent
{
    i32 key;
    i32 action;
    b8 ctrl_pressed;
    b8 alt_pressed;
    b8 shift_pressed;
} KeyEvent;

typedef struct MouseMoveEvent
{
    f32 position_x;
    f32 position_y;
} MouseMoveEvent;

typedef struct MouseButtonEvent
{
    int button;
    int action;
    b8 double_clicked;
} MouseButtonEvent;

typedef struct MouseWheelEvent
{
    f32 x_offset;
    f32 y_offset;
} MouseWheelEvent;

typedef struct Event
{
    EventType type;
    b8 activated;
    union
    {
        KeyEvent key_event;
        MouseMoveEvent mouse_move_event;
        MouseButtonEvent mouse_button_event;
        MouseWheelEvent mouse_wheel_event;
    };
} Event;

void event_init(FTicWindow* window);
void event_poll();
Event* event_subscribe(EventType type);

const CharArray* event_get_key_buffer();
const CharPtrArray* event_get_drop_buffer();
