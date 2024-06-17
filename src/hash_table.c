#include "hash_table.h"
#include <string.h>
// @save

// This code is generated

#define value_cmp(value1, value2) ((value1) != (value2))

global u64 HASH_SEED = 0;

void hash_table_set_seed(u64 seed)
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

HashTableUU64 hash_table_create_uu64(u32 capacity,
                                     u64 (*hash_function)(const void* key,
                                                          u32 len, u64 seed))
{
    capacity = max(round_up_power_of_two(capacity), 32);
    HashTableUU64 out = {
        .cells = (CellUU64*)calloc(capacity, sizeof(CellUU64)),
        .capacity = capacity,
        .hash_function = hash_function,
    };
    return out;
}

HashTableCharU32 hash_table_create_char_u32(
    u32 capacity, u64 (*hash_function)(const void* key, u32 len, u64 seed))
{
    capacity = max(round_up_power_of_two(capacity), 32);
    HashTableCharU32 out = {
        .cells = (CellCharU32*)calloc(capacity, sizeof(CellCharU32)),
        .capacity = capacity,
        .hash_function = hash_function,
    };
    return out;
}

void hash_table_clear_uu64(HashTableUU64* hash_table)
{
    memset(hash_table->cells, 0,
           hash_table->capacity * sizeof(hash_table->cells[0]));
}

void hash_table_clear_char_u32(HashTableCharU32* hash_table)
{
    memset(hash_table->cells, 0,
           hash_table->capacity * sizeof(hash_table->cells[0]));
}

void hash_table_insert_uu64(HashTableUU64* table, u64 key, u64 value)
{
    u32 capacity_mask = table->capacity - 1;
    u64 hashed_index =
        table->hash_function(&key, (u32)sizeof(key), HASH_SEED) & capacity_mask;

    CellUU64* cell = table->cells + hashed_index;
    if (cell->active)
    {
        if (value_cmp(cell->key, key) != 0)
        {
            cell = table->cells + (++hashed_index & capacity_mask);
            for (u32 i = 0; i < table->size && cell->active; i++)
            {
                if (value_cmp(cell->key, key) == 0)
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
        CellUU64* old_storage = table->cells;

        table->size = 0;
        table->capacity *= 2;
        table->cells = (CellUU64*)calloc(table->capacity, sizeof(CellUU64));

        for (u32 i = 0; i < old_capacity; i++)
        {
            cell = old_storage + i;
            if (cell->active)
            {
                hash_table_insert_uu64(table, cell->key, cell->value);
            }
        }
        free(old_storage);
    }
}

void hash_table_insert_char_u32(HashTableCharU32* table, char* key, u32 value)
{
    u32 capacity_mask = table->capacity - 1;
    u64 hashed_index =
        table->hash_function(&key, (u32)strlen(key), HASH_SEED) & capacity_mask;

    CellCharU32* cell = table->cells + hashed_index;
    if (cell->active)
    {
        if (strcmp(cell->key, key) != 0)
        {
            cell = table->cells + (++hashed_index & capacity_mask);
            for (u32 i = 0; i < table->size && cell->active; i++)
            {
                if (strcmp(cell->key, key) == 0)
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
        CellCharU32* old_storage = table->cells;

        table->size = 0;
        table->capacity *= 2;
        table->cells =
            (CellCharU32*)calloc(table->capacity, sizeof(CellCharU32));

        for (u32 i = 0; i < old_capacity; i++)
        {
            cell = old_storage + i;
            if (cell->active)
            {
                hash_table_insert_char_u32(table, cell->key, cell->value);
            }
        }
        free(old_storage);
    }
}

u64* hash_table_get_uu64(HashTableUU64* table, const u64 key)
{
    u32 capacity_mask = table->capacity - 1;
    u64 hashed_index =
        table->hash_function(&key, (u32)sizeof(key), HASH_SEED) & capacity_mask;

    CellUU64* cell = table->cells + hashed_index;
    if (cell->active)
    {
        if (value_cmp(cell->key, key) != 0)
        {
            cell = table->cells + (++hashed_index & capacity_mask);
            for (u32 i = 0; i < table->size && (cell->active || cell->deleted);
                 i++)
            {
                if (value_cmp(cell->key, key) == 0)
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

u32* hash_table_get_char_u32(HashTableCharU32* table, const char* key)
{
    u32 capacity_mask = table->capacity - 1;
    u64 hashed_index =
        table->hash_function(&key, (u32)strlen(key), HASH_SEED) & capacity_mask;

    CellCharU32* cell = table->cells + hashed_index;
    if (cell->active)
    {
        if (strcmp(cell->key, key) != 0)
        {
            cell = table->cells + (++hashed_index & capacity_mask);
            for (u32 i = 0; i < table->size && (cell->active || cell->deleted);
                 i++)
            {
                if (strcmp(cell->key, key) == 0)
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

CellUU64* hash_table_remove_uu64(HashTableUU64* table, const u64 key)
{
    u32 capacity_mask = table->capacity - 1;
    u64 hashed_index =
        table->hash_function(&key, (u32)sizeof(key), HASH_SEED) & capacity_mask;

    CellUU64* cell = table->cells + hashed_index;
    if (cell->active)
    {
        if (value_cmp(cell->key, key) != 0)
        {
            cell = table->cells + (++hashed_index & capacity_mask);
            for (u32 i = 0; i < table->size && (cell->active || cell->deleted);
                 i++)
            {
                if (value_cmp(cell->key, key) == 0)
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

CellCharU32* hash_table_remove_char_u32(HashTableCharU32* table,
                                        const char* key)
{
    u32 capacity_mask = table->capacity - 1;
    u64 hashed_index =
        table->hash_function(&key, (u32)strlen(key), HASH_SEED) & capacity_mask;

    CellCharU32* cell = table->cells + hashed_index;
    if (cell->active)
    {
        if (strcmp(cell->key, key) != 0)
        {
            cell = table->cells + (++hashed_index & capacity_mask);
            for (u32 i = 0; i < table->size && (cell->active || cell->deleted);
                 i++)
            {
                if (strcmp(cell->key, key) == 0)
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

