#pragma once
#include "define.h"
#include "math/ftic_math.h"

typedef struct Vertex
{
    V4 color;
    V2 position;
    V2 texture_coordinates;
    f32 texture_index;
} Vertex;

typedef struct IndexArray
{
    u32 size;
    u32 capacity;
    u32* data;
} IndexArray;

typedef struct VertexArray
{
    u32 size;
    u32 capacity;
    Vertex* data;
} VertexArray;

typedef struct FileAttrib
{
    u8* buffer;
    u64 size;
} FileAttrib;

typedef struct CharArray
{
    u32 size;
    u32 capacity;
    char* data;
} CharArray;

FileAttrib read_file(const char* file_path);
f32 lerp_f32(const f32 a, const f32 b, const f32 t);
V4 v4_lerp(V4 v1, V4 v2, f32 t);
