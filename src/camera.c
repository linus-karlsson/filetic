#include "camera.h"
#include "event.h"

Camera camera_create_default()
{
    Camera result;
    result.position = v3f(0.0f, 0.0f, 1.0f);
    result.orientation = v3f(0.0f, 0.0f, -1.0f);
    result.up = v3f(0.0f, 1.0f, 0.0f);
    result.velocity = v3d();
    result.view_projection.view =
        view(result.position, v3_add(result.position, result.orientation),
             result.up);
    result.speed = 1.5f;
    result.old_speed = result.speed;
    result.sens = 5.0f;
    return result;
}

Camera camera_create(f32 speed, f32 sensitivity)
{
    Camera result;
    result.position = v3f(0.0f, 0.0f, 0.0f);
    result.orientation = v3f(0.0f, 0.0f, -1.0f);
    result.up = v3f(0.0f, 1.0f, 0.0f);
    result.velocity = v3d();
    result.view_projection.view =
        view(result.position, v3_add(result.position, result.orientation),
             result.up);
    result.speed = speed;
    result.old_speed = speed;
    result.sens = sensitivity;
    return result;
}

V2 get_mouse_rotation(Camera* camera, const f64 delta_time)
{
    FTicWindow* window = window_get_current();

    V2 half =
        v2_add(camera->view_port.min, v2_s_multi(camera->view_port.size, 0.5f));

    if (camera->first_clicked)
    {
        window_set_cursor_position(window, half.width, half.height);
        camera->first_clicked = false;
    }

    double x, y;
    window_get_mouse_position(window, &x, &y);
    V2 mouse_position = v2f((f32)x, (f32)y);

    V2 rotation = { 0 };
    rotation.x =
        camera->sens * (f32)((f64)(mouse_position.x - half.width) * delta_time);
    rotation.y = camera->sens *
                 (f32)((f64)(mouse_position.y - half.height) * delta_time);

    window_set_cursor_position(window, half.width, half.height);
    return rotation;
}

b8 camera_update(Camera* camera, const f64 delta_time)
{

    b8 moved = false;
    if (event_is_key_pressed(FTIC_KEY_W))
    {
        v3_add_equal(
            &camera->position,
            v3_s_multi(camera->orientation, (f32)(camera->speed * delta_time)));
        moved = true;
    }
    if (event_is_key_pressed(FTIC_KEY_S))
    {
        v3_add_equal(&camera->position,
                     v3_s_multi(v3_s_multi(camera->orientation, -1.0f),
                                (f32)(camera->speed * delta_time)));
        moved = true;
    }
    if (event_is_key_pressed(FTIC_KEY_A))
    {
        v3_add_equal(
            &camera->position,
            v3_s_multi(v3_s_multi(v3_normalize(v3_cross(camera->orientation,
                                                        camera->up)),
                                  -1.0f),
                       (f32)(camera->speed * delta_time)));
        moved = true;
    }
    if (event_is_key_pressed(FTIC_KEY_D))
    {
        v3_add_equal(
            &camera->position,
            v3_s_multi(v3_normalize(v3_cross(camera->orientation, camera->up)),
                       (f32)(camera->speed * delta_time)));
        moved = true;
    }
    if (event_is_key_pressed(FTIC_KEY_SPACE))
    {
        v3_add_equal(&camera->position,
                     v3_s_multi(camera->up, (f32)(camera->speed * delta_time)));
        moved = true;
    }
    if (event_is_key_pressed(FTIC_KEY_LEFT_CONTROL))
    {
        v3_add_equal(&camera->position,
                     v3_s_multi(v3_s_multi(camera->up, -1.0f),
                                (f32)(camera->speed * delta_time)));
        moved = true;
    }

    if (event_is_key_pressed(FTIC_KEY_LEFT_SHIFT))
    {
        camera->speed = camera->old_speed * 4.0f;
    }
    else
    {
        camera->speed = camera->old_speed;
    }

    const MouseButtonEvent* mouse_button_event = event_get_mouse_button_event();
    if (event_is_mouse_button_pressed(FTIC_MOUSE_BUTTON_RIGHT))
    {
        window_set_input_mode(window_get_current(), FTIC_MODE_CURSOR,
                              FTIC_MODE_CURSOR_HIDDEN);
        V2 rotation = get_mouse_rotation(camera, delta_time);

        V3 temp_orientation =
            v3_rotate(camera->orientation, radians(rotation.y),
                      v3_normalize(v3_cross(camera->orientation, camera->up)));

        if (abs_f32(v3_angle(temp_orientation, camera->up) - radians(90.0f)) <=
            radians(85.0f))
        {
            camera->orientation = temp_orientation;
        }

        camera->orientation =
            v3_rotate(camera->orientation, radians(rotation.x), camera->up);
    }
    else if (mouse_button_event->activated &&
             mouse_button_event->action == FTIC_RELEASE &&
             !camera->first_clicked)
    {
        window_set_input_mode(window_get_current(), FTIC_MODE_CURSOR,
                              FTIC_MODE_CURSOR_NORMAL);
        camera->first_clicked = true;
    }
    return moved;
}
