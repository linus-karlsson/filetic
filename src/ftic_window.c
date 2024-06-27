#include "ftic_window.h"
#include "define.h"
#include "logging.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

global GLFWcursor* cursors[TOTAL_CURSORS] = { 0 };

FTicWindow* window_init(const char* title, int width, int height)
{
    if (!glfwInit())
    {
        log_error_message("glfwInit()", 10);
        ftic_assert(false);
    }

    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(width, height, "FileTic", NULL, NULL);

    cursors[FTIC_HAND_CURSOR] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    cursors[FTIC_RESIZE_H_CURSOR] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    cursors[FTIC_RESIZE_V_CURSOR] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    cursors[FTIC_RESIZE_V_CURSOR] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    cursors[FTIC_MOVE_CURSOR] = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);

    if (!window)
    {
        log_error_message("glfwCreateWindow()", 18);
        glfwTerminate();
        ftic_assert(false);
    }
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    return (FTicWindow*)window;
}

void window_set_on_key_event(FTicWindow* window, OnKeyCallback callback)
{
    glfwSetKeyCallback((GLFWwindow*)window, callback);
}

void window_set_on_button_event(FTicWindow* window, OnButtonCallback callback)
{
    glfwSetMouseButtonCallback((GLFWwindow*)window, callback);
}

void window_set_on_mouse_move_event(FTicWindow* window,
                                    OnMouseMovedCallback callback)
{
    glfwSetCursorPosCallback((GLFWwindow*)window, callback);
}

void window_set_on_mouse_wheel_event(FTicWindow* window,
                                     OnMouseWheelCallback callback)
{
    glfwSetScrollCallback((GLFWwindow*)window, callback);
}

void window_set_on_key_stroke_event(FTicWindow* window,
                                    OnKeyStrokeCallback callback)
{
    glfwSetCharCallback((GLFWwindow*)window, callback);
}

void window_get_mouse_position(FTicWindow* window, double* x, double* y)
{
    glfwGetCursorPos((GLFWwindow*)window, x, y);
}

void window_set_cursor(FTicWindow* window, int cursor)
{
    glfwSetCursor((GLFWwindow*)window, cursors[cursor]);
}

void window_set_on_drop_event(FTicWindow* window, OnDropCallback callback)
{
    glfwSetDropCallback((GLFWwindow*)window, callback);
}

int window_should_close(FTicWindow* window)
{
    return glfwWindowShouldClose((GLFWwindow*)window);
}

void window_wait_event()
{
    glfwWaitEvents();
}

void window_poll_event()
{
    glfwPollEvents();
}

void window_swap(FTicWindow* window)
{
    glfwSwapBuffers((GLFWwindow*)window);
}

void window_get_size(FTicWindow* window, int* width, int* height)
{
    glfwGetWindowSize((GLFWwindow*)window, width, height);
}

f64 window_get_time()
{
    return glfwGetTime();
}
