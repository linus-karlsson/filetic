#pragma once
#include "define.h"
#include "util.h"
#include "collision.h"

typedef struct TextureCoordinates
{
    V2 coordinates[4];
} TextureCoordinates;

AABB set_up_verticies(VertexArray* vertex_array, V2 position, V2 size, V4 color, f32 texture_index, TextureCoordinates texture_coordinates);
AABB set_up_verticies_color(VertexArray* vertex_array, V2 position, V2 size, V4 color[4], f32 texture_index, TextureCoordinates texture_coordinates);
V4 quad_get_gradiant_texture_coordinates();
AABB quad_co(VertexArray* vertex_array, V2 position, V2 size, V4 color, V4 texture_coordinates, f32 texture_index);
AABB quad(VertexArray* vertex_array, V2 position, V2 size, V4 color, f32 texture_index);
AABB quad_shadow(VertexArray* vertex_array, V2 position, V2 size, V4 color, f32 texture_index);
AABB quad_border(VertexArray* vert_array, u32* num_indices, V2 top_left, V2 size, V4 border_color, f32 thickness, f32 tex_index);
AABB quad_border_rounded(VertexArray* vertex_array, u32* num_indices, V2 top_left, V2 size, V4 border_color, f32 thickness, f32 roundness, u32 samples_per_side, f32 texture_index);
AABB quad_gradiant_l_r(VertexArray* vertex_array, V2 position, V2 size, V4 left_color, V4 right_color, f32 texture_index);
AABB quad_gradiant_t_b(VertexArray* vertex_array, V2 position, V2 size, V4 top_color, V4 bottom_color, f32 texture_index);
AABB quad_gradiant_tl_br(VertexArray* vertex_array, V2 position, V2 size, V4 top_color, V4 bottom_color, f32 texture_index);
AABB quad_border_gradiant(VertexArray* vertex_array, u32* num_indices, V2 top_left, V2 size, V4 border_color_top_left, V4 border_color_bottom_right, f32 thickness, f32 tex_index);
void generate_indicies(IndexArray* array, u32 offset, u32 indices_count);

u32 create_default_texture();
u32 load_icon_as_white(const char* file_path);
u32 load_icon_as_white_resize(const char* file_path, i32 width, i32 height);
u32 load_icon(const char* file_path);
u32 load_icon_and_resize(const char* file_path, i32 width, i32 height);

TextureCoordinates default_texture_coordinates();
TextureCoordinates flip_texture_coordinates();
