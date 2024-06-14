#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#define WIN_32_EXTRA_LEAN
#include <Windows.h>
#include <glad/glad.h>
#include <stb/stb_truetype.h>
#include <math.h>

#include "define.h"
#include "platform/platform.h"
#include "logging.h"
#include "buffers.h"
#include "thread_queue.h"
#include "collision.h"
#include "ftic_math.h"
#include "texture.h"
#include "util.h"
#include "font.h"
#include "opengl_util.h"
#include "shader.h"
#include "event.h"

global b8 running = true;

typedef struct SafeFileArray
{
    u32 size;
    u32 capacity;
    DirectoryItem* data;
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
void buffer_set_sub_data(u32 vertex_buffer, u32 target, intptr_t offset,
                         signed long long int size, const void* data)
{
    vertex_buffer_bind(vertex_buffer);
    glBufferSubData(target, offset, size, data);
}

typedef struct RenderingProperties
{
    u32 vertex_buffer_id;
    u32 vertex_array_id;
    u32 index_buffer_id;
    u32 index_count;
    u32 texture_count;
    u32* textures;

    ShaderProperties shader_properties;
    VertexArray vertices;

} RenderingProperties;

typedef struct RenderingPropertiesArray
{
    u32 size;
    u32 capacity;
    RenderingProperties* data;
} RenderingPropertiesArray;

RenderingProperties rendering_properties_init(
    u32 shader_id, u32* textures, u32 texture_count, u32 index_buffer_id,
    const VertexBufferLayout* vertex_buffer_layout, u32 vertex_buffer_size)
{
    RenderingProperties rendering_properties = { 0 };
    array_create(&rendering_properties.vertices, vertex_buffer_size);
    rendering_properties.vertex_array_id = vertex_array_create();
    vertex_array_bind(rendering_properties.vertex_array_id);

    rendering_properties.vertex_buffer_id =
        vertex_buffer_create(NULL, rendering_properties.vertices.capacity,
                             sizeof(Vertex), GL_STREAM_DRAW);
    vertex_array_add_buffer(rendering_properties.vertex_array_id,
                            rendering_properties.vertex_buffer_id,
                            vertex_buffer_layout);

    rendering_properties.index_buffer_id = index_buffer_id;
    rendering_properties.texture_count = texture_count;
    rendering_properties.textures = textures;
    rendering_properties.shader_properties =
        shader_create_properties(shader_id, "proj", "view", "model");

    return rendering_properties;
}

void rendering_properties_draw(const RenderingProperties* rendering_properties,
                               const MVP* mvp)
{
    shader_bind(rendering_properties->shader_properties.shader);
    shader_set_mvp(&rendering_properties->shader_properties, mvp);

    int textures[10] = { 0 };
    for (u32 i = 0; i < rendering_properties->texture_count; ++i)
    {
        textures[i] = (int)i;
    }
    int location = glGetUniformLocation(
        rendering_properties->shader_properties.shader, "textures");
    glUniform1iv(location, rendering_properties->texture_count, textures);

    for (u32 i = 0; i < rendering_properties->texture_count; ++i)
    {
        texture_bind(rendering_properties->textures[i], (int)i);
    }
    vertex_array_bind(rendering_properties->vertex_array_id);
    index_buffer_bind(rendering_properties->index_buffer_id);
    glDrawElements(GL_TRIANGLES, rendering_properties->index_count * 6,
                   GL_UNSIGNED_INT, NULL);
}

void rendering_properties_clear(RenderingProperties* rendering_properties)
{
    rendering_properties->index_count = 0;
    rendering_properties->vertices.size = 0;
}

b8 directory_item(b8 hit, i32 index, V3 starting_position, const f32 scale,
                  const f32 padding_top, const f32 padding_bottom,
                  const f32 quad_height, const DirectoryItem* item,
                  const FontTTF* font, const Event* mouse_move, i32* hit_index,
                  RenderingProperties* render)
{
    const V4 color = index == (*hit_index) ? v4ic(0.3f) : v4ic(0.1f);
    AABB aabb = quad(&render->vertices, starting_position,
                     v2f(200.0f, quad_height), color, 0.0f);
    render->index_count += 1;

    V2 mouse_position = v2f(mouse_move->mouse_move_event.position_x,
                            mouse_move->mouse_move_event.position_y);
    if (!hit && collision_point_in_aabb(mouse_position, &aabb))
    {
        *hit_index = index;
        hit = true;
    }

    const V3 text_position =
        v3_add(starting_position, v3f(padding_top, padding_top, 0.0f));
    render->index_count +=
        text_generation(font->chars, item->name, 1.0f, text_position, scale,
                        font->pixel_height, NULL, NULL, &render->vertices);
    return hit;
}

b8 directory_item_list(V3 starting_position, const f32 scale,
                       const f32 padding_top, const f32 padding_bottom,
                       const f32 quad_height, const DirectoryItemArray* items,
                       const FontTTF* font, const Event* mouse_move,
                       i32* hit_index, RenderingProperties* render)
{
    b8 hit = false;
    for (i32 i = 0; i < (i32)items->size; ++i)
    {
        const V4 color = i == (*hit_index) ? v4ic(0.3f) : v4ic(0.1f);
        AABB aabb = quad(&render->vertices, starting_position,
                         v2f(200.0f, quad_height), color, 0.0f);
        render->index_count += 1;

        V2 mouse_position = v2f(mouse_move->mouse_move_event.position_x,
                                mouse_move->mouse_move_event.position_y);
        if (!hit && collision_point_in_aabb(mouse_position, &aabb))
        {
            *hit_index = i;
            hit = true;
        }

        const V3 text_position =
            v3_add(starting_position, v3f(padding_top, padding_top, 0.0f));
        render->index_count += text_generation(
            font->chars, items->data[i].name, 1.0f, text_position, scale,
            font->pixel_height, NULL, NULL, &render->vertices);
        starting_position.y += quad_height;
    }
    return hit;
}

typedef struct DirectoryPage
{
    f32 text_y;
    f32 offset;
    f32 scroll_offset;
    Directory directory;
} DirectoryPage;

typedef struct DirectoryArray
{
    u32 size;
    u32 capacity;
    DirectoryPage* data;
} DirectoryArray;

f32 clampf32_low(f32 value, f32 low)
{
    return value < low ? low : value;
}

f32 clampf32_high(f32 value, f32 high)
{
    return value > high ? high : value;
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
    event_init(platform);

    FontTTF font = { 0 };
    const i32 width_atlas = 512;
    const i32 height_atlas = 512;
    const f32 pixel_height = 16;
    u8* font_bitmap = (u8*)calloc(width_atlas * height_atlas, sizeof(u8));
    init_ttf_atlas(width_atlas, height_atlas, pixel_height, 96, 32,
                   "res/fonts/arial.ttf", font_bitmap, &font);

    TextureProperties texture_properties = {
        .width = width_atlas,
        .height = height_atlas,
        .bytes = font_bitmap,
    };
    u32 font_texture = texture_create(&texture_properties, GL_RGBA8, GL_RED);
    free(texture_properties.bytes);
    texture_properties.width = 1;
    texture_properties.height = 1;
    u8 pixel = UINT8_MAX;
    texture_properties.bytes = &pixel;
    u32 default_texture = texture_create(&texture_properties, GL_RGBA8, GL_RED);

    u32 font_shader = shader_create("./res/shaders/vertex.glsl",
                                    "./res/shaders/font_fragment.glsl");
    u32 shader2 = shader_create("./res/shaders/vertex.glsl",
                                "./res/shaders/fragment.glsl");
    ftic_assert(font_shader);

    IndexArray index_array = { 0 };
    array_create(&index_array, 100);
    generate_indicies(&index_array, 0, 10000);
    u32 index_buffer_id = index_buffer_create(
        index_array.data, index_array.size, sizeof(u32), GL_STATIC_DRAW);

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

    RenderingPropertiesArray rendering_properties = { 0 };
    array_create(&rendering_properties, 2);
    array_push(&rendering_properties,
               rendering_properties_init(font_shader, &default_texture, 1,
                                         index_buffer_id, &vertex_buffer_layout,
                                         10000 * 4));
    u32 textures[2] = { default_texture, font_texture };
    array_push(&rendering_properties,
               rendering_properties_init(font_shader, textures, 2,
                                         index_buffer_id, &vertex_buffer_layout,
                                         10000 * 4));

    RenderingProperties* default_render = rendering_properties.data;
    RenderingProperties* font_render = rendering_properties.data + 1;

    ThreadQueue thread_queue = { 0 };
    thread_init(100000, 8, &thread_queue);

    SafeFileArray file_array = { 0 };
    safe_array_create(&file_array, 10);

    /*
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
    */

    ClientRect client_rect = platform_get_client_rect(platform);
    GLint viewport_width = client_rect.right - client_rect.left;
    GLint viewport_height = client_rect.bottom - client_rect.top;
    V2 dimensions = v2f((f32)viewport_width, (f32)viewport_height);

    V3 starting_position = v3f(10.0f, 10.0f, 0.0f);
    AABB rect = quad_with_border(
        &default_render->vertices, &default_render->index_count,
        v4f(0.3f, 0.3f, 0.3f, 1.0f), starting_position,
        v2f(100.0f, dimensions.height - 50.0f), 2.0f, 0.0f);
    buffer_set_sub_data(default_render->vertex_buffer_id, GL_ARRAY_BUFFER, 0,
                        sizeof(Vertex) * default_render->vertices.size,
                        default_render->vertices.data);
    default_render->vertices.size = 0;

    DirectoryArray directory_history = { 0 };
    array_create(&directory_history, 10);
    const char* dir = "C:\\*";
    DirectoryPage page = { 0 };
    page.directory = platform_get_directory(dir, (u32)strlen(dir));
    array_push(&directory_history, page);
    DirectoryPage* current_directory = array_back(&directory_history);

    Event* mouse_move = event_subscribe(MOUSE_MOVE);
    Event* mouse_button = event_subscribe(MOUSE_BUTTON);
    Event* mouse_wheel = event_subscribe(MOUSE_WHEEL);

    i32 hit_index = -1;
    f32 scroll_speed = 0.2f;
    f64 last_time = platform_get_time();
    f32 delta_time = 0.0f;
    MVP mvp = { 0 };
    mvp.view = m4d();
    mvp.model = m4d();
    enable_gldebugging();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    while (platform_is_running(platform))
    {
        client_rect = platform_get_client_rect(platform);
        viewport_width = client_rect.right - client_rect.left;
        viewport_height = client_rect.bottom - client_rect.top;
        dimensions = v2f((f32)viewport_width, (f32)viewport_height);
        mvp.projection = ortho(0.0f, (float)viewport_width,
                               (float)viewport_height, 0.0f, -1.0f, 1.0f);
        glViewport(0, 0, viewport_width, viewport_height);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (mouse_button->activated &&
            mouse_button->mouse_button_event.key == FTIC_RIGHT_BUTTON)
        {
            if (directory_history.size > 1)
            {
                platform_reset_directory(
                    &array_back(&directory_history)->directory);
                directory_history.size -= 1;
                current_directory = array_back(&directory_history);
            }
        }

        rendering_properties_clear(font_render);
        V3 text_starting_position =
            v3f(rect.min.x, current_directory->text_y, 0.0f);
        text_starting_position.x += rect.size.width + 20.0f;
        const float scale = 1.0f;
        const float padding_top = 30.0f;
        const float padding_bottom = 10.0f;
        const float quad_height =
            scale * font.pixel_height + padding_top + padding_bottom;
        b8 hit = false;
        i32 index = 0;
        for (; index < (i32)current_directory->directory.sub_directories.size;
             ++index)
        {
            hit = directory_item(
                hit, index, text_starting_position, scale, padding_top,
                padding_bottom, quad_height,
                &current_directory->directory.sub_directories.data[index],
                &font, mouse_move, &hit_index, font_render);
            text_starting_position.y += quad_height;
        }
        b8 skip = false;
        if (mouse_button->mouse_button_event.key == FTIC_LEFT_BUTTON &&
            mouse_button->mouse_button_event.double_clicked)
        {
            if (hit)
            {
                char* path =
                    current_directory->directory.sub_directories.data[hit_index]
                        .path;
                u32 length = (u32)strlen(path);
                path[length++] = '/';
                path[length++] = '*';
                DirectoryPage new_page = { 0 };
                new_page.directory = platform_get_directory(path, length);
                array_push(&directory_history, new_page);
                path[length - 2] = '\0';

                current_directory = array_back(&directory_history);
                skip = true;
            }
        }
        if (!skip)
        {
            for (i32 i = 0; i < (i32)current_directory->directory.files.size;
                 ++i)
            {
                hit = directory_item(
                    hit, i + index, text_starting_position, scale, padding_top,
                    padding_bottom, quad_height,
                    &current_directory->directory.files.data[i], &font,
                    mouse_move, &hit_index, font_render);
                text_starting_position.y += quad_height;
            }
        }
        if (!hit)
        {
            hit_index = -1;
        }
        buffer_set_sub_data(font_render->vertex_buffer_id, GL_ARRAY_BUFFER, 0,
                            sizeof(Vertex) * font_render->vertices.size,
                            font_render->vertices.data);

        if (mouse_wheel->activated)
        {
            f32 total_height = text_starting_position.y - current_directory->text_y;
            f32 z_delta = (f32)mouse_wheel->mouse_wheel_event.z_delta;
            if ((z_delta > 0) || ((total_height + current_directory->offset) > dimensions.y))
            {
                current_directory->offset += z_delta;
            }
            current_directory->offset =
                clampf32_high(current_directory->offset, 0.0f);
        }
        current_directory->scroll_offset +=
            (current_directory->offset - current_directory->scroll_offset) *
            scroll_speed;
        current_directory->text_y = current_directory->scroll_offset;

        for (u32 i = 0; i < rendering_properties.size; ++i)
        {
            rendering_properties_draw(&rendering_properties.data[i], &mvp);
        }
        platform_opengl_swap_buffers(platform);
        poll_event(platform);
        delta_time = (f32)(platform_get_time() - last_time);
    }
    running = false;
    for (u32 i = 0; i < rendering_properties.size; ++i)
    {
        RenderingProperties* rp = rendering_properties.data + i;
        buffer_delete(rp->vertex_buffer_id);
        buffer_delete(rp->index_buffer_id);
    }
    texture_delete(font_texture);
    texture_delete(default_texture);
    shader_destroy(font_shader);
    shader_destroy(shader2);
    platform_opengl_clean(platform);
    threads_destroy(&thread_queue);
    platform_shut_down(platform);
}
