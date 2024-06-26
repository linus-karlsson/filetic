#include "hash.h"

u64 hash_murmur(const void* key, u32 len, u64 seed)
{
    u64 h = seed;
    const char* key2 = *((const char**)key);
    if (len > 3)
    {
        const u32* key_x4 = (const u32*)key2;
        u32 i, n = len >> 2;
        for (i = 0; i < n; i++)
        {
            u32 k = key_x4[i];
            k *= 0xcc9e2d51;
            k = (k << 15) | (k >> 17);
            k *= 0x1b873593;
            h ^= k;
            h = (h << 13) | (h >> 19);
            h = (h * 5) + 0xe6546b64;
        }
        key2 = (const char*)(key_x4 + n);
        len &= 3;
    }

    u64 k1 = 0;
    switch (len)
    {
        case 3:
        {
            k1 ^= key2[2] << 16;
        }
        case 2:
        {
            k1 ^= key2[1] << 8;
        }
        case 1:
        {
            k1 ^= key2[0];
            k1 *= 0xcc9e2d51;
            k1 = (k1 << 15) | (k1 >> 17);
            k1 *= 0x1b873593;
            h ^= k1;
        }
    }

    h ^= len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}

u64 hash_djb2(const void* key, u32 len, u64 seed)
{
    u64 hash = seed;

    const u8* key_u8 = *((const u8**)key);
    for(u32 i = 0; i < len; i++)
    {
        hash = ((hash << 5) + hash) + key_u8[i];
    }
    return hash;
}

