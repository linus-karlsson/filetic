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

typedef struct CellU64Char
{ 
    u64 key; 
    char* value; 
    bool active; 
    bool deleted; 
} CellU64Char;

typedef struct HashTableUU64
{ 
    CellUU64* cells; 
    u32 size; 
    u32 capacity; 
 
    u64 (*hash_function)(const void* key, u32 len, u64 seed); 
} HashTableUU64;

typedef struct HashTableU64Char
{ 
    CellU64Char* cells; 
    u32 size; 
    u32 capacity; 
 
    u64 (*hash_function)(const void* key, u32 len, u64 seed); 
} HashTableU64Char;

HashTableUU64 hash_table_create_uu64(u32 capacity, u64 (*hash_function)(const void* key, u32 len, u64 seed));

HashTableU64Char hash_table_create_u64_char(u32 capacity, u64 (*hash_function)(const void* key, u32 len, u64 seed));

void hash_table_insert_uu64(HashTableUU64* table, u64 key, u64 value);

void hash_table_insert_u64_char(HashTableU64Char* table, u64 key, char* value);

u64* hash_table_get_uu64(HashTableUU64* table, const u64 key);

char** hash_table_get_u64_char(HashTableU64Char* table, const u64 key);

CellUU64* hash_table_remove_uu64(HashTableUU64* table, const u64 key);

CellU64Char* hash_table_remove_u64_char(HashTableU64Char* table, const u64 key);

