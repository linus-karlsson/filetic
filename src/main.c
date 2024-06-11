#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#define WIN_32_EXTRA_LEAN
#include <Windows.h>
#include <glad/glad.h>
#include <cglm/cglm.h>

#include "define.h"
#include "platform/platform.h"
#include "logging.h"

#define array_create(array, array_capacity)                                    \
    do                                                                         \
    {                                                                          \
        (array)->size = 0;                                                     \
        (array)->capacity = (array_capacity);                                  \
        (array)->data = calloc(array_capacity, sizeof((*(array)->data)));      \
    } while (0)

#define array_push(array, value)                                               \
    do                                                                         \
    {                                                                          \
        if ((array)->size >= (array)->capacity)                                \
        {                                                                      \
            (array)->capacity = (u32)((array)->capacity * 2.0f);               \
            (array)->data = realloc((array)->data, (array)->capacity);         \
        }                                                                      \
        (array)->data[(array)->size++] = (value);                              \
    } while (0)

typedef struct File_Attrib
{
    u8* buffer;
    u64 size;
} File_Attrib;

File_Attrib read_file(const char* file_path)
{
    FILE* file = fopen(file_path, "rb");

    if (!file)
    {
        log_file_error(file_path);
        ftic_assert(false);
    }

    File_Attrib file_attrib = { 0 };
    fseek(file, 0, SEEK_END);
    file_attrib.size = (u64)ftell(file);
    rewind(file);

    file_attrib.buffer = (u8*)calloc(file_attrib.size + 1, sizeof(u8));

    if (fread(file_attrib.buffer, 1, file_attrib.size, file) !=
        file_attrib.size)
    {
        log_last_error();
        ftic_assert(false);
    }
    fclose(file);
    return file_attrib;
}

char* get_shader_error_prefix(u32 type)
{
    const char* prefix = "Failed to compile";
    const char* what_file =
        type == GL_VERTEX_SHADER ? " vertex shader: " : " fragment shader: ";
    size_t prefix_message_length = 0;
    char* prefix_message =
        concatinate(prefix, strlen(prefix), what_file, strlen(what_file), 0,
                    &prefix_message_length);
    return prefix_message;
}

u32 shader_compile(u32 type, const u8* source)
{
    u32 id = glCreateShader(type);
    glShaderSource(id, 1, (const char**)&source, NULL);
    glCompileShader(id);

    int result = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        char* prefix_message = get_shader_error_prefix(type);

        int length = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)calloc(length, sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);

        char* error_message = concatinate(
            prefix_message, strlen(prefix_message), message, length, 0, NULL);

        log_error_message(error_message, strlen(error_message));

        free(prefix_message);
        free(message);
        free(error_message);
        glDeleteShader(id);
        return 0;
    }

    return id;
}

u32 shader_create(const char* vertex_file_path, const char* fragment_file_path)
{
    File_Attrib vertex_file = read_file(vertex_file_path);
    File_Attrib fragment_file = read_file(fragment_file_path);
    u32 vertex_shader = shader_compile(GL_VERTEX_SHADER, vertex_file.buffer);
    u32 fragemnt_shader =
        shader_compile(GL_FRAGMENT_SHADER, fragment_file.buffer);

    u32 program = glCreateProgram();

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragemnt_shader);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragemnt_shader);

    return program;
}

void shader_bind(u32 shader)
{
    glUseProgram(shader);
}

void shader_unbind()
{
    glUseProgram(0);
}

void shader_destroy(u32 shader)
{
    glDeleteProgram(shader);
}

u32 vertex_buffer_create(const void* data, u32 count, u32 size, GLenum usage)
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

u32 index_buffer_create(const void* data, u32 count, u32 size, GLenum usage)
{
    u32 index_buffer = 0;
    glGenBuffers(1, &index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * size, data, usage);
    return index_buffer;
}

void index_buffer_bind(u32 index_buffer)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
}

void index_buffer_unbind()
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

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

typedef enum DebugLogLevel
{
    NONE,
    HIGH,
    MEDIUM,
    LOW,
    NOTIFICATION,
} DebugLogLevel;

global DebugLogLevel g_debug_log_level = HIGH;

void set_gldebug_log_level(DebugLogLevel level)
{
    g_debug_log_level = level;
}

void opengl_log_message(GLenum source, GLenum type, GLuint id, GLenum severity,
                        GLsizei length, const GLchar* message,
                        const void* userParam)
{
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
        {
            if (g_debug_log_level >= HIGH)
            {
                log_error_message(message, strlen(message));
                ftic_assert(false);
            }
            break;
        }
        case GL_DEBUG_SEVERITY_MEDIUM:
        {
            if (g_debug_log_level >= MEDIUM)
            {
                log_message(message, strlen(message));
            }
            break;
        }
        case GL_DEBUG_SEVERITY_LOW:
        {
            if (g_debug_log_level >= LOW)
            {
                log_message(message, strlen(message));
            }
            break;
        }
        case GL_DEBUG_SEVERITY_NOTIFICATION:
        {
            log_message(message, strlen(message));
            break;
        }
        default: break;
    }
}

void enable_gldebugging()
{
    glDebugMessageCallback(opengl_log_message, NULL);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
}

typedef struct Vertex
{
    vec4 color;
    vec3 position;
} Vertex;

typedef struct IndexArray
{
    u32 size;
    u32 capacity;
    u32* data;
} IndexArray;

typedef struct VertexArray
{
    u32 size;
    u32 capacity;
    Vertex* data;
} VertexArray;

int main(int argc, char** argv)
{
    Platform* platform = NULL;
    platform_init("FileTic", 1000, 600, &platform);
    platform_opengl_init(platform);
    if (!gladLoadGL())
    {
        const char* message = "Could not load glad!";
        log_error_message(message, strlen(message));
        ftic_assert(false);
    }

    u32 shader = shader_create("./res/shaders/vertex.glsl",
                               "./res/shaders/fragment.glsl");

    IndexArray index_array = { 0 };
    array_create(&index_array, 100);
    array_push(&index_array, 0);
    array_push(&index_array, 1);
    array_push(&index_array, 2);

    VertexArray vertex_array = { 0 };
    array_create(&vertex_array, 100);

    Vertex vertex = {
        .position = { -0.5f, -0.5f, 0.0f },
        .color = { 1.0f, 1.0f, 1.0f, 1.0f },
    };
    array_push(&vertex_array, vertex);
    vertex.position[0] = 0.5f;
    array_push(&vertex_array, vertex);
    vertex.position[0] = 0.f;
    vertex.position[1] = 0.5f;
    array_push(&vertex_array, vertex);

    VertexBufferLayout vertex_buffer_layout = { 0 };
    vertex_buffer_layout_create(2, sizeof(Vertex), &vertex_buffer_layout);
    vertex_buffer_layout_push_float(&vertex_buffer_layout, 4);
    vertex_buffer_layout_push_float(&vertex_buffer_layout, 3);

    u32 vertex_array_id = vertex_array_create();
    vertex_array_bind(vertex_array_id);

    u32 vertex_buffer_id = vertex_buffer_create(
        vertex_array.data, vertex_array.size, sizeof(Vertex), GL_STATIC_DRAW);
    vertex_array_add_buffer(vertex_array_id, vertex_buffer_id,
                            &vertex_buffer_layout);

    u32 index_buffer_id = index_buffer_create(
        index_array.data, index_array.size, sizeof(u32), GL_STATIC_DRAW);

    enable_gldebugging();
    while (platform_is_running(platform))
    {
        ClientRect client_rect = platform_get_client_rect(platform);
        GLint viewport_width = client_rect.right - client_rect.left;
        GLint viewport_height = client_rect.bottom - client_rect.top;
        glViewport(0, 0, viewport_width, viewport_height);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader_bind(shader);
        vertex_array_bind(vertex_array_id);
        index_buffer_bind(index_buffer_id);
        glDrawElements(GL_TRIANGLES, index_array.size, GL_UNSIGNED_INT, NULL);

        platform_opengl_swap_buffers(platform);
        platform_event_fire(platform);
    }
    platform_opengl_clean(platform);
}
