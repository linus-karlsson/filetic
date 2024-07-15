#include "ui_test.h"
#include <stdio.h>

int main(int argc, char** argv)
{
    ui_test_begin();
    {
        ui_test_set_scroll_offset();
        ui_test_smooth_scroll();
        ui_test_push_window_to_front();
        ui_test_push_window_to_back();
        ui_test_push_window_to_first_docked();
        ui_test_generate_id();
        ui_test_get_window_index();
        ui_test_set_docking();
    }
    ui_test_end();
}
