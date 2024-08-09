#include "theme.h"
#include "globals.h"
#include "ui.h"

void theme_set_dark(ThemeColorPicker* picker)
{
    global_set_clear_color(v4ic(0.1f));
    global_set_secondary_color(v4f(0.0f, 0.58f, 1.0f, 1.0f));
    global_set_text_color(v4ic(1.0f));
    global_set_tab_color(v4ic(0.25f));
    global_set_bar_top_color(v4ic(0.25f));
    global_set_bar_bottom_color(v4ic(0.15f));
    global_set_border_color(v4ic(0.35f));
    global_set_scroll_bar_color(v4ic(0.55f));

    picker->secondary_color.at = v2f(200.0f, 0.0f);
    picker->secondary_color.spectrum_at = 114.0f;
    picker->clear_color.at = v2f(0.0f, 180.0f);
    picker->text_color.at = v2i(0.0f);
    picker->tab_color.at = v2f(0.0f, 150.0f);
    picker->bar_top_color.at = v2f(0.0f, 150.0f);
    picker->bar_bottom_color.at = v2f(0.0f, 170.0f);
    picker->border_color.at = v2f(0.0f, 130.0f);
    picker->scroll_bar_color.at = v2f(0.0f, 90.0f);
}

void theme_set_light(ThemeColorPicker* picker)
{
    global_set_clear_color(v4ic(0.9f));
    global_set_secondary_color(v4f(0.0f, 0.58f, 1.0f, 1.0f));
    global_set_text_color(v4ic(0.0f));
    global_set_tab_color(v4ic(0.705f));
    global_set_bar_top_color(v4ic(0.9f));
    global_set_bar_bottom_color(v4ic(0.9f));
    global_set_border_color(v4ic(1.0f));
    global_set_scroll_bar_color(v4ic(0.2f));

    picker->clear_color.at = v2f(0.0f, 20.0f);
    picker->secondary_color.at = v2f(200.0f, 0.0f);
    picker->secondary_color.spectrum_at = 114.0f;
    picker->text_color.at = v2f(0.0f, 200.0f);
    picker->tab_color.at = v2f(0.0f, 59.0f);
    picker->bar_top_color.at = v2f(0.0f, 20.0f);
    picker->bar_bottom_color.at = v2f(0.0f, 20.0f);
    picker->border_color.at = v2d();
    picker->scroll_bar_color.at = v2f(0.0f, 160.0f);
}

void theme_set_tron(ThemeColorPicker* picker)
{
    global_set_clear_color(v4ic(0.015f));
    global_set_secondary_color(v4f(1.0f, 0.36f, 0.0f, 1.0f));
    global_set_text_color(v4f(0.0f, 0.76f, 1.0f, 1.0f));
    global_set_tab_color(v4f(0.119f, 0.128f, 0.155f, 1.0f));
    global_set_bar_top_color(v4ic(0.2f));
    global_set_bar_bottom_color(v4ic(0.06f));
    global_set_border_color(v4f(0.0784f, 0.38f, 0.475f, 1.0f));
    global_set_scroll_bar_color(v4f(0.15225f, 0.3908f, 0.525f, 1.0f));

    picker->clear_color.at = v2f(16.0f, 197.0f);
    picker->clear_color.spectrum_at = 116;
    picker->secondary_color.at = v2f(200.0f, 0.0f);
    picker->secondary_color.spectrum_at = 12.0f;
    picker->text_color.at = v2f(200.0f, 0.0f);
    picker->text_color.spectrum_at = 108.0f;
    picker->tab_color.at = v2f(46.0f, 169.0f);
    picker->tab_color.spectrum_at = 125.0f;
    picker->bar_top_color.at = v2f(0.0f, 160.0f);
    picker->bar_bottom_color.at = v2f(0.0f, 188.0f);
    picker->border_color.at = v2f(167.0f, 105.0f);
    picker->border_color.spectrum_at = 108.0f;
    picker->scroll_bar_color.at = v2f(142.0f, 95.0f);
    picker->scroll_bar_color.spectrum_at = 122.0f;
}

void theme_set_slime(ThemeColorPicker* picker)
{
    global_set_clear_color(v4f(0.0308f, 0.08f, 0.0371f, 1.0f));
    global_set_secondary_color(v4f(0.0f, 1.0f, 0.13f, 1.0f));
    global_set_text_color(v4f(0.98f, 1.0f, 0.0f, 1.0f));
    global_set_tab_color(v4f(0.0666f, 0.185f, 0.1175f, 1.0f));
    global_set_bar_top_color(v4f(0.125f, 0.27f, 0.154f, 1.0f));
    global_set_bar_bottom_color(v4f(0.033f, 0.085f, 0.0445f, 1.0f));
    global_set_border_color(v4f(0.3315f, 0.51f, 0.0f, 1.0f));
    global_set_scroll_bar_color(v4f(0.40647f, 0.62f, 0.0992f, 1.0f));

    picker->clear_color.at = v2f(123.0f, 187.0f);
    picker->clear_color.spectrum_at = 71;
    picker->secondary_color.at = v2f(200.0f, 0.0f);
    picker->secondary_color.spectrum_at = 71.0f;
    picker->text_color.at = v2f(200.0f, 0.0f);
    picker->text_color.spectrum_at = 34.0f;
    picker->tab_color.at = v2f(128.0f, 163.0f);
    picker->tab_color.spectrum_at = 81.0f;
    picker->bar_top_color.at = v2f(110.0f, 146.0f);
    picker->bar_top_color.spectrum_at = 74.0f;
    picker->bar_bottom_color.at = v2f(122.0f, 183.0f);
    picker->bar_bottom_color.spectrum_at = 74.0f;
    picker->border_color.at = v2f(200.0f, 98.0f);
    picker->border_color.spectrum_at = 45.0f;
    picker->scroll_bar_color.at = v2f(168.0f, 76.0f);
    picker->scroll_bar_color.spectrum_at = 47.0f;
}
