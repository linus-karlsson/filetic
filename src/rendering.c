#include "rendering.h"
#include "opengl_util.h"
#include "texture.h"
#include <glad/glad.h>
#include <math.h>

Render render_create(const u32 shader_id, const U32Array textures,
                     const VertexBufferLayout* vertex_buffer_layout,
                     const u32 vertex_buffer_id, const u32 index_buffer_id)
{
    Render render = { 0 };
    render.vertex_buffer_id = vertex_buffer_id;
    render.vertex_array_id = vertex_array_create();
    vertex_array_bind(render.vertex_array_id);
    vertex_array_add_buffer(render.vertex_array_id, render.vertex_buffer_id,
                            vertex_buffer_layout);
    render.index_buffer_id = index_buffer_id;
    render.textures = textures;
    render.shader_properties =
        shader_create_properties(shader_id, "proj", "view", "model");

    return render;
}

void render_destroy(Render* render)
{
    buffer_delete(render->vertex_buffer_id);
    buffer_delete(render->index_buffer_id);
    for (u32 i = 0; i < render->textures.size; ++i)
    {
        texture_delete(render->textures.data[i]);
    }
    free(render->textures.data);
}

void rendering_properties_clear(RenderingProperties* rendering_properties)
{
    rendering_properties->vertices.size = 0;
}

void rendering_properties_check_and_grow_vertex_buffer(
    RenderingProperties* rendering_properties)
{
    if (rendering_properties->vertices.size >
        rendering_properties->vertex_buffer_capacity)
    {
        vertex_buffer_orphan(rendering_properties->render.vertex_buffer_id,
                             rendering_properties->vertices.capacity *
                                 sizeof(Vertex),
                             GL_STREAM_DRAW, NULL);
        rendering_properties->vertex_buffer_capacity =
            rendering_properties->vertices.capacity;
    }
}

void rendering_properties_check_and_grow_index_buffer(
    RenderingProperties* rendering_properties, const u32 total_index_count)
{
    if (total_index_count > rendering_properties->indices.size)
    {
        rendering_properties->indices.size = 0;
        rendering_properties->indices.capacity *= 2;
        rendering_properties->indices.data =
            (u32*)calloc(rendering_properties->indices.capacity, sizeof(u32));
        generate_indicies(&rendering_properties->indices, 0,
                          rendering_properties->indices.capacity * 6);
        index_buffer_orphan(rendering_properties->render.index_buffer_id,
                            rendering_properties->indices.capacity *
                                sizeof(u32),
                            GL_STATIC_DRAW, rendering_properties->indices.data);
        free(rendering_properties->indices.data);
    }
}

void render_begin_draw(const Render* render, const MVP* mvp)
{
    shader_bind(render->shader_properties.shader);
    shader_set_mvp(&render->shader_properties, mvp);

    int textures[64] = { 0 };
    for (u32 i = 0; i < render->textures.size; ++i)
    {
        textures[i] = (int)i;
    }
    int location =
        glGetUniformLocation(render->shader_properties.shader, "textures");
    ftic_assert(location != -1);
    glUniform1iv(location, render->textures.size, textures);

    for (u32 i = 0; i < render->textures.size; ++i)
    {
        texture_bind(render->textures.data[i], (int)i);
    }
    vertex_array_bind(render->vertex_array_id);
    index_buffer_bind(render->index_buffer_id);
}

void render_draw(const u32 index_offset, const u32 index_count,
                 const AABB* scissor)
{
    if (index_count)
    {
        glEnable(GL_SCISSOR_TEST);
        glScissor((int)roundf(scissor->min.x), (int)roundf(scissor->min.y),
                  (int)roundf(scissor->size.width), (int)roundf(scissor->size.height));

        GLintptr offset = index_offset * sizeof(u32);
        glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT,
                       (void*)offset);

        glDisable(GL_SCISSOR_TEST);
    }
}

void render_end_draw(const Render* render)
{
    for (u32 i = 0; i < render->textures.size; ++i)
    {
        texture_unbind((int)i);
    }
    index_buffer_unbind();
    vertex_array_unbind();
    shader_unbind();
}
