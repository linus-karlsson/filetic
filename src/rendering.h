#pragma once
#include "define.h"
#include "collision.h"
#include "shader.h"
#include "util.h"
#include "buffers.h"

typedef struct RenderingProperties
{
    u32 vertex_buffer_id;
    u32 vertex_buffer_capacity;
    u32 vertex_array_id;
    u32 index_buffer_id;
    U32Array textures;

    AABB scissor;

    ShaderProperties shader_properties;
    VertexArray vertices;
    IndexArray indices;
} RenderingProperties;

typedef struct RenderingPropertiesArray
{
    u32 size;
    u32 capacity;
    RenderingProperties* data;
} RenderingPropertiesArray;

RenderingProperties rendering_properties_initialize( u32 shader_id, U32Array textures, const VertexBufferLayout* vertex_buffer_layout, u32 vertex_buffer_size, u32 index_buffer_size);
void rendering_properties_clear(RenderingProperties* rendering_properties);
void rendering_properties_array_clear(RenderingPropertiesArray* rendering_properties);
void rendering_properties_check_and_grow_buffers(RenderingProperties* rendering_properties, u32 total_index_count);
void rendering_properties_begin_draw(const RenderingProperties* rendering_properties, const MVP* mvp);
void rendering_properties_draw(const u32 index_offset, const u32 index_count, const AABB* scissor);
void rendering_properties_end_draw(const RenderingProperties* rendering_properties);
