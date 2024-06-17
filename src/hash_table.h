#pragma once
#include <stdbool.h>
#include "define.h"
// @save

// This code is generated

// @end

typedef struct CellUU64
{ 
    u64 key; 
    u64 value; 
    bool active; 
    bool deleted; 
 
} CellUU64;

typedef struct CellCharU32
{ 
    char* key; 
    u32 value; 
    bool active; 
    bool deleted; 
 
} CellCharU32;

typedef struct HashTableUU64
{ 
    CellUU64* cells; 
    u32 size; 
    u32 capacity; 
 
    u64 (*hash_function)(const void* key, u32 len, u64 seed); 
} HashTableUU64;

typedef struct HashTableCharU32
{ 
    CellCharU32* cells; 
    u32 size; 
    u32 capacity; 
 
    u64 (*hash_function)(const void* key, u32 len, u64 seed); 
} HashTableCharU32;

HashTableUU64 hash_table_create_uu64(u32 capacity, u64 (*hash_function)(const void* key, u32 len, u64 seed));

HashTableCharU32 hash_table_create_char_u32(u32 capacity, u64 (*hash_function)(const void* key, u32 len, u64 seed));

void hash_table_clear_uu64(HashTableUU64* hash_table);

void hash_table_clear_char_u32(HashTableCharU32* hash_table);

void hash_table_insert_uu64(HashTableUU64* table, u64 key, u64 value);

void hash_table_insert_char_u32(HashTableCharU32* table, char* key, u32 value);

u64* hash_table_get_uu64(HashTableUU64* table, const u64 key);

u32* hash_table_get_char_u32(HashTableCharU32* table, const char* key);

CellUU64* hash_table_remove_uu64(HashTableUU64* table, const u64 key);

CellCharU32* hash_table_remove_char_u32(HashTableCharU32* table, const char* key);

