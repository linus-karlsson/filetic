#pragma once
#include "../define.h"

typedef void Platform;

void platform_init(const char* title, u16 width, u16 height,
                   Platform** platform);
