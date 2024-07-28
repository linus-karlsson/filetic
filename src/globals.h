#pragma once
#include "define.h"
#include "math/ftic_math.h"

V4 global_get_clear_color();
V4 global_get_highlight_color();
V4 global_get_border_color();
V4 global_get_lighter_color();
V4 global_get_bright_color();
V4 global_get_secondary_color();
V4 global_get_text_color();
V4 global_get_tab_color();
f32 global_get_border_width();

void global_set_clear_color(V4 v);
void global_set_highlight_color(V4 v);
void global_set_border_color(V4 v);
void global_set_lighter_color(V4 v);
void global_set_bright_color(V4 v);
void global_set_secondary_color(V4 v);
void global_set_text_color(V4 v);
void global_set_tab_color(V4 v);
void global_set_border_width(f32 v);

void global_set_default_text_color();
void global_set_default_secondary_color();
void global_set_default_bright_color();
void global_set_default_lighter_color();
void global_set_default_border_color();
void global_set_default_highlight_color();
void global_set_default_clear_color();
void global_set_default_tab_color();
