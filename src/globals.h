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
V4 global_get_bar_top_color();
V4 global_get_bar_bottom_color();
V4 global_get_scroll_bar_color();
f32 global_get_border_width();

void global_set_clear_color(V4 v);
void global_set_highlight_color(V4 v);
void global_set_border_color(V4 v);
void global_set_lighter_color(V4 v);
void global_set_bright_color(V4 v);
void global_set_secondary_color(V4 v);
void global_set_text_color(V4 v);
void global_set_tab_color(V4 v);
void global_set_bar_top_color(V4 v);
void global_set_bar_bottom_color(V4 v);
void global_set_scroll_bar_color(V4 v);
void global_set_border_width(f32 v);
