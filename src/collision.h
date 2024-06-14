#pragma once
#include "define.h"
#include "ftic_math.h"

typedef struct AABB
{
    V2 min;
    V2 size;
    u32 id;
} AABB;

b8 collision_point_in_point(V2 point_pos, V2 target, V2 target_size);
b8 collision_point_in_aabb_2d(V2 point_pos, const AABB* target);