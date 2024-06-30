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
    u32 index_count;
    u32 texture_count;
    u32* textures;

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

RenderingProperties rendering_properties_initialize( u32 shader_id, u32* textures, u32 texture_count, const VertexBufferLayout* vertex_buffer_layout, u32 vertex_buffer_size, u32 index_buffer_size);
void rendering_properties_clear(RenderingProperties* rendering_properties);
void rendering_properties_array_clear(RenderingPropertiesArray* rendering_properties);
void rendering_properties_check_and_grow_buffers(RenderingProperties* rendering_properties);
void rendering_properties_draw(const RenderingProperties* rendering_properties, const MVP* mvp);
