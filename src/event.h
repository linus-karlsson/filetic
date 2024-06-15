#pragma once

#include "define.h"
#include "platform/platform.h"
#include "util.h"

#define FTIC_KEY_SPACE 32
#define FTIC_KEY_COMMA 44
#define FTIC_KEY_MINUS 189
#define FTIC_KEY_PERIOD 190
#define FTIC_KEY_SHIFT 16
#define FTIC_KEY_CTRL 17
#define FTIC_KEY_BACKSPACE 8
#define FTIC_KEY_TAB 9
#define FTIC_KEY_ENTER 13
#define FTIC_KEY_CAPS 20
#define FTIC_KEY_SPACE 32
#define FTIC_KEY_APOSTROPHE 191

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

#define FTIC_KEY_LEFT 37
#define FTIC_KEY_UP 38
#define FTIC_KEY_RIGHT 39
#define FTIC_KEY_DOWN 40

#define FTIC_LEFT_BUTTON 1
#define FTIC_MIDDLE_BUTTON 3
#define FTIC_RIGHT_BUTTON 2

#define FTIC_BUTTON_PRESS 1
#define FTIC_BUTTON_RELEASE 0

#define FTIC_NORMAL_CURSOR 0
#define FTIC_HAND_CURSOR 1
#define FTIC_RESIZE_H_CURSOR 2
#define FTIC_RESIZE_V_CURSOR 3
#define FTIC_RESIZE_NW_CURSOR 4
#define FTIC_MOVE_CURSOR 5
#define FTIC_HIDDEN_CURSOR 6

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
    u16 key;
    u8 action;
} KeyEvent;

typedef struct MouseMoveEvent
{
    i16 position_x;
    i16 position_y;
} MouseMoveEvent;

typedef struct MouseButtonEvent
{
    u8 key;
    b8 double_clicked;
} MouseButtonEvent;

typedef struct MouseWheelEvent
{
    i16 z_delta;
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

void event_init(Platform* platform);
void event_poll(Platform* platform);
Event* event_subscribe(EventType type);

const CharArray* event_get_char_buffer();

