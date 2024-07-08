#pragma once
#include <stdbool.h>
#include "define.h"
// @save

// This code is generated

// @end

typedef struct SetCellU64
{ 
    u64 key; 
    bool active; 
    bool deleted; 
} SetCellU64;

typedef struct SetCellCharPtr
{ 
    char* key; 
    bool active; 
    bool deleted; 
} SetCellCharPtr;

typedef struct SetU64
{ 
    SetCellU64* cells; 
    u32 size; 
    u32 capacity; 
 
    u64 (*hash_function)(const void* key, u32 len, u64 seed); 
} SetU64;

typedef struct SetCharPtr
{ 
    SetCellCharPtr* cells; 
    u32 size; 
    u32 capacity; 
 
    u64 (*hash_function)(const void* key, u32 len, u64 seed); 
} SetCharPtr;

SetU64 set_create_u64(u32 capacity, u64 (*hash_function)(const void* key, u32 len, u64 seed));

SetCharPtr set_create_char_ptr(u32 capacity, u64 (*hash_function)(const void* key, u32 len, u64 seed));

void set_clear_u64(SetU64* set);

void set_clear_char_ptr(SetCharPtr* set);

void set_insert_u64(SetU64* set, u64 key);

void set_insert_char_ptr(SetCharPtr* set, char* key);

b8 set_contains_u64(SetU64* set, const u64 key);

b8 set_contains_char_ptr(SetCharPtr* set, const char* key);

SetCellU64* set_remove_u64(SetU64* set, const u64 key);

SetCellCharPtr* set_remove_char_ptr(SetCharPtr* set, const char* key);

