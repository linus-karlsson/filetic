#include "camera.h"
#include "event.h"

Camera camera_create_default()
{
    Camera result;
    result.pos = v3f(0.0f, 0.0f, 1.0f);
    result.ori = v3f(0.0f, 0.0f, -1.0f);
    result.up = v3f(0.0f, 1.0f, 0.0f);
    result.vel = v3d();
    result.view_projection.view =
        view(result.pos, v3_add(result.pos, result.ori), result.up);
    result.speed = 1.5f;
    result.old_speed = result.speed;
    result.sens = 5.0f;
    return result;
}

Camera camera_create(f32 speed, f32 sensitivity)
{
    Camera result;
    result.pos = v3f(0.0f, 0.0f, 0.0f);
    result.ori = v3f(0.0f, 0.0f, -1.0f);
    result.up = v3f(0.0f, 1.0f, 0.0f);
    result.vel = v3d();
    result.view_projection.view =
        view(result.pos, v3_add(result.pos, result.ori), result.up);
    result.speed = speed;
    result.old_speed = speed;
    result.sens = sensitivity;
    return result;
}

V2 get_mouse_rotation(const V2 view_port, const f32 sens, const f32 delta_time,
                      V2* last, b8* first_clicked)
{
    V2 half = v2_s_multi(view_port, 0.5f);

    V2 mouse_position = event_get_mouse_position();
    if (mouse_position.x >= view_port.width - 300 || mouse_position.x <= 300)
    {
        window_set_cursor_position(window_get_current(), half.width,
                                   mouse_position.y);
        mouse_position.x = half.width;
        last->x = mouse_position.x;
    }
    if (mouse_position.y >= view_port.height - 200 || mouse_position.y <= 200)
    {
        window_set_cursor_position(window_get_current(), mouse_position.x,
                                   half.height);
        mouse_position.y = half.height;
        last->y = mouse_position.y;
    }

    V2 rotation = { 0 };

    if (!*first_clicked)
    {
        rotation.x = sens * (f32)((mouse_position.x - last->x)) * delta_time;
        rotation.y = sens * (f32)((mouse_position.y - last->y)) * delta_time;
    }
    else
    {
        *first_clicked = false;
    }

    *last = mouse_position;
    return rotation;
}

b8 camera_update(Camera* camera, const V2 view_port, const f32 delta_time,
                 b8* first_clicked, V2* last)
{

    b8 moved = false;
    if (event_is_key_pressed(FTIC_KEY_W))
    {
        v3_add_equal(&camera->pos,
                     v3_s_multi(camera->ori, (camera->speed * delta_time)));
        moved = true;
    }
    if (event_is_key_pressed(FTIC_KEY_S))
    {
        v3_add_equal(&camera->pos, v3_s_multi(v3_s_multi(camera->ori, -1.0f),
                                              (camera->speed * delta_time)));
        moved = true;
    }
    if (event_is_key_pressed(FTIC_KEY_A))
    {
        v3_add_equal(&camera->pos,
                     v3_s_multi(v3_s_multi(v3_normalize(v3_cross(camera->ori,
                                                                 camera->up)),
                                           -1.0f),
                                (camera->speed * delta_time)));
        moved = true;
    }
    if (event_is_key_pressed(FTIC_KEY_D))
    {
        v3_add_equal(&camera->pos,
                     v3_s_multi(v3_normalize(v3_cross(camera->ori, camera->up)),
                                (camera->speed * delta_time)));
        moved = true;
    }
    if (event_is_key_pressed(FTIC_KEY_SPACE))
    {
        v3_add_equal(&camera->pos,
                     v3_s_multi(camera->up, (camera->speed * delta_time)));
        moved = true;
    }
    if (event_is_key_pressed(FTIC_KEY_RIGHT_CONTROL))
    {
        v3_add_equal(&camera->pos, v3_s_multi(v3_s_multi(camera->up, -1.0f),
                                              (camera->speed * delta_time)));
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
        V2 rotation = get_mouse_rotation(view_port, camera->sens, delta_time,
                                         last, first_clicked);

        V3 temp_orientation =
            v3_rotate(camera->ori, radians(rotation.y),
                      v3_normalize(v3_cross(camera->ori, camera->up)));

        if (abs_f32(v3_angle(temp_orientation, camera->up) - radians(90.0f)) <=
            radians(85.0f))
        {
            camera->ori = temp_orientation;
        }

        camera->ori = v3_rotate(camera->ori, radians(rotation.x), camera->up);
    }
    else if (mouse_button_event->activated && !*first_clicked)
    {
        window_set_input_mode(window_get_current(), FTIC_MODE_CURSOR,
                              FTIC_MODE_CURSOR_NORMAL);
        *first_clicked = true;
    }
    return moved;
}
