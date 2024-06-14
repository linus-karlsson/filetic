#pragma once
#include "define.h"
#include "util.h"
#include "collision.h"

typedef struct TextureCoordinates
{
    V2 coordinates[4];
} TextureCoordinates;

AABB quad_co(VertexArray* vertex_array, V3 position, V2 size, V4 color,
             V4 texture_coordinates, f32 texture_index);

AABB quad(VertexArray* vertex_array, V3 position, V2 size, V4 color,
          f32 texture_index);

void generate_indicies(IndexArray* array, u32 offset, u32 indices_count);
