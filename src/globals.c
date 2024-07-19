#include "globals.h"

global V4 clear_color = {
    .r = 0.09f,
    .g = 0.09f,
    .b = 0.09f,
    .a = 1.0f,
};

global V4 high_light_color = {
    .r = 0.2f,
    .g = 0.2f,
    .b = 0.2f,
    .a = 1.0f,
};

global V4 border_color = {
    .r = 0.35f,
    .g = 0.35f,
    .b = 0.35f,
    .a = 1.0f,
};

global V4 lighter_color = {
    .r = 0.55f,
    .g = 0.55f,
    .b = 0.55f,
    .a = 1.0f,
};
global V4 bright_color = {
    .r = 0.7f,
    .g = 0.7f,
    .b = 0.7f,
    .a = 1.0f,
};
global V4 secondary_color = {
    .r = 0.0f,
    .g = 0.59f,
    .b = 1.0f,
    .a = 1.0f,
};

global V4 text_color = {
    .r = 1.0f,
    .g = 1.0f,
    .b = 1.0f,
    .a = 1.0f,
};

global f32 border_width = 1.0f;


V4 global_get_clear_color()
{
    return clear_color;
}

V4 global_get_highlight_color()
{
    return high_light_color;
}

V4 global_get_border_color()
{
    return border_color;
}

V4 global_get_lighter_color()
{
    return lighter_color;
}

V4 global_get_bright_color()
{
    return bright_color;
}

V4 global_get_secondary_color()
{
    return secondary_color;
}

V4 global_get_text_color()
{
    return text_color;
}

f32 global_get_border_width()
{
    return border_width;
}

void global_set_clear_color(V4 v)
{
    clear_color = v;
} 

void global_set_highlight_color(V4 v)
{
    high_light_color = v;
}

void global_set_border_color(V4 v)
{
    border_color = v;
}

void global_set_lighter_color(V4 v)
{
    lighter_color = v;
}

void global_set_bright_color(V4 v)
{
    bright_color = v;
}

void global_set_secondary_color(V4 v)
{
    secondary_color = v;
}

void global_set_text_color(V4 v)
{
    text_color = v;
}

void global_set_border_width(f32 v)
{
    border_width = v;
}
