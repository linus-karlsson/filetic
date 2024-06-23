#include "ftic_window.h"
#include "define.h"
#include "logging.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

FTicWindow* window_init(const char* title, int width, int height)
{
    if (!glfwInit())
    {
        log_error_message("glfwInit()", 10);
        ftic_assert(false);
    }

    GLFWwindow* window = glfwCreateWindow(width, height, "FileTic", NULL, NULL);

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

void window_set_on_button_event(FTicWindow* window, OnButtonCallback callback){
    glfwSetMouseButtonCallback((GLFWwindow*)window, callback);
}

void window_set_on_mouse_move_event(FTicWindow* window, OnMouseMovedCallback callback){
    glfwSetCursorPosCallback((GLFWwindow*)window, callback);
}

void window_set_on_mouse_wheel_event(FTicWindow* window, OnMouseWheelCallback callback)
{
    glfwSetScrollCallback((GLFWwindow*)window, callback);
}

void window_set_on_key_stroke_event(FTicWindow* window, OnKeyStrokeCallback callback)
{
    glfwSetCharCallback((GLFWwindow*)window, callback);
}

void window_get_mouse_position(FTicWindow* window, double* x, double* y)
{
    glfwGetCursorPos((GLFWwindow*)window, x, y);
}

int window_should_close(FTicWindow* window)
{
    return glfwWindowShouldClose((GLFWwindow*)window);
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
