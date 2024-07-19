#include "opengl_util.h"
#include "texture.h"
#include <glad/glad.h>
#include <math.h>

internal AABB set_up_verticies(VertexArray* vertex_array, V2 position, V2 size,
                               V4 color, f32 texture_index,
                               TextureCoordinates texture_coordinates)
{
    position = v2f(round_f32(position.x), round_f32(position.y));
    size = v2f(round_f32(size.x), round_f32(size.y));

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
    out.min = position;
    out.size = size;
    return out;
}


AABB quad_co(VertexArray* vertex_array, V2 position, V2 size, V4 color,
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

AABB quad(VertexArray* vertex_array, V2 position, V2 size, V4 color,
          f32 texture_index)
{
    TextureCoordinates texture_coordinates = { v2d(), v2f(0.0f, 1.0f),
                                               v2f(1.0f, 1.0f),
                                               v2f(1.0f, 0.0f) };
    return set_up_verticies(vertex_array, position, size, color, texture_index,
                            texture_coordinates);
}

AABB quad_shadow(VertexArray* vertex_array, V2 position, V2 size, V4 color,
                 f32 texture_index)
{
    V4 shadow_color = v4ic(0.0f);
    V4 end_color = shadow_color;
    end_color.a = 0.6f;
    V2 shadow_position = v2_s_add(position, 2.5f);
    quad_gradiant_tl_br(vertex_array, shadow_position, size, shadow_color,
                        end_color, 0.0f);
    return quad(vertex_array, position, size, color, texture_index);
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

internal AABB quad_gradiant_internal(VertexArray* vertex_array, V2 position,
                                     V2 size, V4 corner_colors[4],
                                     f32 texture_index)
{
    TextureCoordinates texture_coordinates = { v2d(), v2f(0.0f, 1.0f),
                                               v2f(1.0f, 1.0f),
                                               v2f(1.0f, 0.0f) };

    position = v2f(round_f32(position.x), round_f32(position.y));
    size = v2f(round_f32(size.x), round_f32(size.y));
    Vertex vertex = {
        .position = position,
        .texture_coordinates = texture_coordinates.coordinates[0],
        .color = corner_colors[0],
        .texture_index = texture_index,
    };
    array_push(vertex_array, vertex);
    vertex.position.y += size.y;
    vertex.texture_coordinates = texture_coordinates.coordinates[1];
    vertex.color = corner_colors[1];
    array_push(vertex_array, vertex);
    vertex.position.x += size.x;
    vertex.texture_coordinates = texture_coordinates.coordinates[2];
    vertex.color = corner_colors[2];
    array_push(vertex_array, vertex);
    vertex.position.y -= size.y;
    vertex.texture_coordinates = texture_coordinates.coordinates[3];
    vertex.color = corner_colors[3];
    array_push(vertex_array, vertex);

    AABB out = {
        .min = position,
        .size = size,
    };
    return out;
}

AABB quad_gradiant_l_r(VertexArray* vertex_array, V2 position, V2 size,
                       V4 left_color, V4 right_color, f32 texture_index)
{
    V4 corner_colors[] = { left_color, left_color, right_color, right_color };
    return quad_gradiant_internal(vertex_array, position, size, corner_colors,
                                  texture_index);
}

AABB quad_gradiant_t_b(VertexArray* vertex_array, V2 position, V2 size,
                       V4 top_color, V4 bottom_color, f32 texture_index)
{
    V4 corner_colors[] = { top_color, bottom_color, bottom_color, top_color };
    return quad_gradiant_internal(vertex_array, position, size, corner_colors,
                                  texture_index);
}

AABB quad_gradiant_tl_br(VertexArray* vertex_array, V2 position, V2 size,
                         V4 top_color, V4 bottom_color, f32 texture_index)
{
    V4 half_color = v4_lerp(top_color, bottom_color, 0.5f);
    V4 corner_colors[] = { top_color, half_color, bottom_color, half_color };
    return quad_gradiant_internal(vertex_array, position, size, corner_colors,
                                  texture_index);
}

AABB quad_border_gradiant(VertexArray* vertex_array, u32* num_indices,
                          V2 top_left, V2 size, V4 border_color_top_left,
                          V4 border_color_bottom_right, f32 thickness,
                          f32 texture_index)
{
    V2 h_size = v2f(size.x, thickness);
    V2 v_size = v2f(thickness, size.y);

    V4 border_color_top_right =
        v4_lerp(border_color_top_left, border_color_bottom_right, 0.5f);
    quad_gradiant_l_r(vertex_array, top_left, h_size, border_color_top_left,
                      border_color_top_right, texture_index);

    quad_gradiant_l_r(vertex_array,
                      v2f(top_left.x, top_left.y + v_size.y - thickness),
                      h_size, border_color_top_right, border_color_bottom_right,
                      texture_index);

    quad_gradiant_t_b(vertex_array, top_left, v_size, border_color_top_left,
                      border_color_top_right, texture_index);

    quad_gradiant_t_b(vertex_array,
                      v2f(top_left.x + h_size.x - thickness, top_left.y),
                      v_size, border_color_top_right, border_color_bottom_right,
                      texture_index);

    if (num_indices)
    {
        *num_indices += 4 * 6;
    }
    AABB out = { 0 };
    out.min = top_left;
    out.size = size;
    return out;
}

AABB quad_border(VertexArray* vertex_array, u32* num_indices, V2 top_left,
                 V2 size, V4 color, f32 thickness, f32 texture_index)
{
    V2 h_size = v2f(size.x, thickness);
    V2 v_size = v2f(thickness, size.y);

    quad(vertex_array, top_left, h_size, color, texture_index);

    quad(vertex_array, v2f(top_left.x, top_left.y + v_size.y - thickness),
         h_size, color, texture_index);

    quad(vertex_array, top_left, v_size, color, texture_index);

    quad(vertex_array, v2f(top_left.x + h_size.x - thickness, top_left.y),
         v_size, color, texture_index);

    if (num_indices)
    {
        *num_indices += 4 * 6;
    }
    AABB out = { 0 };
    out.min = top_left;
    out.size = size;
    return out;
}

V2 get_position(VertexArray* vertex_array, const V2 focal, const f32 half_size,
                const f32 thickness, const f32 degree)
{
    return v2d();
}

AABB quad_border_rounded(VertexArray* vertex_array, u32* num_indices,
                         V2 top_left, V2 size, V4 color, f32 thickness,
                         f32 roundness, u32 samples_per_side, f32 texture_index)
{

    V2 h_size = v2f(size.x, thickness);
    V2 v_size = v2f(thickness, size.y);
    f32 half_v_size = v_size.y * 0.5f;
    const f32 pivot_offset = lerp_f32(0.0f, half_v_size, roundness);

    top_left.x += pivot_offset;
    h_size.width -= pivot_offset * 2;

    V2 pivot_point_up = top_left;
    pivot_point_up.y += pivot_offset;
    V2 pivot_point_down = v2f(top_left.x, top_left.y + v_size.y);
    pivot_point_down.y -= pivot_offset;

    quad(vertex_array, top_left, h_size, color, texture_index);

    quad(vertex_array, v2f(top_left.x, top_left.y + v_size.y - thickness),
         h_size, color, texture_index);

    quad(vertex_array, v2f(pivot_point_up.x - pivot_offset, pivot_point_up.y),
         v2f(thickness, pivot_point_down.y - pivot_point_up.y), color, texture_index);

    TextureCoordinates texture_coordinates = { v2d(), v2f(0.0f, 1.0f),
                                               v2f(1.0f, 1.0f),
                                               v2f(1.0f, 0.0f) };

    V2* start_pivot = &pivot_point_up;
    V2* end_pivot = &pivot_point_down;
    f32 degree = PI * 0.5f;
    f32 degree_increase = degree / samples_per_side;
    for (u32 i = 0; i < 2; ++i)
    {
        V2 pivot_point = *start_pivot;
        for (u32 j = 0; j < 2; ++j)
        {
            for (u32 k = 0; k < samples_per_side; ++k)
            {

                Vertex vertex = {
                    .position = v2d(),
                    .texture_coordinates = texture_coordinates.coordinates[0],
                    .color = color,
                    .texture_index = texture_index,
                };
                vertex.position = pivot_point;
                vertex.position.x += cosf(degree) * pivot_offset;
                vertex.position.y -= sinf(degree) * pivot_offset;
                array_push(vertex_array, vertex);

                vertex.position = pivot_point;
                vertex.position.x += cosf(degree) * (pivot_offset - thickness);
                vertex.position.y -= sinf(degree) * (pivot_offset - thickness);
                vertex.texture_coordinates = texture_coordinates.coordinates[1];
                array_push(vertex_array, vertex);

                degree += degree_increase;

                vertex.position = pivot_point;
                vertex.position.x += cosf(degree) * pivot_offset;
                vertex.position.y -= sinf(degree) * pivot_offset;
                vertex.texture_coordinates = texture_coordinates.coordinates[2];
                array_push(vertex_array, vertex);

                vertex.position = pivot_point;
                vertex.position.x += cosf(degree) * (pivot_offset - thickness);
                vertex.position.y -= sinf(degree) * (pivot_offset - thickness);
                vertex.texture_coordinates = texture_coordinates.coordinates[3];
                array_push(vertex_array, vertex);
            }
            pivot_point = *end_pivot;
        }
        pivot_point_up.x += h_size.x;
        pivot_point_down.x += h_size.x;

        start_pivot = &pivot_point_down;
        end_pivot = &pivot_point_up;

        degree = 1.5f * PI;
    }
    pivot_point_up.x -= h_size.x;

    quad(vertex_array, v2f(pivot_point_up.x + pivot_offset - thickness, pivot_point_up.y),
         v2f(thickness, pivot_point_down.y - pivot_point_up.y), color, texture_index);

    if (num_indices)
    {
        *num_indices += (4 + (4 * samples_per_side)) * 6;
    }
    AABB out = { 0 };
    out.min = top_left;
    out.size = size;
    return out;
}

u32 create_default_texture()
{
    u8 pixels[4] = { UINT8_MAX, UINT8_MAX, UINT8_MAX, UINT8_MAX };
    TextureProperties texture_properties = {
        .width = 1,
        .height = 1,
        .bytes = pixels,
    };
    return texture_create(&texture_properties, GL_RGBA8, GL_RGBA);
}

u32 load_icon_as_white(const char* file_path)
{
    TextureProperties texture_properties = { 0 };
    texture_load(file_path, &texture_properties);
    const u32 size = texture_properties.height * texture_properties.width;
    const u32 size_in_bytes = size * 4;
    for (u32 i = 0; i < size_in_bytes; i += 4)
    {
        texture_properties.bytes[i] = UINT8_MAX;
        texture_properties.bytes[i + 1] = UINT8_MAX;
        texture_properties.bytes[i + 2] = UINT8_MAX;
    }
    u32 icon_texture = texture_create(&texture_properties, GL_RGBA8, GL_RGBA);
    free(texture_properties.bytes);
    return icon_texture;
}

u32 load_icon(const char* file_path)
{
    TextureProperties texture_properties = { 0 };
    texture_load(file_path, &texture_properties);
    u32 icon_texture = texture_create(&texture_properties, GL_RGBA8, GL_RGBA);
    free(texture_properties.bytes);
    return icon_texture;
}
