#include "random.h"

#define RANDOM_MAX_U32 0x7FFFFFFF
#define RANDOM_MAX_U64 0x7FFFFFFFFFFFFFFF

u32 random_u32s(u32 seed)
{
    seed = (seed << 13) ^ seed;
    return ((seed * (seed * seed * 15731 + 789221) + 1376312589) & RANDOM_MAX_U32);
}

u32 random_u32ss(u32 seed, u32 low, u32 high)
{
    return (random_u32s(seed) / (RANDOM_MAX_U32 / (high + 1 - low))) + low;
}

f32 random_f32s(u32 seed, f32 low, f32 high)
{
    return ((f32)random_u32s(seed) / ((f32)RANDOM_MAX_U32 / (high - low))) + low;
}

u64 random_u64s(u64 seed)
{
    seed = (seed << 13) ^ seed;
    return ((seed * (seed * seed * 15731 + 789221) + 1376312589) & RANDOM_MAX_U64);
}

u64 random_u64ss(u64 seed, u64 low, u64 high)
{
    return (random_u64s(seed) / (RANDOM_MAX_U64 / (high + 1 - low))) + low;
}

