#include "ui_test.h"
#include "collision_test.h"
#include <stdio.h>

int main(int argc, char** argv)
{
    ui_test_begin();
    {
        ui_test_set_scroll_offset();
        ui_test_smooth_scroll();
        ui_test_push_window_to_front();
        ui_test_push_window_to_back();
        ui_test_generate_id();
        ui_test_dock_node_create_();
        ui_test_dock_node_create();
        ui_test_dock_node_create_multiple_windows();
        ui_test_dock_node_set_split();
        ui_test_dock_node_resize_traverse();
        ui_test_dock_node_dock_window();
        ui_test_dock_node_resize_from_root();
        ui_test_find_node();
        ui_test_dock_node_remove_node();
        ui_test_set_docking();
        ui_test_display_docking();
        ui_test_remove_window_from_shared_dock_space();
        ui_test_set_resize_cursor();
        ui_test_look_for_window_resize();
        ui_test_check_window_collisions();
        ui_test_check_if_window_should_be_docked();
        ui_test_check_dock_space_resize();
        ui_test_sync_current_frame_windows();
        ui_test_ui_window_close();
        ui_test_ui_window_close_current();
        ui_test_ui_window_set_size();
        ui_test_ui_window_set_position();
    }
    ui_test_end();

    collision_test_begin();
    {
        collision_test_collision_aabb_in_aabb();
        collision_test_collision_point_in_aabb_what_side();
        collision_test_collision_point_in_aabb();
        collision_test_collision_point_in_point();
        collision_test_aabb_equal();
    }
    collision_test_end();
}
