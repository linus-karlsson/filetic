#pragma once
#include "define.h"
#include "math/ftic_math.h"

typedef struct AABB
{
    V2 min;
    V2 size;
} Rect, AABB;

typedef struct AABB3D
{
    V3 min;
    V3 size;
} Rect3D, AABB3D;

typedef struct AABBArray
{
    u32 size;
    u32 capacity;
    AABB* data;
}AABBArray;

b8 aabb_equal(const AABB* first, const AABB* second);

b8 collision_point_in_point(V2 point_pos, V2 target, V2 target_size);
b8 collision_point_in_aabb(V2 point_pos, const AABB* target);

u32 collision_point_in_aabb_what_side(V2 point_pos, const AABB* target);
b8 collision_aabb_in_aabb(const AABB* test_obj, const AABB* target_obj);
