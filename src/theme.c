#include "theme.h"
#include "globals.h"
#include "ui.h"

void theme_set_dark(ColorPicker* clear_color, ColorPicker* secondary_color, ColorPicker* text_color,
                    ColorPicker* tab_color)
{
    global_set_clear_color(v4ic(0.1f));
    global_set_secondary_color(v4f(0.0f, 0.58f, 1.0f, 1.0f));
    global_set_text_color(v4ic(1.0f));
    global_set_tab_color(v4ic(0.25f));

    secondary_color->at = v2f(200.0f, 0.0f);
    secondary_color->spectrum_at = 114.0f;
    clear_color->at = v2f(0.0f, 180.0f);
    text_color->at = v2i(0.0f);
    tab_color->at = v2f(0.0f, 150.0f);
}

void theme_set_light(ColorPicker* clear_color, ColorPicker* secondary_color,
                     ColorPicker* text_color, ColorPicker* tab_color)
{
    global_set_clear_color(v4ic(0.9f));
    global_set_secondary_color(v4f(0.0f, 0.58f, 1.0f, 1.0f));
    global_set_text_color(v4ic(0.0f));
    global_set_tab_color(v4ic(0.705f));

    clear_color->at = v2f(0.0f, 20.0f);
    secondary_color->at = v2f(200.0f, 0.0f);
    secondary_color->spectrum_at = 114.0f;
    text_color->at = v2f(0.0f, 200.0f);
    tab_color->at = v2f(0.0f, 59.0f);
}

void theme_set_tron(ColorPicker* clear_color, ColorPicker* secondary_color, ColorPicker* text_color,
                    ColorPicker* tab_color)
{
    global_set_clear_color(v4ic(0.015f));
    global_set_secondary_color(v4f(1.0f, 0.36f, 0.0f, 1.0f));
    global_set_text_color(v4f(0.0f, 0.7f, 1.0f, 1.0f));
    global_set_tab_color(v4f(0.119f, 0.128f, 0.155f, 1.0f));

    clear_color->at = v2f(16.0f, 197.0f);
    clear_color->spectrum_at = 116;
    secondary_color->at = v2f(200.0f, 0.0f);
    secondary_color->spectrum_at = 12.0f;
    text_color->at = v2f(200.0f, 0.0f);
    text_color->spectrum_at = 110.0f;
    tab_color->at = v2f(46.0f, 169.0f);
    tab_color->spectrum_at = 125.0f;
}

void theme_set_slime(ColorPicker* clear_color, ColorPicker* secondary_color, ColorPicker* text_color,
                    ColorPicker* tab_color)
{
    global_set_clear_color(v4f(0.0308f, 0.08f, 0.0371f, 1.0f));
    global_set_secondary_color(v4f(0.0f, 1.0f, 0.13f, 1.0f));
    global_set_text_color(v4f(0.98f, 1.0f, 0.0f, 1.0f));
    global_set_tab_color(v4f(0.0666f, 0.185f, 0.1175f, 1.0f));

    clear_color->at = v2f(123.0f, 187.0f);
    clear_color->spectrum_at = 71;
    secondary_color->at = v2f(200.0f, 0.0f);
    secondary_color->spectrum_at = 71.0f;
    text_color->at = v2f(200.0f, 0.0f);
    text_color->spectrum_at = 34.0f;
    tab_color->at = v2f(128.0f, 163.0f);
    tab_color->spectrum_at = 81.0f;
}
