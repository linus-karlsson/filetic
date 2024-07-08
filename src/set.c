#include "set.h"
#include <string.h>
// @save

// This code is generated

#define value_cmp(value1, value2) ((value1) != (value2))

global u64 HASH_SEED = 0;

internal void set_set_seed(u64 seed)
{
    // | 1 to make it odd
    HASH_SEED = seed | 1;
}

internal u32 round_up_power_of_two(u32 capacity)
{
    capacity--;
    capacity |= capacity >> 1;
    capacity |= capacity >> 2;
    capacity |= capacity >> 4;
    capacity |= capacity >> 8;
    capacity |= capacity >> 16;
    return capacity + 1;
}

// @end

SetU64 set_create_u64(u32 capacity,
                      u64 (*hash_function)(const void* key, u32 len, u64 seed))
{
    capacity = max(round_up_power_of_two(capacity), 32);
    SetU64 out = {
        .cells = (SetCellU64*)calloc(capacity, sizeof(SetCellU64)),
        .capacity = capacity,
        .hash_function = hash_function,
    };
    return out;
}

SetCharPtr set_create_char_ptr(u32 capacity,
                               u64 (*hash_function)(const void* key, u32 len,
                                                    u64 seed))
{
    capacity = max(round_up_power_of_two(capacity), 32);
    SetCharPtr out = {
        .cells = (SetCellCharPtr*)calloc(capacity, sizeof(SetCellCharPtr)),
        .capacity = capacity,
        .hash_function = hash_function,
    };
    return out;
}

void set_clear_u64(SetU64* set)
{
    memset(set->cells, 0, set->capacity * sizeof(set->cells[0]));
}

void set_clear_char_ptr(SetCharPtr* set)
{
    memset(set->cells, 0, set->capacity * sizeof(set->cells[0]));
}

void set_insert_u64(SetU64* set, u64 key)
{
    u32 capacity_mask = set->capacity - 1;
    u64 hashed_index =
        set->hash_function(&key, (u32)sizeof(key), HASH_SEED) & capacity_mask;

    SetCellU64* cell = set->cells + hashed_index;
    if (cell->active)
    {
        if (value_cmp(cell->key, key) != 0)
        {
            cell = set->cells + (++hashed_index & capacity_mask);
            for (u32 i = 1; i < set->size && cell->active; ++i)
            {
                if (value_cmp(cell->key, key) == 0)
                {
                    goto add_node;
                }
                cell = set->cells + (++hashed_index & capacity_mask);
            }
        }
        else
        {
            goto add_node;
        }
    }
    cell->active = true;
    cell->deleted = false;
    cell->key = key;
add_node:
    if (set->size++ >= (u32)(set->capacity * 0.4f))
    {
        u32 old_capacity = set->capacity;
        SetCellU64* old_storage = set->cells;

        set->size = 0;
        set->capacity *= 2;
        set->cells = (SetCellU64*)calloc(set->capacity, sizeof(SetCellU64));

        for (u32 i = 0; i < old_capacity; i++)
        {
            cell = old_storage + i;
            if (cell->active)
            {
                set_insert_u64(set, cell->key);
            }
        }
        free(old_storage);
    }
}

void set_insert_char_ptr(SetCharPtr* set, char* key)
{
    u32 capacity_mask = set->capacity - 1;
    u64 hashed_index =
        set->hash_function(&key, (u32)strlen(key), HASH_SEED) & capacity_mask;

    SetCellCharPtr* cell = set->cells + hashed_index;
    if (cell->active)
    {
        if (strcmp(cell->key, key) != 0)
        {
            cell = set->cells + (++hashed_index & capacity_mask);
            for (u32 i = 1; i < set->size && cell->active; ++i)
            {
                if (strcmp(cell->key, key) == 0)
                {
                    goto add_node;
                }
                cell = set->cells + (++hashed_index & capacity_mask);
            }
        }
        else
        {
            goto add_node;
        }
    }
    cell->active = true;
    cell->deleted = false;
    cell->key = key;
add_node:
    if (set->size++ >= (u32)(set->capacity * 0.4f))
    {
        u32 old_capacity = set->capacity;
        SetCellCharPtr* old_storage = set->cells;

        set->size = 0;
        set->capacity *= 2;
        set->cells =
            (SetCellCharPtr*)calloc(set->capacity, sizeof(SetCellCharPtr));

        for (u32 i = 0; i < old_capacity; i++)
        {
            cell = old_storage + i;
            if (cell->active)
            {
                set_insert_char_ptr(set, cell->key);
            }
        }
        free(old_storage);
    }
}

b8 set_contains_u64(SetU64* set, const u64 key)
{
    u32 capacity_mask = set->capacity - 1;
    u64 hashed_index =
        set->hash_function(&key, (u32)sizeof(key), HASH_SEED) & capacity_mask;

    SetCellU64* cell = set->cells + hashed_index;
    if (cell->active || cell->deleted)
    {
        if (value_cmp(cell->key, key) != 0)
        {
            cell = set->cells + (++hashed_index & capacity_mask);
            for (u32 i = 0;
                 i < set->capacity && (cell->active || cell->deleted); ++i)
            {
                if (cell->active)
                {
                    if (value_cmp(cell->key, key) == 0)
                    {
                        return true;
                    }
                }
                cell = set->cells + (++hashed_index & capacity_mask);
            }
        }
        else
        {
            return true;
        }
    }
    return false;
}

b8 set_contains_char_ptr(SetCharPtr* set, const char* key)
{
    u32 capacity_mask = set->capacity - 1;
    u64 hashed_index =
        set->hash_function(&key, (u32)strlen(key), HASH_SEED) & capacity_mask;

    SetCellCharPtr* cell = set->cells + hashed_index;
    if (cell->active || cell->deleted)
    {
        if (strcmp(cell->key, key) != 0)
        {
            cell = set->cells + (++hashed_index & capacity_mask);
            for (u32 i = 0;
                 i < set->capacity && (cell->active || cell->deleted); ++i)
            {
                if (cell->active)
                {
                    if (strcmp(cell->key, key) == 0)
                    {
                        return true;
                    }
                }
                cell = set->cells + (++hashed_index & capacity_mask);
            }
        }
        else
        {
            return true;
        }
    }
    return false;
}

SetCellU64* set_remove_u64(SetU64* set, const u64 key)
{
    u32 capacity_mask = set->capacity - 1;
    u64 hashed_index =
        set->hash_function(&key, (u32)sizeof(key), HASH_SEED) & capacity_mask;

    SetCellU64* cell = set->cells + hashed_index;
    if (cell->active || cell->deleted)
    {
        if (value_cmp(cell->key, key) != 0)
        {
            cell = set->cells + (++hashed_index & capacity_mask);
            for (u32 i = 0;
                 i < set->capacity && (cell->active || cell->deleted); ++i)
            {
                if (cell->active)
                {
                    if (value_cmp(cell->key, key) == 0)
                    {
                        cell->active = false;
                        cell->deleted = true;
                        set->size--;
                        return cell;
                    }
                }
                cell = set->cells + (++hashed_index & capacity_mask);
            }
        }
        else
        {
            cell->active = false;
            cell->deleted = true;
            set->size--;
            return cell;
        }
    }
    return NULL;
}

SetCellCharPtr* set_remove_char_ptr(SetCharPtr* set, const char* key)
{
    u32 capacity_mask = set->capacity - 1;
    u64 hashed_index =
        set->hash_function(&key, (u32)strlen(key), HASH_SEED) & capacity_mask;

    SetCellCharPtr* cell = set->cells + hashed_index;
    if (cell->active || cell->deleted)
    {
        if (strcmp(cell->key, key) != 0)
        {
            cell = set->cells + (++hashed_index & capacity_mask);
            for (u32 i = 0;
                 i < set->capacity && (cell->active || cell->deleted); ++i)
            {
                if (cell->active)
                {
                    if (strcmp(cell->key, key) == 0)
                    {
                        cell->active = false;
                        cell->deleted = true;
                        set->size--;
                        return cell;
                    }
                }
                cell = set->cells + (++hashed_index & capacity_mask);
            }
        }
        else
        {
            cell->active = false;
            cell->deleted = true;
            set->size--;
            return cell;
        }
    }
    return NULL;
}

