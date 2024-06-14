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
