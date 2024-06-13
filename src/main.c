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
#include "buffers.h"
#include "thread_queue.h"

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

void parse_all_subdirectories(const char* start_directory, const u32 length)
{
    Directory directory = platform_get_directory(start_directory, length);

    for (u32 i = 0; i < directory.files.size; ++i)
    {
        char* name = directory.files.data[i].name;
        log_message(name, strlen(name));
        free(name);
    }
    for (u32 i = 0; i < directory.sub_directories.size; ++i)
    {
        char* name = directory.sub_directories.data[i];
        size_t directory_name_length = strlen(name);
        name[directory_name_length++] = '\\';
        name[directory_name_length++] = '*';
        char* full_directory =
            concatinate(start_directory, length - 1, name,
                        directory_name_length, 0, &directory_name_length);
        parse_all_subdirectories(full_directory, (u32)directory_name_length);
        free(full_directory);
        free(name);
    }
}

internal b8 string_contains(const char* string, const u32 string_length,
                            const char* value, const u32 value_length)
{
    if (value_length > string_length) return false;

    for (u32 i = 0, j = 0; i < string_length; ++i)
    {
        j = string[i] == value[j] ? j + 1 : 0;
        if (j == value_length) return true;
    }
    return false;
}

typedef struct FindingCallbackAttribute
{
    ThreadTaskQueue* thread_queue;
    char* start_directory;
    u32 start_directory_length;
    const char* string_to_match;
    u32 string_to_match_length;
} FindingCallbackAttribute;

void finding_callback(void* data)
{
    FindingCallbackAttribute* arguments = (FindingCallbackAttribute*)data;

    Directory directory = platform_get_directory(
        arguments->start_directory, arguments->start_directory_length);

    for (u32 i = 0; i < directory.sub_directories.size; ++i)
    {
        char* name = directory.sub_directories.data[i];
        size_t directory_name_length = strlen(name);
        name[directory_name_length++] = '\\';
        name[directory_name_length++] = '*';
        char* full_directory = concatinate(
            arguments->start_directory, arguments->start_directory_length - 1,
            name, directory_name_length, 0, &directory_name_length);

        FindingCallbackAttribute* next_arguments =
            (FindingCallbackAttribute*)calloc(1,
                                              sizeof(FindingCallbackAttribute));
        next_arguments->thread_queue = arguments->thread_queue;
        next_arguments->start_directory = full_directory;
        next_arguments->start_directory_length = (u32)directory_name_length;
        next_arguments->string_to_match = arguments->string_to_match;
        next_arguments->string_to_match_length =
            arguments->string_to_match_length;
        ThreadTask task = thread_task(finding_callback, next_arguments);
        thread_tasks_push(next_arguments->thread_queue, &task, 1, NULL);
        free(name);
    }

    for (u32 i = 0; i < directory.files.size; ++i)
    {
        char* name = directory.files.data[i].name;
        if (string_contains(name, (u32)strlen(name), arguments->string_to_match,
                            arguments->string_to_match_length))
        {
            log_message(name, strlen(name));
        }
        free(name);
    }
    free(arguments->start_directory);
    free(data);
}

void find_matching_string(const char* start_directory, const u32 length,
                          const char* string_to_match,
                          const u32 string_to_match_length)
{
    Directory directory = platform_get_directory(start_directory, length);

    for (u32 i = 0; i < directory.files.size; ++i)
    {
        char* name = directory.files.data[i].name;
        if (string_contains(name, (u32)strlen(name), string_to_match,
                            string_to_match_length))
        {
            log_message(name, strlen(name));
        }
        free(name);
    }
    for (u32 i = 0; i < directory.sub_directories.size; ++i)
    {
        char* name = directory.sub_directories.data[i];
        size_t directory_name_length = strlen(name);
        name[directory_name_length++] = '\\';
        name[directory_name_length++] = '*';
        char* full_directory =
            concatinate(start_directory, length - 1, name,
                        directory_name_length, 0, &directory_name_length);
        find_matching_string(full_directory, (u32)directory_name_length,
                             string_to_match, string_to_match_length);
        free(full_directory);
        free(name);
    }
}

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

    ThreadQueue thread_queue = { 0 };
    thread_init(100000, 6, &thread_queue);

    const char* dir = "C:\\Users\\linus\\dev\\*";
    char* dir2 = (char*)calloc(strlen(dir) + 1, sizeof(char));
    memcpy(dir2, dir, strlen(dir));
    const char* string_to_match = "logging";

    FindingCallbackAttribute* arguments =
        (FindingCallbackAttribute*)calloc(1, sizeof(FindingCallbackAttribute));
    arguments->thread_queue = &thread_queue.task_queue;
    arguments->start_directory = dir2;
    arguments->start_directory_length = (u32)strlen(dir2);
    arguments->string_to_match = string_to_match;
    arguments->string_to_match_length = (u32)strlen(string_to_match);
    finding_callback(arguments);

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
    threads_destroy(&thread_queue);
    platform_shut_down(platform);
}
