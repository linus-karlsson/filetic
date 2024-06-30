#include "rendering.h"
#include "opengl_util.h"
#include "texture.h"
#include <glad/glad.h>

RenderingProperties
rendering_properties_initialize(u32 shader_id, u32* textures, u32 texture_count,
                                const VertexBufferLayout* vertex_buffer_layout,
                                u32 vertex_buffer_size, u32 index_buffer_size)
{
    RenderingProperties rendering_properties = { 0 };
    array_create(&rendering_properties.vertices, vertex_buffer_size);
    rendering_properties.vertex_array_id = vertex_array_create();
    vertex_array_bind(rendering_properties.vertex_array_id);

    rendering_properties.vertex_buffer_id =
        vertex_buffer_create(NULL, rendering_properties.vertices.capacity,
                             sizeof(Vertex), GL_STREAM_DRAW);
    rendering_properties.vertex_buffer_capacity =
        rendering_properties.vertices.capacity;
    vertex_array_add_buffer(rendering_properties.vertex_array_id,
                            rendering_properties.vertex_buffer_id,
                            vertex_buffer_layout);


    array_create(&rendering_properties.indices, index_buffer_size);
    generate_indicies(&rendering_properties.indices, 0, index_buffer_size / 6);
    u32 index_buffer_id = index_buffer_create(rendering_properties.indices.data,
                                              rendering_properties.indices.size,
                                              sizeof(u32), GL_STATIC_DRAW);
    free(rendering_properties.indices.data);

    rendering_properties.index_buffer_id = index_buffer_id;
    rendering_properties.texture_count = texture_count;
    rendering_properties.textures = textures;
    rendering_properties.shader_properties =
        shader_create_properties(shader_id, "proj", "view", "model");

    return rendering_properties;
}

void rendering_properties_clear(RenderingProperties* rendering_properties)
{
    rendering_properties->index_count = 0;
    rendering_properties->vertices.size = 0;
    //rendering_properties->indices.size = 0;
}

void rendering_properties_array_clear(
    RenderingPropertiesArray* rendering_properties)
{
    for (u32 i = 0; i < rendering_properties->size; ++i)
    {
        rendering_properties_clear(&rendering_properties->data[i]);
    }
}

void rendering_properties_check_and_grow_buffers(
    RenderingProperties* rendering_properties)
{
    if (rendering_properties->vertices.size >
        rendering_properties->vertex_buffer_capacity)
    {
        vertex_buffer_orphan(rendering_properties->vertex_buffer_id,
                             rendering_properties->vertices.capacity *
                                 sizeof(Vertex),
                             GL_STREAM_DRAW, NULL);
        rendering_properties->vertex_buffer_capacity =
            rendering_properties->vertices.capacity;
    }

    if (rendering_properties->index_count * 6 >
        rendering_properties->indices.size)
    {
        rendering_properties->indices.size = 0;
        rendering_properties->indices.capacity *= 2;
        rendering_properties->indices.data =
            (u32*)calloc(rendering_properties->indices.capacity, sizeof(u32));
        generate_indicies(&rendering_properties->indices, 0,
                          rendering_properties->indices.capacity * 6);
        index_buffer_orphan(rendering_properties->index_buffer_id,
                            rendering_properties->indices.capacity *
                                sizeof(u32),
                            GL_STATIC_DRAW, rendering_properties->indices.data);
        free(rendering_properties->indices.data);
    }
}

void rendering_properties_draw(const RenderingProperties* rendering_properties,
                               const MVP* mvp)
{
    if (rendering_properties->index_count)
    {
        shader_bind(rendering_properties->shader_properties.shader);
        shader_set_mvp(&rendering_properties->shader_properties, mvp);

        int textures[20] = { 0 };
        for (u32 i = 0; i < rendering_properties->texture_count; ++i)
        {
            textures[i] = (int)i;
        }
        int location = glGetUniformLocation(
            rendering_properties->shader_properties.shader, "textures");
        ftic_assert(location != -1);
        glUniform1iv(location, rendering_properties->texture_count, textures);

        for (u32 i = 0; i < rendering_properties->texture_count; ++i)
        {
            texture_bind(rendering_properties->textures[i], (int)i);
        }

        const AABB* scissor = &rendering_properties->scissor;
        glScissor((int)scissor->min.x, (int)scissor->min.y,
                  (int)scissor->size.width, (int)scissor->size.height);

        vertex_array_bind(rendering_properties->vertex_array_id);
        index_buffer_bind(rendering_properties->index_buffer_id);
        glDrawElements(GL_TRIANGLES, rendering_properties->index_count * 6,
                       GL_UNSIGNED_INT, NULL);
    }
}
