#include "collision.h"

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
