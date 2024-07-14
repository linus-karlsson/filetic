#pragma once
#include "define.h"

template <Key>
struct SetCell
{
    Key key;
    bool active;
    bool deleted;
};

template <Key>
struct Set 
{
    SetCell<Key>* cells;
    u32 size;
    u32 capacity;

    u64 (*hash_function)(const void* key, u32 len, u64 seed);
};

template <Key>
Set<Key> set_create(u32 capacity, u64 (*hash_function)(const void* key, u32 len, u64 seed))
{
    capacity = max(round_up_power_of_two(capacity), 32);
    Set<Key> out = {
        .cells = (SetCell<Key>*)calloc(capacity, sizeof(SetCell<Key>)),
        .capacity = capacity,
        .hash_function = hash_function,
    };
    return out;
}

template <Key, Len, Cmp>
void set_insert(Set<Key>* set, Key key)
{
    u32 capacity_mask = set->capacity - 1;
    u64 hashed_index =
        set->hash_function(&key, (u32)Len(key), HASH_SEED) & capacity_mask;

    SetCell<Key>* cell = set->cells + hashed_index;
    if (cell->active)
    {
        if (Cmp(cell->key, key) != 0)
        {
            cell = set->cells + (++hashed_index & capacity_mask);
            for (u32 i = 1; i < set->size && cell->active; ++i)
            {
                if (Cmp(cell->key, key) == 0)
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
        SetCell<Key>* old_storage = set->cells;

        set->size = 0;
        set->capacity *= 2;
        set->cells = (SetCell<Key>*)calloc(set->capacity,
                                                 sizeof(SetCell<Key>));

        for (u32 i = 0; i < old_capacity; i++)
        {
            cell = old_storage + i;
            if (cell->active)
            {
                set_insert(set, cell->key);
            }
        }
        free(old_storage);
    }
}

template <Key, Len, Cmp>
b8 set_contains(Set<Key>* set, const Key key)
{
    u32 capacity_mask = set->capacity - 1;
    u64 hashed_index =
        set->hash_function(&key, (u32)Len(key), HASH_SEED) & capacity_mask;

    SetCell<Key>* cell = set->cells + hashed_index;
    if (cell->active || cell->deleted)
    {
        if (Cmp(cell->key, key) != 0)
        {
            cell = set->cells + (++hashed_index & capacity_mask);
            for (u32 i = 0;
                 i < set->capacity && (cell->active || cell->deleted); ++i)
            {
                if (cell->active)
                {
                    if (Cmp(cell->key, key) == 0)
                    {
                        return true;
                    }
                }
                cell = set->cells + (++hashed_index & capacity_mask);
            }
        }
        else if(cell->active)
        {
            return true;
        }
    }
    return false;
}

template <Key, Len, Cmp>
SetCell<Key>* set_remove(Set<Key>* set, const Key key)
{
    u32 capacity_mask = set->capacity - 1;
    u64 hashed_index =
        set->hash_function(&key, (u32)Len(key), HASH_SEED) & capacity_mask;

    SetCell<Key>* cell = set->cells + hashed_index;
    if (cell->active || cell->deleted)
    {
        if (Cmp(cell->key, key) != 0)
        {
            cell = set->cells + (++hashed_index & capacity_mask);
            for (u32 i = 0;
                 i < set->capacity && (cell->active || cell->deleted); ++i)
            {
                if (cell->active)
                {
                    if (Cmp(cell->key, key) == 0)
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
        else if(cell->active)
        {
            cell->active = false;
            cell->deleted = true;
            set->size--;
            return cell;
        }
    }
    return NULL;
}

template <Key>
void set_clear(Set<Key>* set)
{
    memset(set->cells, 0, set->capacity * sizeof(set->cells[0]));
}
