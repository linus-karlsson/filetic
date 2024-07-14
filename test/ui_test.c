#include "ui_test.h"
#include "ui.c"
#include "asserts.h"

void ui_test_set_scroll_offset()
{
    f32 actual = set_scroll_offset(-10.0f, 2500.0f, 1000.0f, 0.0f);
    ASSERT_EQUALS(-1000.0f, actual);

    actual = set_scroll_offset(-1.0f, 1000.0f, 500.0f, 0.0f);
    ASSERT_EQUALS(-100.0f, actual);
}

void ui_test_set_scroll_offset_clamp_high()
{
    f32 actual = set_scroll_offset(-10.0f, 2500.0f, 1000.0f, 0.0f);
    ASSERT_EQUALS(-1000.0f, actual);

    actual = set_scroll_offset(-1.0f, 1000.0f, 500.0f, 0.0f);
    ASSERT_EQUALS(-100.0f, actual);
}

void ui_test_set_scroll_offset_clamp_low()
{

}
