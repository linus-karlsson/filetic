#pragma once
#include "define.h"

u64 hash_murmur(const void* key, u32 len, u64 seed);
u64 hash_djb2(const void* key, u32 len, u64 seed);
