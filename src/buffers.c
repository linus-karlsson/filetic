#include "buffers.h"
#include <stdlib.h>
#include <glad/glad.h>

u32 vertex_buffer_create(const void* data, const u32 count, const u32 size,
                         const u32 usage)
{
    u32 vertex_buffer = 0;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, size * count, data, usage);
    return vertex_buffer;
}

void vertex_buffer_bind(uint32_t vertex_buffer)
{
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
}

void vertex_buffer_unbind()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

u32 index_buffer_create(const void* data, const u32 count, const u32 size,
                        const u32 usage)
{
    u32 index_buffer = 0;
    glGenBuffers(1, &index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * size, data, usage);
    return index_buffer;
}

void index_buffer_bind(const u32 index_buffer)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
}

void index_buffer_unbind()
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void vertex_buffer_layout_create(const u32 capacity, const u32 type_size,
                                 VertexBufferLayout* vertex_buffer_layout)
{
    vertex_buffer_layout->size = 0;
    vertex_buffer_layout->stride = type_size;
    array_create(&vertex_buffer_layout->items, capacity);
}

void vertex_buffer_layout_push_float(VertexBufferLayout* vertex_buffer_layout,
                                     const u32 count)
{
    const VertexBufferItem item = {
        .type = GL_FLOAT,
        .count = count,
        .size = sizeof(f32) * count,
    };
    array_push(&vertex_buffer_layout->items, item);
}

u32 vertex_array_create()
{
    u32 vertex_array = 0;
    glGenVertexArrays(1, &vertex_array);
    return vertex_array;
}

void vertex_array_bind(u32 vertex_array)
{
    glBindVertexArray(vertex_array);
}

void vertex_array_unbind()
{
    glBindVertexArray(0);
}

void vertex_array_add_buffer(const u32 vertex_array, const u32 vertex_buffer,
                             const VertexBufferLayout* vertex_buffer_layout)
{
    vertex_array_bind(vertex_array);
    vertex_buffer_bind(vertex_buffer);

    u64 offset = 0;
    for (u32 i = 0; i < vertex_buffer_layout->items.size; ++i)
    {
        const VertexBufferItem* item = vertex_buffer_layout->items.data + i;
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, item->count, item->type, GL_FALSE,
                              vertex_buffer_layout->stride, (void*)offset);
        offset += item->size;
    }
}
