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

char* get_last_error()
{
    DWORD error = GetLastError();
    if (!error) return NULL;

    char* message = NULL;

    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char*)&message, 0,
        NULL);

    return message;
}

int main(int argc, char** argv)
{
    WNDCLASS window_class;
    HWND win;
    const int width = 600;
    const int height = 300;

    window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = msg_handler;
    window_class.hInstance = GetModuleHandle(0);
    window_class.lpszClassName = "filetic";

    ATOM res = RegisterClass(&window_class);
    char* message = get_last_error();
    assert(res);

    win = CreateWindowEx(0, window_class.lpszClassName, "FileTic",
                         WS_OVERLAPPEDWINDOW | WS_VISIBLE, 10, 10, width,
                         height, 0, 0, window_class.hInstance, 0);

    assert(win);

    running = true;
    while (running)
    {
        MSG msg;
        while (PeekMessage(&msg, win, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

#if 0
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

#endif
