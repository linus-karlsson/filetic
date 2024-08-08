#pragma once
#include "define.h"
#include "math/ftic_math.h"
#include "util.h"
#include "collision.h"

typedef struct Camera
{
    VP view_projection;
    V3 acceleration;
    V3 velocity;
    V3 position;
    V3 orientation;
    V3 up;
    f32 speed;
    f32 old_speed;
    f32 sens;

    AABB view_port;
    b8 first_clicked;
} Camera;

Camera camera_create_default();
Camera camera_create(f32 speed, f32 sensitivity);

b8 camera_update(Camera* camera, const f64 delta_time);
void camera_print(const Camera* camera);
void camera_set_based_on_mesh_aabb(Camera* camera, const AABB3D* mesh_aabb);
