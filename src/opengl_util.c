#include "opengl_util.h"

internal AABB set_up_verticies(VertexArray* vertex_array, V3 position, V2 size,
                               V4 color, f32 texture_index,
                               TextureCoordinates texture_coordinates)
{
    Vertex vertex = {
        .position = position,
        .texture_coordinates = texture_coordinates.coordinates[0],
        .color = color,
        .texture_index = texture_index,
    };
    array_push(vertex_array, vertex);
    vertex.position.y += size.y;
    vertex.texture_coordinates = texture_coordinates.coordinates[1];
    array_push(vertex_array, vertex);
    vertex.position.x += size.x;
    vertex.texture_coordinates = texture_coordinates.coordinates[2];
    array_push(vertex_array, vertex);
    vertex.position.y -= size.y;
    vertex.texture_coordinates = texture_coordinates.coordinates[3];
    array_push(vertex_array, vertex);

    AABB out;
    out.min = v2_v3(position);
    out.size = size;
    return out;
}

AABB quad_co(VertexArray* vertex_array, V3 position, V2 size, V4 color,
             V4 texture_coordinates, f32 texture_index)
{
    TextureCoordinates _tex_coords = {
        v2_v4(texture_coordinates),
        v2f(texture_coordinates.x, texture_coordinates.w),
        v2f(texture_coordinates.z, texture_coordinates.w),
        v2f(texture_coordinates.z, texture_coordinates.y)
    };
    return set_up_verticies(vertex_array, position, size, color, texture_index,
                            _tex_coords);
}

AABB quad(VertexArray* vertex_array, V3 position, V2 size, V4 color,
          f32 texture_index)
{
    TextureCoordinates texture_coordinates = { v2d(), v2f(0.0f, 1.0f),
                                               v2f(1.0f, 1.0f),
                                               v2f(1.0f, 0.0f) };
    return set_up_verticies(vertex_array, position, size, color, texture_index,
                            texture_coordinates);
}

void generate_indicies(IndexArray* array, u32 offset, u32 indices_count)
{
    const u32 INDEX_TABLE[] = { 0, 1, 2, 2, 3, 0 };
    for (u32 i = 0; i < indices_count; i++)
    {
        for (u32 j = 0; j < 6; j++)
        {
            array_push(array, ((INDEX_TABLE[j] + (4 * i)) + offset));
        }
    }
}

AABB quad_gradiant_l_r(VertexArray* vertex_array, V3 position, V2 size,
                       V4 left_color, V4 right_color, f32 texture_index)
{
    TextureCoordinates texture_coordinates = { v2d(), v2f(0.0f, 1.0f),
                                               v2f(1.0f, 1.0f),
                                               v2f(1.0f, 0.0f) };
    Vertex vertex = {
        .position = position,
        .texture_coordinates = texture_coordinates.coordinates[0],
        .color = left_color,
        .texture_index = texture_index,
    };
    array_push(vertex_array, vertex);
    vertex.position.y += size.y;
    vertex.texture_coordinates = texture_coordinates.coordinates[1];
    array_push(vertex_array, vertex);
    vertex.position.x += size.x;
    vertex.texture_coordinates = texture_coordinates.coordinates[2];
    vertex.color = right_color;
    array_push(vertex_array, vertex);
    vertex.position.y -= size.y;
    vertex.texture_coordinates = texture_coordinates.coordinates[3];
    array_push(vertex_array, vertex);

    AABB out = {
        .min = v2_v3(position),
        .size = size,
    };
    return out;
}

AABB quad_gradiant_t_b(VertexArray* vertex_array, V3 position, V2 size,
                       V4 top_color, V4 bottom_color, f32 texture_index)
{
    TextureCoordinates texture_coordinates = { v2d(), v2f(0.0f, 1.0f),
                                               v2f(1.0f, 1.0f),
                                               v2f(1.0f, 0.0f) };
    Vertex vertex = {
        .position = position,
        .texture_coordinates = texture_coordinates.coordinates[0],
        .color = top_color,
        .texture_index = texture_index,
    };
    array_push(vertex_array, vertex);
    vertex.position.y += size.y;
    vertex.texture_coordinates = texture_coordinates.coordinates[1];
    vertex.color = bottom_color;
    array_push(vertex_array, vertex);
    vertex.position.x += size.x;
    vertex.texture_coordinates = texture_coordinates.coordinates[2];
    vertex.color = bottom_color;
    array_push(vertex_array, vertex);
    vertex.position.y -= size.y;
    vertex.texture_coordinates = texture_coordinates.coordinates[3];
    vertex.color = top_color;
    array_push(vertex_array, vertex);

    AABB out = {
        .min = v2_v3(position),
        .size = size,
    };
    return out;
}

V4 v4_lerp(V4 v1, V4 v2, f32 t)
{
    return v4_add(v1, v4_s_multi(v4_sub(v2, v1), t));
}

AABB quad_border_gradiant(VertexArray* vertex_array, u32* num_indices,
                          V3 top_left, V2 size, V4 border_color_top_left,
                          V4 border_color_bottom_right, f32 thickness,
                          f32 tex_index)
{
    V2 h_size = v2f(size.x, thickness);
    V2 v_size = v2f(thickness, size.y);

    V4 border_color_top_right =
        v4_lerp(border_color_top_left, border_color_bottom_right, 0.5f);
    quad_gradiant_l_r(vertex_array, top_left, h_size, border_color_top_left,
                      border_color_top_right, tex_index);

    quad_gradiant_l_r(
        vertex_array,
        v3f(top_left.x, top_left.y + v_size.y - thickness, top_left.z), h_size,
        border_color_top_right, border_color_bottom_right, tex_index);

    quad_gradiant_t_b(vertex_array, top_left, v_size, border_color_top_left,
                      border_color_top_right, tex_index);

    quad_gradiant_t_b(
        vertex_array,
        v3f(top_left.x + h_size.x - thickness, top_left.y, top_left.z), v_size,
        border_color_top_right, border_color_bottom_right, tex_index);

    if (num_indices)
    {
        *num_indices += 4;
    }
    AABB out = { 0 };
    out.min = v2_v3(top_left);
    out.size = size;
    return out;
}

AABB quad_border(VertexArray* vertex_array, u32* num_indices, V3 top_left,
                 V2 size, V4 border_color, f32 thickness, f32 tex_index)
{
    V2 h_size = v2f(size.x, thickness);
    V2 v_size = v2f(thickness, size.y);

    quad(vertex_array, top_left, h_size, border_color, tex_index);

    quad(vertex_array,
         v3f(top_left.x, top_left.y + v_size.y - thickness, top_left.z), h_size,
         border_color, tex_index);

    quad(vertex_array, top_left, v_size, border_color, tex_index);

    quad(vertex_array,
         v3f(top_left.x + h_size.x - thickness, top_left.y, top_left.z), v_size,
         border_color, tex_index);

    if (num_indices)
    {
        *num_indices += 4;
    }
    AABB out = { 0 };
    out.min = v2_v3(top_left);
    out.size = size;
    return out;
}
