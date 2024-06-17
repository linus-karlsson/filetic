#pragma once
#include "define.h"

template <Key, Value>
struct Cell
{
    Key key;
    Value value;
    bool active;
    bool deleted;
};

template <Key, Value>
struct HashTable
{
    Cell<Key, Value>* cells;
    u32 size;
    u32 capacity;

    u64 (*hash_function)(const void* key, u32 len, u64 seed);
};

template <Key, Value>
HashTable<Key, Value> hash_table_create(u32 capacity, u64 (*hash_function)(const void* key, u32 len, u64 seed))
{
    capacity = max(round_up_power_of_two(capacity), 32);
    HashTable<Key, Value> out = {
        .cells =
            (Cell<Key, Value>*)calloc(capacity, sizeof(Cell<Key, Value>)),
        .capacity = capacity,
        .hash_function = hash_function,
    };
    return out;
}

template <Key, Value, Len, Cmp>
void hash_table_insert(HashTable<Key, Value>* table, Key key, Value value)
{
    u32 capacity_mask = table->capacity - 1;
    u64 hashed_index =
        table->hash_function(&key, (u32)Len(key), HASH_SEED) & capacity_mask;

    Cell<Key, Value>* cell = table->cells + hashed_index;
    if (cell->active)
    {
        if (Cmp(cell->key, key) != 0)
        {
            cell = table->cells + (++hashed_index & capacity_mask);
            for (u32 i = 0; i < table->size && cell->active; i++)
            {
                if (Cmp(cell->key, key) == 0)
                {
                    goto add_node;
                }
                cell = table->cells + (++hashed_index & capacity_mask);
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
    cell->value = value;
    if (table->size++ >= (u32)(table->capacity * 0.4f))
    {
        u32 old_capacity = table->capacity;
        Cell<Key, Value>* old_storage = table->cells;

        table->size = 0;
        table->capacity *= 2;
        table->cells = 
            (Cell<Key, Value>*)calloc(table->capacity, sizeof(Cell<Key, Value>));

        for (u32 i = 0; i < old_capacity; i++)
        {
            cell = old_storage + i;
            if (cell->active)
            {
                hash_table_insert(table, cell->key, cell->value);
            }
        }
        free(old_storage);
    }
}

template <Key, Value, Len, Cmp>
Value* hash_table_get(HashTable<Key, Value>* table, const Key key)
{
    u32 capacity_mask = table->capacity - 1;
    u64 hashed_index =
        table->hash_function(&key, (u32)Len(key), HASH_SEED) & capacity_mask;

    Cell<Key, Value>* cell = table->cells + hashed_index;
    if (cell->active)
    {
        if (Cmp(cell->key, key) != 0)
        {
            cell = table->cells + (++hashed_index & capacity_mask);
            for (u32 i = 0; i < table->size && (cell->active || cell->deleted);
                 i++)
            {
                if (Cmp(cell->key, key) == 0)
                {
                    return &cell->value;
                }
                cell = table->cells + (++hashed_index & capacity_mask);
            }
        }
        else
        {
            return &cell->value;
        }
    }
    return NULL;
}

template <Key, Value, Len, Cmp>
Cell<Key, Value>* hash_table_remove(HashTable<Key, Value>* table, const Key key)
{
    u32 capacity_mask = table->capacity - 1;
    u64 hashed_index =
        table->hash_function(&key, (u32)Len(key), HASH_SEED) & capacity_mask;

    Cell<Key, Value>* cell = table->cells + hashed_index;
    if (cell->active)
    {
        if (Cmp(cell->key, key) != 0)
        {
            cell = table->cells + (++hashed_index & capacity_mask);
            for (u32 i = 0; i < table->size && (cell->active || cell->deleted);
                 i++)
            {
                if (Cmp(cell->key, key) == 0)
                {
                    cell->active = false;
                    cell->deleted = true;
                    table->size--;
                    return cell;
                }
                cell = table->cells + (++hashed_index & capacity_mask);
            }
        }
        else
        {
            cell->active = false;
            cell->deleted = true;
            table->size--;
            return cell;
        }
    }
    return NULL;
}
