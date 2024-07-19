#pragma once
#include "define.h"
#include "math/ftic_math.h"

typedef struct Indices
{
    u32 vertex_index;
    u32 texture_index;
    u32 normal_index;
} Indices;

typedef struct V3Array
{
    u32 size;
    u32 capacity;
    V3* data;
} V3Array;

typedef struct V2Array
{
    u32 size;
    u32 capacity;
    V2* data;
} V2Array;

typedef struct IndicesArray
{
    u32 size;
    u32 capacity;
    Indices* data;
} IndicesArray;

typedef struct ObjectLoad
{
    V3Array vertex_positions;
    V3Array normals;
    V2Array texture_coordinates;

    IndicesArray indices;
} ObjectLoad;

void object_load_model(ObjectLoad* object_load, const char* model_path);
void object_load_free(ObjectLoad* object_load);
