#include "collision.h"

b8 aabb_equal(const AABB* first, const AABB* second)
{
    return v2_equal(first->min, second->min) &&
           v2_equal(first->size, second->size);
}

b8 collision_point_in_point(V2 point_pos, V2 target, V2 target_size)
{
    target.x -= target_size.x * 0.5f;
    target.y -= target_size.y * 0.5f;
    return (point_pos.x >= target.x && point_pos.y >= target.y &&
            point_pos.x < target.x + target_size.x &&
            point_pos.y < target.y + target_size.y);
}

b8 collision_point_in_aabb(V2 point_pos, const AABB* target)
{
    b8 res = point_pos.x >= target->min.x && point_pos.y >= target->min.y &&
             point_pos.x < target->min.x + target->size.x &&
             point_pos.y < target->min.y + target->size.y;
    return res;
}

u32 collision_point_in_aabb_what_side(V2 point_pos, const AABB* target)
{
    u32 side = 0;
    if (collision_point_in_aabb(point_pos, target))
    {
        const f32 middle_point = target->min.x + target->size.width * 0.5f;
        side = point_pos.x < middle_point ? 1 : 2;
    }
    return side;
}

b8 collision_aabb_in_aabb(const AABB* test_obj, const AABB* target_obj)
{
    return test_obj->min.x <= target_obj->min.x + target_obj->size.x &&
           test_obj->min.x + test_obj->size.x >= target_obj->min.x &&
           test_obj->min.y <= target_obj->min.y + target_obj->size.y &&
           test_obj->min.y + test_obj->size.y >= target_obj->min.y;
}
