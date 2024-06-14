#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#define WIN_32_EXTRA_LEAN
#include <Windows.h>
#include <glad/glad.h>
#include <stb/stb_truetype.h>

#include "define.h"
#include "platform/platform.h"
#include "logging.h"
#include "buffers.h"
#include "thread_queue.h"
#include "collision.h"
#include "ftic_math.h"
#include "texture.h"

typedef struct FileAttrib
{
    u8* buffer;
    u64 size;
} FileAttrib;

typedef struct Vertex
{
    V4 color;
    V3 position;
    V2 texture_coordinates;
    f32 texture_index;
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

typedef struct SafeFileArray
{
    u32 size;
    u32 capacity;
    File* data;
    FTicMutex mutex;
} SafeFileArray;

typedef struct FindingCallbackAttribute
{
    ThreadTaskQueue* thread_queue;
    char* start_directory;
    u32 start_directory_length;
    const char* string_to_match;
    u32 string_to_match_length;
    SafeFileArray* array;
} FindingCallbackAttribute;

global b8 running = true;

FileAttrib read_file(const char* file_path)
{
    FILE* file = fopen(file_path, "rb");

    if (!file)
    {
        log_file_error(file_path);
        ftic_assert(false);
    }

    FileAttrib file_attrib = { 0 };
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
        concatinate(prefix, strlen(prefix), what_file, strlen(what_file), 0, 0,
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

        char* error_message =
            concatinate(prefix_message, strlen(prefix_message), message, length,
                        0, 0, NULL);

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
    FileAttrib vertex_file = read_file(vertex_file_path);
    FileAttrib fragment_file = read_file(fragment_file_path);
    u32 vertex_shader = shader_compile(GL_VERTEX_SHADER, vertex_file.buffer);
    u32 fragemnt_shader =
        shader_compile(GL_FRAGMENT_SHADER, fragment_file.buffer);
    free(vertex_file.buffer);
    free(fragment_file.buffer);

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

void parse_all_subdirectories(const char* start_directory, const u32 length)
{
    Directory directory = platform_get_directory(start_directory, length);

    for (u32 i = 0; i < directory.files.size; ++i)
    {
        const char* name = directory.files.data[i].name;
        log_message(name, strlen(name));
        free(directory.files.data[i].path);
    }
    for (u32 i = 0; i < directory.sub_directories.size; ++i)
    {
        char* path = directory.sub_directories.data[i].path;
        size_t directory_name_length = strlen(path);
        path[directory_name_length++] = '/';
        path[directory_name_length++] = '*';
        parse_all_subdirectories(path, (u32)directory_name_length);
        free(path);
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

void finding_callback(void* data)
{
    FindingCallbackAttribute* arguments = (FindingCallbackAttribute*)data;

    Directory directory = platform_get_directory(
        arguments->start_directory, arguments->start_directory_length);

    for (u32 i = 0; i < directory.sub_directories.size && running; ++i)
    {
        char* path = directory.sub_directories.data[i].path;
        size_t directory_name_length = strlen(path);
        path[directory_name_length++] = '\\';
        path[directory_name_length++] = '*';

        FindingCallbackAttribute* next_arguments =
            (FindingCallbackAttribute*)calloc(1,
                                              sizeof(FindingCallbackAttribute));
        next_arguments->array = arguments->array;
        next_arguments->thread_queue = arguments->thread_queue;
        next_arguments->start_directory = path;
        next_arguments->start_directory_length = (u32)directory_name_length;
        next_arguments->string_to_match = arguments->string_to_match;
        next_arguments->string_to_match_length =
            arguments->string_to_match_length;
        ThreadTask task = thread_task(finding_callback, next_arguments);
        thread_tasks_push(next_arguments->thread_queue, &task, 1, NULL);
    }

    for (u32 i = 0; i < directory.files.size && running; ++i)
    {
        const char* name = directory.files.data[i].name;
        char* path = directory.files.data[i].path;
        if (string_contains(name, (u32)strlen(name), arguments->string_to_match,
                            arguments->string_to_match_length))
        {
            safe_array_push(arguments->array, directory.files.data[i]);
            log_message(path, strlen(path));
        }
        else
        {
            free(path);
        }
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
        const char* name = directory.files.data[i].name;
        if (string_contains(name, (u32)strlen(name), string_to_match,
                            string_to_match_length))
        {
            log_message(name, strlen(name));
        }
        free(directory.files.data[i].path);
    }
    for (u32 i = 0; i < directory.sub_directories.size; ++i)
    {
        char* path = directory.sub_directories.data[i].path;
        size_t directory_name_length = strlen(path);
        path[directory_name_length++] = '/';
        path[directory_name_length++] = '*';
        find_matching_string(path, (u32)directory_name_length, string_to_match,
                             string_to_match_length);
        free(path);
    }
}

typedef struct TextureCoordinates
{
    V2 coordinates[4];
} TextureCoordinates;

internal AABB set_up_verticies(VertexArray* vertex_array, V3 position, V2 size,
                               V4 color, f32 texture_index,
                               TextureCoordinates texture_coordinates)
{
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
    out.min = v2_v3(position);
    out.size = size;
    return out;
}

AABB quad_co(VertexArray* vertex_array, V3 position, V2 size, V4 color,
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

AABB quad(VertexArray* vertex_array, V3 position, V2 size, V4 color,
          f32 texture_index)
{
    TextureCoordinates texture_coordinates = { v2d(), v2f(0.0f, 1.0f),
                                               v2f(1.0f, 1.0f),
                                               v2f(1.0f, 0.0f) };
    return set_up_verticies(vertex_array, position, size, color, texture_index,
                            texture_coordinates);
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

int shader_get_uniform_location(u32 shader, const char* name)
{
    int location = glGetUniformLocation(shader, name);
    ftic_assert(location != -1);
    return location;
}

void shader_set_uniform_matrix4fv(u32 shader, int uniform_location, M4 value)
{
    glUniformMatrix4fv(uniform_location, 1, GL_FALSE, &value.data[0][0]);
}

typedef struct ShaderProperties
{
    u32 shader;
    int projection_location;
    int view_location;
    int model_location;
} ShaderProperties;

typedef struct MVP
{
    M4 projection;
    M4 view;
    M4 model;
} MVP;

ShaderProperties shader_create_properties(u32 shader, const char* projection,
                                          const char* view, const char* model)
{
    ShaderProperties shader_properties = {
        .shader = shader,
        .projection_location = shader_get_uniform_location(shader, projection),
        .view_location = shader_get_uniform_location(shader, view),
        .model_location = shader_get_uniform_location(shader, model),
    };
    return shader_properties;
}

void shader_set_MVP(const ShaderProperties* shader_properties, const MVP* mvp)
{
    shader_set_uniform_matrix4fv(shader_properties->shader,
                                 shader_properties->projection_location,
                                 mvp->projection);
    shader_set_uniform_matrix4fv(shader_properties->shader,
                                 shader_properties->view_location, mvp->view);
    shader_set_uniform_matrix4fv(shader_properties->shader,
                                 shader_properties->model_location, mvp->model);
}

typedef struct CharacterTTF
{
    V2 dimensions;
    V2 offset;
    V4 text_coords;
    f32 x_advance;
} CharacterTTF;

typedef struct FontTTF
{
    f32 tex_index;
    f32 line_height;
    f32 pixel_height;
    u32 char_count;
    CharacterTTF* chars;
} FontTTF;

void init_ttf_atlas(i32 width_atlas, i32 height_atlas, f32 pixel_height,
                    u32 glyph_count, u32 glyph_offset,
                    const char* font_file_path, u8* bitmap, FontTTF* font_out)
{
    FileAttrib file = read_file(font_file_path);
    stbtt_bakedchar* data =
        (stbtt_bakedchar*)calloc(glyph_count, sizeof(stbtt_bakedchar));
    stbtt_BakeFontBitmap(file.buffer, 0, pixel_height, bitmap, 512, 512,
                         glyph_offset, glyph_count, data);

    FontTTF font = { 0 };
    font.line_height = pixel_height;
    font.pixel_height = pixel_height;
    font.char_count = glyph_count;
    font.chars = (CharacterTTF*)calloc(glyph_count, sizeof(CharacterTTF));
    for (u32 i = 0; i < glyph_count; i++)
    {
        CharacterTTF* c_ttf = &font.chars[i];
        stbtt_bakedchar bc = data[i];
        c_ttf->dimensions = v2f((f32)(bc.x1 - bc.x0), (f32)(bc.y1 - bc.y0));
        c_ttf->offset = v2f(bc.xoff, bc.yoff);
        c_ttf->x_advance = bc.xadvance;
        c_ttf->text_coords =
            v4f((f32)bc.x0 / width_atlas, (f32)bc.y0 / height_atlas,
                (f32)bc.x1 / width_atlas, (f32)bc.y1 / height_atlas);
    }
    *font_out = font;
    free(data);
    free(file.buffer);
}

u32 text_gen(const CharacterTTF* c_ttf, const char* text, float texture_index,
             V3 pos, f32 scale, f32 line_height, u32* new_lines_count,
             f32* x_advance, VertexArray* array)
{
    u32 count = 0;
    f32 start_x = pos.x;
    u32 new_lines = 0;
    for (; *text; text++)
    {
        char current_char = *text;
        if (current_char == '\n')
        {
            pos.y += line_height * scale;
            pos.x = start_x;
            new_lines++;
            continue;
        }
        if (closed_interval(0, (current_char - 32), 96))
        {
            const CharacterTTF* c = c_ttf + (current_char - 32);

            V2 size = v2_s_multi(c->dimensions, scale);
            V3 curr_pos = v3_add(pos, v3_v2(v2_s_multi(c->offset, scale)));
            quad_co(array, curr_pos, size, v4i(1.0f), c->text_coords,
                    texture_index);

            pos.x += c->x_advance * scale;
            count++;
        }
    }
    if (new_lines_count)
    {
        *new_lines_count = new_lines;
    }
    if (x_advance)
    {
        *x_advance = pos.x - start_x;
    }
    return count;
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

    FontTTF font = { 0 };
    const i32 width_atlas = 512;
    const i32 height_atlas = 512;
    const f32 pixel_height = 48;
    u8* font_bitmap = (u8*)calloc(width_atlas * height_atlas, sizeof(u8));
    init_ttf_atlas(width_atlas, height_atlas, pixel_height, 96, 32,
                   "res/fonts/arial.ttf", font_bitmap, &font);

    TextureProperties texture_properties = {
        .width = width_atlas,
        .height = height_atlas,
        .bytes = font_bitmap,
    };
    u32 font_texture = texture_create(&texture_properties, GL_RGBA8, GL_RED);

    u32 font_shader = shader_create("./res/shaders/vertex.glsl",
                                    "./res/shaders/font_fragment.glsl");
    ftic_assert(font_shader);

    u32 shader2 = shader_create("./res/shaders/vertex.glsl",
                                "./res/shaders/fragment.glsl");

    IndexArray index_array = { 0 };
    array_create(&index_array, 100);
    generate_indicies(&index_array, 0, 1000);

    VertexArray vertex_array = { 0 };
    array_create(&vertex_array, 100);
    u32 count =
        text_gen(font.chars, "Hello testing", 0.0f, v3f(100.0f, 100.0f, 0.0f),
                 1.0f, font.pixel_height, NULL, NULL, &vertex_array);

    VertexBufferLayout vertex_buffer_layout = { 0 };
    vertex_buffer_layout_create(4, sizeof(Vertex), &vertex_buffer_layout);
    vertex_buffer_layout_push_float(&vertex_buffer_layout, 4,
                                    offsetof(Vertex, color));
    vertex_buffer_layout_push_float(&vertex_buffer_layout, 3,
                                    offsetof(Vertex, position));
    vertex_buffer_layout_push_float(&vertex_buffer_layout, 2,
                                    offsetof(Vertex, texture_coordinates));
    vertex_buffer_layout_push_float(&vertex_buffer_layout, 1,
                                    offsetof(Vertex, texture_index));

    u32 vertex_array_id = vertex_array_create();
    vertex_array_bind(vertex_array_id);

    u32 vertex_buffer_id = vertex_buffer_create(
        vertex_array.data, vertex_array.size, sizeof(Vertex), GL_STATIC_DRAW);
    vertex_array_add_buffer(vertex_array_id, vertex_buffer_id,
                            &vertex_buffer_layout);

    u32 index_buffer_id = index_buffer_create(
        index_array.data, index_array.size, sizeof(u32), GL_STATIC_DRAW);

    ThreadQueue thread_queue = { 0 };
    thread_init(100000, 8, &thread_queue);

    SafeFileArray file_array = { 0 };
    safe_array_create(&file_array, 10);

    const char* dir = "C:\\Users\\linus\\*";
    char* dir2 = (char*)calloc(strlen(dir) + 1, sizeof(char));
    memcpy(dir2, dir, strlen(dir));
    const char* string_to_match = "init.vim";

    FindingCallbackAttribute* arguments =
        (FindingCallbackAttribute*)calloc(1, sizeof(FindingCallbackAttribute));
    arguments->thread_queue = &thread_queue.task_queue;
    arguments->array = &file_array;
    arguments->start_directory = dir2;
    arguments->start_directory_length = (u32)strlen(dir2);
    arguments->string_to_match = string_to_match;
    arguments->string_to_match_length = (u32)strlen(string_to_match);
    finding_callback(arguments);

    ShaderProperties shader_properties =
        shader_create_properties(font_shader, "proj", "view", "model");

    MVP mvp = { 0 };
    mvp.view = m4d();
    mvp.model = m4d();
    enable_gldebugging();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    while (platform_is_running(platform))
    {
        ClientRect client_rect = platform_get_client_rect(platform);
        GLint viewport_width = client_rect.right - client_rect.left;
        GLint viewport_height = client_rect.bottom - client_rect.top;
        mvp.projection = ortho(0.0f, (float)viewport_width,
                               (float)viewport_height, 0.0f, -1.0f, 1.0f);
        mvp.view = m4d();
        glViewport(0, 0, viewport_width, viewport_height);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader_bind(font_shader);
        shader_set_MVP(&shader_properties, &mvp);
        texture_bind(font_texture, 0);
        vertex_array_bind(vertex_array_id);
        index_buffer_bind(index_buffer_id);
        glDrawElements(GL_TRIANGLES, count * 6, GL_UNSIGNED_INT, NULL);

        platform_opengl_swap_buffers(platform);
        platform_event_fire(platform);
    }
    running = false;
    platform_opengl_clean(platform);
    threads_destroy(&thread_queue);
    platform_shut_down(platform);
}
