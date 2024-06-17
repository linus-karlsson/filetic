#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

global V4 border_color = { .r = 0.3f, .g = 0.3f, .b = 0.3f, .a = 1.0f };

typedef struct SafeFileArray
{
    DirectoryItemArray array;
    FTicMutex mutex;
} SafeFileArray;

typedef struct FindingCallbackAttribute
{
    ThreadTaskQueue* thread_queue;
    char* start_directory;
    const char* string_to_match;
    u32 start_directory_length;
    u32 string_to_match_length;
    u32 running_id;
    SafeFileArray* file_array;
    SafeFileArray* folder_array;
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

internal char* copy_string(const char* string, const u32 string_length,
                           const u32 extra_length)
{
    char* result =
        (char*)calloc(string_length + extra_length + 1, sizeof(char));
    memcpy(result, string, string_length);
    return result;
}

internal void safe_add_directory_item(const DirectoryItem* item,
                                      FindingCallbackAttribute* arguments,
                                      SafeFileArray* safe_array)
{
    const char* name = item->name;
    const u32 name_length = (u32)strlen(name);
    char* path = item->path;
    if (string_contains(name, name_length, arguments->string_to_match,
                        arguments->string_to_match_length))
    {
        const u32 path_length = (u32)strlen(path);
        DirectoryItem copy = {
            .size = item->size,
            .path = copy_string(path, path_length, 2),
        };
        copy.name = copy.path + path_length - name_length;
        safe_array_push(safe_array, copy);
    }
}

b8 running_callbacks[100] = { 0 };

void finding_callback(void* data)
{
    FindingCallbackAttribute* arguments = (FindingCallbackAttribute*)data;

    b8 should_free_directory = false;
    b8 running = running_callbacks[arguments->running_id];
    Directory directory = { 0 };
    if (running)
    {
        directory = platform_get_directory(arguments->start_directory,
                                           arguments->start_directory_length);
        should_free_directory = true;
    }

    for (u32 i = 0; i < directory.sub_directories.size && running; ++i)
    {

        safe_add_directory_item(&directory.sub_directories.data[i], arguments,
                                arguments->folder_array);

        FindingCallbackAttribute* next_arguments =
            (FindingCallbackAttribute*)calloc(1,
                                              sizeof(FindingCallbackAttribute));

        char* path = directory.sub_directories.data[i].path;
        size_t directory_name_length = strlen(path);

        next_arguments->start_directory =
            copy_string(path, (u32)directory_name_length, 2);
        next_arguments->start_directory[directory_name_length++] = '\\';
        next_arguments->start_directory[directory_name_length++] = '*';
        next_arguments->file_array = arguments->file_array;
        next_arguments->folder_array = arguments->folder_array;
        next_arguments->thread_queue = arguments->thread_queue;
        next_arguments->start_directory_length = (u32)directory_name_length;
        next_arguments->string_to_match = arguments->string_to_match;
        next_arguments->string_to_match_length =
            arguments->string_to_match_length;
        next_arguments->running_id = arguments->running_id;

        ThreadTask task = thread_task(finding_callback, next_arguments);
        thread_tasks_push(next_arguments->thread_queue, &task, 1, NULL);
    }

    for (u32 i = 0; i < directory.files.size && running; ++i)
    {
        safe_add_directory_item(&directory.files.data[i], arguments,
                                arguments->file_array);
    }
    if (should_free_directory)
    {
        platform_reset_directory(&directory);
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

void clear_search_result(SafeFileArray* files)
{
    platform_mutex_lock(&files->mutex);
    for (u32 j = 0; j < files->array.size; ++j)
    {
        free(files->array.data[j].path);
    }
    files->array.size = 0;
    platform_mutex_unlock(&files->mutex);
}

typedef struct RenderingProperties
{
    u32 vertex_buffer_id;
    u32 vertex_buffer_capacity;
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
    rendering_properties.vertex_buffer_capacity =
        rendering_properties.vertices.capacity;
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

void rendering_properties_check_and_grow_buffers(
    RenderingProperties* rendering_properties, IndexArray* index_array)
{
    if (rendering_properties->vertices.size >
        rendering_properties->vertex_buffer_capacity)
    {
        vertex_buffer_orphan(rendering_properties->vertex_buffer_id,
                             rendering_properties->vertices.capacity *
                                 sizeof(Vertex),
                             GL_STREAM_DRAW, NULL);
        rendering_properties->vertex_buffer_capacity =
            rendering_properties->vertices.capacity;
    }

    if (rendering_properties->index_count * 6 > index_array->size)
    {
        index_array->size = 0;
        index_array->capacity *= 2;
        index_array->data = (u32*)calloc(index_array->capacity, sizeof(u32));
        generate_indicies(index_array, 0, index_array->capacity * 6);
        index_buffer_orphan(rendering_properties->index_buffer_id,
                            index_array->capacity * sizeof(u32), GL_STATIC_DRAW,
                            index_array->data);
        free(index_array->data);
    }
}

void rendering_properties_clear(RenderingProperties* rendering_properties)
{
    rendering_properties->index_count = 0;
    rendering_properties->vertices.size = 0;
}

void format_file_size(u64 size_in_bytes, char* output, size_t output_size)
{
    const u64 KB = 1024;
    const u64 MB = 1024 * KB;
    const u64 GB = 1024 * MB;

    if (size_in_bytes >= GB)
    {
        sprintf_s(output, output_size, "%.2f GB", (double)size_in_bytes / GB);
    }
    else if (size_in_bytes >= MB)
    {
        sprintf_s(output, output_size, "%.2f MB", (double)size_in_bytes / MB);
    }
    else if (size_in_bytes >= KB)
    {
        sprintf_s(output, output_size, "%.2f KB", (double)size_in_bytes / KB);
    }
    else
    {
        sprintf_s(output, output_size, "%llu B", size_in_bytes);
    }
}

b8 directory_item(b8 hit, i32 index, const V3 starting_position,
                  const f32 scale, const f32 padding_top, const f32 height,
                  const f32 width, const DirectoryItem* item,
                  const FontTTF* font, const Event* mouse_move, f32 icon_index,
                  i32* hit_index, RenderingProperties* render)
{
    AABB aabb = { .min = v2_v3(starting_position), .size = v2f(width, height) };

    b8 this_hit = false;
    V2 mouse_position = v2f(mouse_move->mouse_move_event.position_x,
                            mouse_move->mouse_move_event.position_y);
    if (!hit && collision_point_in_aabb(mouse_position, &aabb))
    {
        *hit_index = index;
        this_hit = true;
        hit = true;
    }
    const V4 color = this_hit ? v4ic(0.3f) : v4ic(0.1f);
    quad(&render->vertices, starting_position, aabb.size, color, 0.0f);
    render->index_count += 1;

    aabb =
        quad(&render->vertices,
             v3f(starting_position.x + 5.0f, starting_position.y + 3.0f, 0.0f),
             v2f(20.0f, 20.0f), v4i(1.0f), icon_index);
    render->index_count += 1;

    V3 text_position =
        v3_add(starting_position, v3f(padding_top, padding_top, 0.0f));

    text_position.x += aabb.size.x;

    f32 x_advance = 0.0f;
    if (item->size) // NOTE(Linus): Add the size text to the right side
    {
        char buffer[100] = { 0 };
        format_file_size(item->size, buffer, 100);
        x_advance =
            text_x_advance(font->chars, buffer, (u32)strlen(buffer), scale);
        V3 size_text_position = text_position;
        size_text_position.x = starting_position.x + width - x_advance - 5.0f;
        render->index_count += text_generation(
            font->chars, buffer, 1.0f, size_text_position, scale,
            font->pixel_height, NULL, NULL, &render->vertices);
    }

    const u32 text_len = (u32)strlen(item->name);
    i32 i = text_check_length_within_boundary(
        font->chars, item->name, text_len, scale,
        (width - aabb.size.x - padding_top - x_advance));
    b8 too_long = i >= 3;
    char saved_name[4];
    if (too_long)
    {
        i32 j = i - 3;
        saved_name[0] = item->name[j];
        item->name[j++] = '.';
        saved_name[1] = item->name[j];
        item->name[j++] = '.';
        saved_name[2] = item->name[j];
        item->name[j++] = '.';
        saved_name[3] = item->name[j];
        item->name[j] = '\0';
    }
    render->index_count +=
        text_generation(font->chars, item->name, 1.0f, text_position, scale,
                        font->pixel_height, NULL, NULL, &render->vertices);

    if (too_long)
    {
        i32 j = i - 3;
        item->name[j++] = saved_name[0];
        item->name[j++] = saved_name[1];
        item->name[j++] = saved_name[2];
        item->name[j] = saved_name[3];
    }
    return hit;
}

typedef struct DirectoryPage
{
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

void extract_only_aplha_channel(TextureProperties* texture_properties)
{
    const u32 size = texture_properties->height * texture_properties->width;
    const u32 size_in_bytes = size * 4;
    u8* bytes = (u8*)calloc(size, sizeof(u8));
    for (u32 i = 3, j = 0; i < size_in_bytes; i += 4, j++)
    {
        ftic_assert(j < size);
        bytes[j] = texture_properties->bytes[i];
    }
    free(texture_properties->bytes);
    texture_properties->bytes = bytes;
}

void render_input(const FontTTF* font, const char* text, u32 text_length,
                  f32 scale, V3 text_position, f32 cursor_y, b8 active,
                  f64 delta_time, f64* time, RenderingProperties* render)
{
    // Blinking cursor
    if (active)
    {
        *time += delta_time;
        if (*time >= 0.4f)
        {
            const f32 x_advance =
                text_x_advance(font->chars, text, text_length, scale);

            quad(&render->vertices,
                 v3f(text_position.x + x_advance + 1.0f, cursor_y + 2.0f, 0.0f),
                 v2f(2.0f, 16.0f), v4i(1.0f), 0.0f);
            render->index_count++;

            *time = *time >= 0.8f ? 0 : *time;
        }
    }
    if (text_length)
    {
        render->index_count +=
            text_generation(font->chars, text, 1.0f, text_position, scale,
                            font->line_height, NULL, NULL, &render->vertices);
    }
}

void check_and_open_file(const Event* mouse_button_event, const b8 hit,
                         const DirectoryItem* current_files,
                         const i32 file_index)
{
    if (mouse_button_event->mouse_button_event.double_clicked &&
        mouse_button_event->mouse_button_event.key == FTIC_LEFT_BUTTON)
    {
        if (hit)
        {
            const char* file_path = current_files[file_index].path;

            platform_open_file(file_path);
        }
    }
}

void check_and_open_folder(const Event* mouse_button_event, const b8 hit,
                           const i32 index,
                           const DirectoryItem* current_folders,
                           DirectoryArray* directory_history,
                           DirectoryPage** current_directory)
{
    if (mouse_button_event->mouse_button_event.double_clicked &&
        mouse_button_event->mouse_button_event.key == FTIC_LEFT_BUTTON)
    {
        if (hit)
        {
            char* path = current_folders[index].path;
            if (strcmp(path, (*current_directory)->directory.parent) == 0)
            {
                return;
            }
            u32 length = (u32)strlen(path);
            path[length++] = '\\';
            path[length++] = '*';
            DirectoryPage new_page = { 0 };
            new_page.directory = platform_get_directory(path, length);
            array_push(directory_history, new_page);
            path[length - 2] = '\0';

            *current_directory = array_back(directory_history);
        }
    }
}

b8 item_in_view(const f32 position_y, const f32 height, const f32 window_height)
{
    const f32 value = position_y + height;
    return closed_interval(-height, value, window_height + height);
}

b8 directory_item_list(const DirectoryItemArray* sub_directories,
                       const DirectoryItemArray* files, const f32 scale,
                       const FontTTF* font, const f32 padding_top,
                       const f32 item_height, const f32 item_width,
                       const f32 dimension_y, const Event* mouse_move_event,
                       const Event* mouse_button_event, V3* starting_position,
                       RenderingProperties* font_render,
                       DirectoryArray* directory_history,
                       DirectoryPage** current_directory)
{
    b8 folder_hit = false;
    i32 hit_index = -1;
    for (i32 i = 0; i < (i32)sub_directories->size; ++i)
    {
        if (item_in_view(starting_position->y, item_height, dimension_y))
        {
            folder_hit =
                directory_item(folder_hit, i, *starting_position, scale,
                               font->pixel_height + padding_top, item_height,
                               item_width, &sub_directories->data[i], font,
                               mouse_move_event, 3.0f, &hit_index, font_render);
        }
        starting_position->y += item_height;
    }
    check_and_open_folder(mouse_button_event, folder_hit, hit_index,
                          sub_directories->data, directory_history,
                          current_directory);
    b8 file_hit = false;
    hit_index = -1;
    for (i32 i = 0; i < (i32)files->size; ++i)
    {
        if (item_in_view(starting_position->y, item_height, dimension_y))
        {
            file_hit =
                directory_item(file_hit, i, *starting_position, scale,
                               font->pixel_height + padding_top, item_height,
                               item_width, &files->data[i], font,
                               mouse_move_event, 2.0f, &hit_index, font_render);
        }
        starting_position->y += item_height;
    }
    check_and_open_file(mouse_button_event, file_hit, files->data, hit_index);

    return folder_hit || file_hit;
}

void set_scroll_offset(const f32 current_y, const f32 offset_y,
                       const f32 dimension_y, const Event* mouse_wheel_event,
                       f32* offset)
{
    f32 total_height = current_y - offset_y;
    f32 z_delta = (f32)mouse_wheel_event->mouse_wheel_event.z_delta;
    if ((z_delta > 0) || ((total_height + (*offset)) > dimension_y))
    {
        *offset += z_delta;
    }
    *offset = clampf32_high(*offset, 0.0f);
}

f32 smooth_scroll(const f64 delta_time, const f32 offset, f32 scroll_offset)
{
    scroll_offset += (f32)((offset - scroll_offset) * (delta_time * 15.0f));
    return scroll_offset;
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

    texture_load("res/icons/files.png", &texture_properties);
    extract_only_aplha_channel(&texture_properties);
    u32 file_icon_texture =
        texture_create(&texture_properties, GL_RGBA8, GL_RED);
    free(texture_properties.bytes);

    texture_load("res/icons/folder.png", &texture_properties);
    extract_only_aplha_channel(&texture_properties);
    u32 folder_icon_texture =
        texture_create(&texture_properties, GL_RGBA8, GL_RED);
    free(texture_properties.bytes);

    texture_load("res/icons/close.png", &texture_properties);
    extract_only_aplha_channel(&texture_properties);
    u32 close_icon_texture =
        texture_create(&texture_properties, GL_RGBA8, GL_RED);
    free(texture_properties.bytes);

    u32 font_shader = shader_create("./res/shaders/vertex.glsl",
                                    "./res/shaders/font_fragment.glsl");
    u32 shader2 = shader_create("./res/shaders/vertex.glsl",
                                "./res/shaders/fragment.glsl");
    ftic_assert(font_shader);

    IndexArray index_array = { 0 };
    array_create(&index_array, 100 * 6);
    generate_indicies(&index_array, 0, index_array.capacity * 6);
    u32 index_buffer_id = index_buffer_create(
        index_array.data, index_array.size, sizeof(u32), GL_STATIC_DRAW);
    free(index_array.data);

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

    // TODO: make the vertex array growing in OpengGL

    RenderingPropertiesArray rendering_properties = { 0 };
    array_create(&rendering_properties, 1);
    u32 textures[] = { default_texture, font_texture, file_icon_texture,
                       folder_icon_texture, close_icon_texture };
    array_push(&rendering_properties,
               rendering_properties_init(
                   font_shader, textures, static_array_size(textures),
                   index_buffer_id, &vertex_buffer_layout, 100 * 4));

    RenderingProperties* font_render = rendering_properties.data;

    ThreadQueue thread_queue = { 0 };
    thread_init(100000, 8, &thread_queue);

    SafeFileArray file_array = { 0 };
    safe_array_create(&file_array, 10);
    SafeFileArray folder_array = { 0 };
    safe_array_create(&folder_array, 10);

    DirectoryArray directory_history = { 0 };
    array_create(&directory_history, 10);
    const char* dir = "C:\\*";
    DirectoryPage page = { 0 };
    page.directory = platform_get_directory(dir, (u32)strlen(dir));
    array_push(&directory_history, page);
    DirectoryPage* current_directory = array_back(&directory_history);

    Event* key_event = event_subscribe(KEY);
    Event* mouse_move = event_subscribe(MOUSE_MOVE);
    Event* mouse_button = event_subscribe(MOUSE_BUTTON);
    Event* mouse_wheel = event_subscribe(MOUSE_WHEEL);

    CharArray search_buffer = { 0 };
    array_create(&search_buffer, 30);
    b8 search_bar_hit = false;
    f64 search_blinking_time = 0.4f;

    f32 offset = 0.0f;
    f32 scroll_offset = 0.0f;

    i32 hit_index = -1;

    b8 enter_pressed = false;

    u32 running_id = 0;
    u32 last_running_id = 0;

    f64 last_time = platform_get_time();
    f64 delta_time = 0.0f;
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
        V2 dimensions = v2f((f32)viewport_width, (f32)viewport_height);
        V2 mouse_position = v2f(mouse_move->mouse_move_event.position_x,
                                mouse_move->mouse_move_event.position_y);
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

        V3 starting_position = v3f(150.0f, 0.0f, 0.0f);
        AABB rect = quad(&font_render->vertices, starting_position,
                         v2f(2.0f, dimensions.y), border_color, 0.0f);
        ++font_render->index_count;

        V3 search_bar_position = v3f(dimensions.x * 0.6f, 20.0f, 0.0f);

        V3 text_starting_position =
            v3f(rect.min.x, 30.0f + current_directory->scroll_offset, 0.0f);
        text_starting_position.x += 20.0f;

        const f32 scale = 1.0f;
        const f32 padding_top = 2.0f;
        const f32 quad_height = scale * font.pixel_height + padding_top * 5.0f;
        const f32 width =
            (search_bar_position.x - 20.0f) - text_starting_position.x;

        AABB directory_aabb = {
            .min = v2f(text_starting_position.x, 0.0f),
            .size = v2f(width, dimensions.y),
        };

        b8 hit = directory_item_list(
            &current_directory->directory.sub_directories,
            &current_directory->directory.files, scale, &font, padding_top,
            quad_height, width, dimensions.y, mouse_move, mouse_button,
            &text_starting_position, font_render, &directory_history,
            &current_directory);

        quad(&font_render->vertices,
             v3f(directory_aabb.min.x + directory_aabb.size.x, 0.0f, 0.0f),
             v2f(2.0f, dimensions.y), border_color, 0.0f);
        ++font_render->index_count;

        // Search bar
        const f32 search_bar_width = 250.0f;
        const f32 search_bar_input_text_padding = 10.0f;
        AABB search_bar_aabb = quad_with_border(
            &font_render->vertices, &font_render->index_count, border_color,
            search_bar_position, v2f(search_bar_width, 40.0f), 2.0f, 0.0f);

        if (mouse_button->activated)
        {
            if (collision_point_in_aabb(mouse_position, &search_bar_aabb))
            {
                search_bar_hit = true;
            }
            else
            {
                search_bar_hit = false;
                search_blinking_time = 0.4f;
            }
        }

        if (search_bar_hit)
        {
            const CharArray* key_buffer = event_get_key_buffer();
            for (u32 i = 0; i < key_buffer->size; ++i)
            {
                char current_char = key_buffer->data[i];
                if (current_char == '\b') // BACKSPACE
                {
                    if (search_buffer.size)
                    {
                        *array_back(&search_buffer) = 0;
                        --search_buffer.size;
                    }
                }
                else if (current_char == '\r') // ENTER
                {
                    clear_search_result(&file_array);
                    clear_search_result(&folder_array);

                    running_callbacks[last_running_id] = false;

                    if (search_buffer.size)
                    {
                        last_running_id = running_id;
                        running_callbacks[running_id++] = true;
                        running_id %= 100;

                        const char* parent =
                            current_directory->directory.parent;
                        size_t parent_length = strlen(parent);
                        char* dir2 =
                            (char*)calloc(parent_length + 3, sizeof(char));
                        memcpy(dir2, parent, parent_length);
                        dir2[parent_length++] = '\\';
                        dir2[parent_length++] = '*';

                        const char* string_to_match = search_buffer.data;

                        FindingCallbackAttribute* arguments =
                            (FindingCallbackAttribute*)calloc(
                                1, sizeof(FindingCallbackAttribute));
                        arguments->thread_queue = &thread_queue.task_queue;
                        arguments->file_array = &file_array;
                        arguments->folder_array = &folder_array;
                        arguments->start_directory = dir2;
                        arguments->start_directory_length = (u32)parent_length;
                        arguments->string_to_match = string_to_match;
                        arguments->string_to_match_length = search_buffer.size;
                        arguments->running_id = last_running_id;
                        finding_callback(arguments);
                    }
                    search_bar_hit = false;
                    search_blinking_time = 0.4f;
                    break;
                }
                else if (closed_interval(0, (current_char - 32), 96))
                {
                    array_push(&search_buffer, current_char);
                    f32 x_advance =
                        text_x_advance(font.chars, search_buffer.data,
                                       search_buffer.size, scale);
                    if (x_advance >=
                        search_bar_width - (search_bar_input_text_padding * 2))
                    {
                        *array_back(&search_buffer) = 0;
                        --search_buffer.size;
                    }
                }
            }
        }

        V3 search_text_position = search_bar_position;
        search_text_position.x += search_bar_input_text_padding;
        search_text_position.y += scale * font.pixel_height + 10.0f;

        render_input(&font, search_buffer.data, search_buffer.size, scale,
                     search_text_position, search_bar_position.y + 10.0f,
                     search_bar_hit, delta_time, &search_blinking_time,
                     font_render);

        AABB search_result_aabb = { 0 };

        V3 search_result_position = search_bar_position;
        search_result_position.y +=
            search_bar_aabb.size.y + 20.0f + scroll_offset;

        b8 search_hit = false;
        if (file_array.array.size || folder_array.array.size)
        {
            search_result_aabb.min = v2f(search_result_position.x, 0.0f);

            const f32 search_result_width =
                (dimensions.x - search_result_position.x) - 10.0f;

            platform_mutex_lock(&file_array.mutex);
            platform_mutex_lock(&folder_array.mutex);

            search_hit = directory_item_list(
                &folder_array.array, &file_array.array, scale, &font,
                padding_top, quad_height, search_result_width, dimensions.y,
                mouse_move, mouse_button, &search_result_position, font_render,
                &directory_history, &current_directory);

            search_result_aabb.size =
                v2f(dimensions.x - search_result_position.x,
                    (file_array.array.size + folder_array.array.size) *
                        quad_height);

            platform_mutex_unlock(&folder_array.mutex);
            platform_mutex_unlock(&file_array.mutex);
        }
        if (hit || search_hit)
        {
            platform_change_cursor(platform, FTIC_HAND_CURSOR);
        }
        else
        {
            platform_change_cursor(platform, FTIC_NORMAL_CURSOR);
            hit_index = -1;
        }

        rendering_properties_check_and_grow_buffers(font_render, &index_array);

        buffer_set_sub_data(font_render->vertex_buffer_id, GL_ARRAY_BUFFER, 0,
                            sizeof(Vertex) * font_render->vertices.size,
                            font_render->vertices.data);

        if (mouse_wheel->activated)
        {
            if (collision_point_in_aabb(mouse_position, &search_result_aabb))
            {
                set_scroll_offset(search_result_position.y, scroll_offset,
                                  dimensions.y, mouse_wheel, &offset);
            }
            else if (collision_point_in_aabb(mouse_position, &directory_aabb))
            {
                set_scroll_offset(
                    text_starting_position.y, current_directory->scroll_offset,
                    dimensions.y, mouse_wheel, &current_directory->offset);
            }
        }
        current_directory->scroll_offset =
            smooth_scroll(delta_time, current_directory->offset,
                          current_directory->scroll_offset);
        scroll_offset = smooth_scroll(delta_time, offset, scroll_offset);

        for (u32 i = 0; i < rendering_properties.size; ++i)
        {
            rendering_properties_draw(&rendering_properties.data[i], &mvp);
        }
        platform_opengl_swap_buffers(platform);
        event_poll(platform);

        f64 now = platform_get_time();
        delta_time = now - last_time;
        const u32 target_milliseconds = 8;
        const u64 curr_milliseconds = (u64)(delta_time * 1000.0f);
        if (target_milliseconds > curr_milliseconds)
        {
            const u64 milli_to_sleep =
                (u64)(target_milliseconds - curr_milliseconds);
            platform_sleep(milli_to_sleep);
            now = platform_get_time();
            delta_time = now - last_time;
        }
        last_time = now;
    }
    memset(running_callbacks, 0, sizeof(running_callbacks));
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
