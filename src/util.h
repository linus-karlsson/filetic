#pragma once
#include "define.h"
#include "math/ftic_math.h"

typedef struct Vertex
{
    V4 color;
    V3 position;
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


FileAttrib read_file(const char* file_path);
