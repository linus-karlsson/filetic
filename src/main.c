#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "define.h"

LRESULT msg_handler(HWND win, UINT msg, WPARAM w_param, LPARAM l_param)
{

}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show_cmd)
{
    WNDCLASS window_class;
    HWND win;
    const u16 width = 600;
    const u16 height = 300;

    window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = msg_handler;
    window_class.hInstance = instance;
    window_class.lpszClassName = "filetic";

    return 0;
}

