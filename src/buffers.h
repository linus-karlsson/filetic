#pragma once
#include "define.h"

typedef struct VertexBufferItem
{
    u32 type;
    u32 count;
    u32 size;
} VertexBufferItem;

typedef struct VertexBufferItemArray
{
    u32 size;
    u32 capacity;
    VertexBufferItem* data;
} VertexBufferItemArray;

typedef struct VertexBufferLayout
{
    u32 size;
    u32 stride;
    VertexBufferItemArray items;
} VertexBufferLayout;

u32 vertex_buffer_create(const void* data, const u32 count, const u32 size, const u32 usage);
void vertex_buffer_bind(const u32 vertex_buffer);
void vertex_buffer_unbind();

u32 index_buffer_create(const void* data, const u32 count, const u32 size, const u32 usage);
void index_buffer_bind(const u32 index_buffer);
void index_buffer_unbind();

void vertex_buffer_layout_create(const u32 capacity, const u32 type_size, VertexBufferLayout* vertex_buffer_layout);
void vertex_buffer_layout_push_float(VertexBufferLayout* vertex_buffer_layout, const u32 count);
u32 vertex_array_create();
void vertex_array_bind(const u32 vertex_array);
void vertex_array_unbind();
void vertex_array_add_buffer(const u32 vertex_array, const u32 vertex_buffer, const VertexBufferLayout* vertex_buffer_layout);
