#pragma once
#include "define.h"
#include "collision.h"
#include "shader.h"
#include "util.h"
#include "buffers.h"

typedef struct Render
{
    u32 vertex_buffer_id;
    u32 vertex_array_id;
    u32 index_buffer_id;
    U32Array textures;
    AABB scissor;
    ShaderProperties shader_properties;
} Render;

typedef struct RenderingProperties
{
    Render render;
    IndexArray indices;
    VertexArray vertices;
    u32 vertex_buffer_capacity;
} RenderingProperties;

Render render_create(const u32 shader_id, const U32Array textures, const VertexBufferLayout* vertex_buffer_layout, const u32 vertex_buffer_id, const u32 index_buffer_id);
void render_destroy(Render* render);
void rendering_properties_clear(RenderingProperties* rendering_properties);
void rendering_properties_check_and_grow_vertex_buffer(RenderingProperties* rendering_properties);
void rendering_properties_check_and_grow_index_buffer( RenderingProperties* rendering_properties, const u32 total_index_count);
void render_begin_draw(const Render* render, const u32 shader, const MVP* mvp);
void render_draw(const u32 index_offset, const u32 index_count, const AABB* scissor);
void render_end_draw(const Render* render);
