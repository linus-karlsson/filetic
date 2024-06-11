#pragma once
#include "../define.h"

typedef void Platform;

typedef struct ClientRect
{
    u64 left;
    u64 top;
    u64 right;
    u64 bottom;
} ClientRect;

void platform_init(const char* title, u16 width, u16 height,
                   Platform** platform);

ClientRect platform_get_client_rect
