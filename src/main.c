#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "define.h"

global b8 running = false;

LRESULT msg_handler(HWND win, UINT msg, WPARAM w_param, LPARAM l_param)
{
    LRESULT result = 0;
    switch (msg)
    {
        case WM_DESTROY:
        case WM_QUIT:
        {
            running = false;
            break;
        }
        default:
        {
            result = DefWindowProc(win, msg, w_param, l_param);
            break;
        }
    }
    return result;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line,
                   int show_cmd)
{
    WNDCLASS window_class = {0};
    HWND win;
    const int width = 600;
    const int height = 300;

    window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = msg_handler;
    window_class.hInstance = instance;
    window_class.lpszClassName = "filetic";

    ATOM res = RegisterClass(&window_class);
    assert(res);

    win = CreateWindowEx(0, window_class.lpszClassName, "FileTic",
                         WS_OVERLAPPEDWINDOW | WS_VISIBLE, 10, 10, width,
                         height, 0, 0, window_class.hInstance, 0);

    assert(win);

    running = true;
    while (running)
    {
        MSG msg;
        while(PeekMessage(&msg, win, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}

