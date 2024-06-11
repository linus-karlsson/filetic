#pragma once
#include "define.h"

typedef void Platform;

typedef struct ClientRect
{
    int left;
    int top;
    int right;
    int bottom;
} ClientRect;

void platform_init(const char* title, u16 width, u16 height,
                   Platform** platform);
b8 platform_is_running(Platform* platform);
void platform_event_fire(Platform* platform);

char* platform_get_last_error();
void platform_print_string(const char* string);
void platform_local_free(void* memory);

void platform_opengl_init(Platform* platform);
void platform_opengl_clean(Platform* platform);
void platform_opengl_swap_buffers(Platform* platform);

ClientRect platform_get_client_rect(Platform* platform);
