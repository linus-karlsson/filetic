#pragma once
#include <stdbool.h>
#include "define.h"
// @save

// This code is generated

// @end

typedef struct CellUU64
{ 
    u64* key; 
    u64 value; 
    bool active; 
    bool deleted; 
 
} CellUU64;

typedef struct HashTableUU64
{ 
    CellUU64* cells; 
    u32 size; 
    u32 capacity; 
 
    u64 (*hash_function)(const void* key, u32 len, u64 seed); 
} HashTableUU64;

HashTableUU64 hash_table_createUu64(u32 capacity, u64 (*hash_function)(const void* key, u32 len, u64 seed));

void hash_table_insertUu64(HashTableUU64* table, u64* key, u64 value);

u64* hash_table_getUu64(HashTableUU64* table, const u64* key);

CellUU64* hash_table_removeUu64(HashTableUU64* table, const u64* key);

