#pragma once
#include "define.h"
#include "ftic_math.h"

typedef struct AABB
{
    V2 min;
    V2 size;
} AABB;

typedef struct AABBArray
{
    u32 size;
    u32 capacity;
    AABB* data;
}AABBArray;

b8 collision_point_in_point(V2 point_pos, V2 target, V2 target_size);
b8 collision_point_in_aabb(V2 point_pos, const AABB* target);
