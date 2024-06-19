#pragma once
#include "define.h"
#include "util.h"
#include "collision.h"

typedef struct TextureCoordinates
{
    V2 coordinates[4];
} TextureCoordinates;

AABB quad_co(VertexArray* vertex_array, V3 position, V2 size, V4 color, V4 texture_coordinates, f32 texture_index);
AABB quad(VertexArray* vertex_array, V3 position, V2 size, V4 color, f32 texture_index);
AABB quad_with_border(VertexArray* vert_array, u32* num_indices, V3 top_left, V2 size, V4 border_color, f32 thickness, f32 tex_index);
void generate_indicies(IndexArray* array, u32 offset, u32 indices_count);
