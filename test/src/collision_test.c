#include "collision_test.h"
#include "collision.c"
#include "asserts.h"

global u32 g_total_test_failed_count = 0;

void collision_test_begin()
{
    printf("Collision tests:\n");
}

void collision_test_end()
{
    if (g_total_test_failed_count)
    {
        printf("\tTotal failed tests: %u\n", g_total_test_failed_count);
    }
    else
    {
        printf("\tNo failed tests\n");
    }
}

void collision_test_aabb_equal()
{
    V2 min1 = v2f(0.0f, 0.0f);
    V2 size1 = v2f(1.0f, 1.0f);
    V2 min2 = v2f(0.0f, 0.0f);
    V2 size2 = v2f(1.0f, 1.0f);
    V2 min3 = v2f(1.0f, 1.0f);
    V2 size3 = v2f(2.0f, 2.0f);

    AABB a = { .min = min1, .size = size1 };
    AABB b = { .min = min2, .size = size2 };
    AABB c = { .min = min3, .size = size3 };

    ASSERT_EQUALS(true, aabb_equal(&a, &b), EQUALS_FORMAT_U32);
    ASSERT_EQUALS(false, aabb_equal(&a, &c), EQUALS_FORMAT_U32);
}

void collision_test_collision_point_in_point()
{
    V2 point1 = v2f(0.0f, 0.0f);
    V2 target = v2f(0.0f, 0.0f);
    V2 target_size = v2f(2.0f, 2.0f);

    ASSERT_EQUALS(true, collision_point_in_point(point1, target, target_size),
                  EQUALS_FORMAT_U32);

    V2 point2 = v2f(1.5f, 1.5f);
    ASSERT_EQUALS(false, collision_point_in_point(point2, target, target_size),
                  EQUALS_FORMAT_U32);

    V2 point3 = v2f(1.0f, 1.0f);
    ASSERT_EQUALS(true, collision_point_in_point(point3, target, target_size),
                  EQUALS_FORMAT_U32);
}

void collision_test_collision_point_in_aabb()
{
    V2 point1 = v2f(0.5f, 0.5f);
    V2 min = v2f(0.0f, 0.0f);
    V2 size = v2f(1.0f, 1.0f);
    AABB aabb = { .min = min, .size = size };

    ASSERT_EQUALS(true, collision_point_in_aabb(point1, &aabb),
                  EQUALS_FORMAT_U32);

    V2 point2 = v2f(1.5f, 1.5f);
    ASSERT_EQUALS(false, collision_point_in_aabb(point2, &aabb),
                  EQUALS_FORMAT_U32);

    V2 point3 = v2f(1.1f, 1.1f);
    ASSERT_EQUALS(false, collision_point_in_aabb(point3, &aabb),
                  EQUALS_FORMAT_U32);
}

void collision_test_collision_point_in_aabb_what_side()
{
    V2 point1 = v2f(0.25f, 0.5f);
    V2 min = v2f(0.0f, 0.0f);
    V2 size = v2f(1.0f, 1.0f);
    AABB aabb = { .min = min, .size = size };

    ASSERT_EQUALS(1, collision_point_in_aabb_what_side(point1, &aabb),
                  EQUALS_FORMAT_U32);

    V2 point2 = v2f(0.75f, 0.5f);
    ASSERT_EQUALS(2, collision_point_in_aabb_what_side(point2, &aabb),
                  EQUALS_FORMAT_U32);

    V2 point3 = v2f(1.5f, 0.5f);
    ASSERT_EQUALS(0, collision_point_in_aabb_what_side(point3, &aabb),
                  EQUALS_FORMAT_U32);
}

void collision_test_collision_aabb_in_aabb()
{
    V2 min1 = v2f(0.0f, 0.0f);
    V2 size1 = v2f(2.0f, 2.0f);
    V2 min2 = v2f(1.0f, 1.0f);
    V2 size2 = v2f(1.0f, 1.0f);
    V2 min3 = v2f(3.0f, 3.0f);
    V2 size3 = v2f(1.0f, 1.0f);

    AABB a = { .min = min1, .size = size1 };
    AABB b = { .min = min2, .size = size2 };
    AABB c = { .min = min3, .size = size3 };

    ASSERT_EQUALS(true, collision_aabb_in_aabb(&a, &b), EQUALS_FORMAT_U32);
    ASSERT_EQUALS(true, collision_aabb_in_aabb(&b, &a), EQUALS_FORMAT_U32);
    ASSERT_EQUALS(false, collision_aabb_in_aabb(&a, &c), EQUALS_FORMAT_U32);
    ASSERT_EQUALS(false, collision_aabb_in_aabb(&b, &c), EQUALS_FORMAT_U32);
}

