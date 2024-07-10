#pragma once
#include "define.h"
#include "math/ftic_math.h"
#include "util.h"

typedef struct Camera
{
    VP view_projection;
    V3 acc;
    V3 vel;
    V3 pos;
    V3 ori;
    V3 up;
    f32 speed;
    f32 old_speed;
    f32 sens;

    V2 view_port;
    b8 first_clicked;
} Camera;

Camera camera_create_default();
Camera camera_create(f32 speed, f32 sensitivity);

b8 camera_update(Camera* camera, const f32 delta_time);
void camera_print(const Camera* camera);
V2 get_mouse_rotation(Camera* camera, const f32 delta_time);

