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
#include "hash.h"

global V4 clear_color = { .r = 0.1f, .g = 0.1f, .b = 0.1f, .a = 1.0f };
global V4 high_light_color = { .r = 0.2f, .g = 0.2f, .b = 0.2f, .a = 1.0f };
global V4 border_color = { .r = 0.35f, .g = 0.35f, .b = 0.35f, .a = 1.0f };
global V4 lighter_color = { .r = 0.6f, .g = 0.6f, .b = 0.6f, .a = 1.0f };
global f32 border_width = 2.0f;

typedef struct SafeFileArray
{
    DirectoryItemArray array;
    FTicMutex mutex;
} SafeFileArray;

typedef struct B8PtrArray
{
    u32 size;
    u32 capacity;
    b8** data;
} B8PtrArray;

// TODO(Linus): find a better way to do this
typedef struct SelectedItemValues
{
    CharPtrArray paths;
    CharPtrArray names;
    HashTableCharU32 selected_items;
} SelectedItemValues;

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
    memset(files->array.data, 0,
           files->array.size * sizeof(files->array.data[0]));
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

    AABB scissor;

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
    if (rendering_properties->index_count)
    {
        shader_bind(rendering_properties->shader_properties.shader);
        shader_set_mvp(&rendering_properties->shader_properties, mvp);

        int textures[20] = { 0 };
        for (u32 i = 0; i < rendering_properties->texture_count; ++i)
        {
            textures[i] = (int)i;
        }
        int location = glGetUniformLocation(
            rendering_properties->shader_properties.shader, "textures");
        ftic_assert(location != -1);
        glUniform1iv(location, rendering_properties->texture_count, textures);

        for (u32 i = 0; i < rendering_properties->texture_count; ++i)
        {
            texture_bind(rendering_properties->textures[i], (int)i);
        }

        const AABB* scissor = &rendering_properties->scissor;
        glScissor((int)scissor->min.x, (int)scissor->min.y,
                  (int)scissor->size.width, (int)scissor->size.height);

        vertex_array_bind(rendering_properties->vertex_array_id);
        index_buffer_bind(rendering_properties->index_buffer_id);
        glDrawElements(GL_TRIANGLES, rendering_properties->index_count * 6,
                       GL_UNSIGNED_INT, NULL);
    }
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
                  const f32 width, const FontTTF* font,
                  const Event* mouse_button_event,
                  const Event* mouse_move_event, f32 icon_index,
                  DirectoryItem* item, SelectedItemValues* selected_item_values,
                  i32* hit_index, RenderingProperties* render)
{
    AABB aabb = { .min = v2_v3(starting_position), .size = v2f(width, height) };

    u32* check_if_selected = hash_table_get_char_u32(
        &selected_item_values->selected_items, item->name);
    b8 selected = check_if_selected ? true : false;

    b8 this_hit = false;
    V2 mouse_position = v2f(mouse_move_event->mouse_move_event.position_x,
                            mouse_move_event->mouse_move_event.position_y);
    if (collision_point_in_aabb(mouse_position, &aabb))
    {
        if (!hit)
        {
            *hit_index = index;
            this_hit = true;
            hit = true;
        }
        const MouseButtonEvent* event = &mouse_button_event->mouse_button_event;
        if (!check_if_selected && mouse_button_event->activated &&
            event->action == 0 &&
            (event->key == FTIC_LEFT_BUTTON || event->key == FTIC_RIGHT_BUTTON))
        {
            if (!check_if_selected)
            {
                const u32 name_length = (u32)strlen(item->name);
                char* name = (char*)calloc(name_length + 3, sizeof(char));
                memcpy(name, item->name, name_length);
                hash_table_insert_char_u32(
                    &selected_item_values->selected_items, name, 1);
                array_push(&selected_item_values->names, name);
                array_push(&selected_item_values->paths, item->path);
                selected = true;
            }
        }
    }
    const V4 color = (this_hit || selected) ? v4ic(0.3f) : v4ic(0.1f);
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

internal b8 item_in_view(const f32 position_y, const f32 height,
                         const f32 window_height)
{
    const f32 value = position_y + height;
    return closed_interval(-height, value, window_height + height);
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
            path[length - 1] = '\0';

            *current_directory = array_back(directory_history);
        }
    }
}

typedef struct DirectoryItemListReturnValue
{
    u32 count;
    f32 total_height;
    b8 hit;
} DirectoryItemListReturnValue;

DirectoryItemListReturnValue directory_item_list(
    const DirectoryItemArray* sub_directories, const DirectoryItemArray* files,
    const f32 scale, const FontTTF* font, const f32 padding_top,
    const f32 item_height, const f32 item_width, const f32 dimension_y,
    const Event* mouse_move_event, const Event* mouse_button_event,
    V3* starting_position, SelectedItemValues* selected_item_values,
    RenderingProperties* font_render, DirectoryArray* directory_history,
    DirectoryPage** current_directory)
{
    const f32 starting_y = starting_position->y;

    b8 folder_hit = false;
    i32 hit_index = -1;
    for (i32 i = 0; i < (i32)sub_directories->size; ++i)
    {
        if (item_in_view(starting_position->y, item_height, dimension_y))
        {
            folder_hit = directory_item(
                folder_hit, i, *starting_position, scale,
                font->pixel_height + padding_top, item_height, item_width, font,
                mouse_button_event, mouse_move_event, 3.0f,
                &sub_directories->data[i], selected_item_values, &hit_index,
                font_render);
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
            file_hit = directory_item(
                file_hit, i, *starting_position, scale,
                font->pixel_height + padding_top, item_height, item_width, font,
                mouse_button_event, mouse_move_event, 2.0f, &files->data[i],
                selected_item_values, &hit_index, font_render);
        }
        starting_position->y += item_height;
    }
    check_and_open_file(mouse_button_event, file_hit, files->data, hit_index);

    return (DirectoryItemListReturnValue){
        .count = sub_directories->size + files->size,
        .total_height = starting_position->y - starting_y,
        .hit = (folder_hit || file_hit),
    };
}

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

void set_scroll_offset(const u32 item_count, const f32 item_height,
                       const f32 area_y, const Event* mouse_wheel_event,
                       f32* offset)
{
    const f32 total_height = item_count * item_height;
    const f32 z_delta = (f32)mouse_wheel_event->mouse_wheel_event.z_delta;
    *offset += z_delta;
    *offset = clampf32_high(*offset, 0.0f);

    f32 low = area_y - (total_height + item_height);
    low = clampf32_high(low, 0.0f);
    *offset = clampf32_low(*offset, low);
}

f32 smooth_scroll(const f64 delta_time, const f32 offset, f32 scroll_offset)
{
    scroll_offset += (f32)((offset - scroll_offset) * (delta_time * 15.0f));
    return scroll_offset;
}

internal u64 u64_hash_function(const void* data, u32 len, u64 seed)
{
    return *(u64*)data;
}

void reset_selected_items(SelectedItemValues* selected_item_values)
{
    for (u32 i = 0; i < selected_item_values->names.size; ++i)
    {
        free(selected_item_values->names.data[i]);
    }
    hash_table_clear_char_u32(&selected_item_values->selected_items);
    selected_item_values->names.size = 0;
    selected_item_values->paths.size = 0;
}

void reload_directory(DirectoryPage* directory_page)
{
    char* path = directory_page->directory.parent;
    u32 length = (u32)strlen(path);
    path[length++] = '\\';
    path[length++] = '*';
    Directory reloaded_directory = platform_get_directory(path, length);
    path[length - 2] = '\0';
    path[length - 1] = '\0';
    platform_reset_directory(&directory_page->directory);
    directory_page->directory = reloaded_directory;
}

b8 is_ctrl_and_key_pressed(const Event* event, u32 key)
{
    return event->activated && event->key_event.action == 1 &&
           event->key_event.ctrl_pressed && event->key_event.key == key;
}

u32 load_icon_as_only_red(const char* file_path)
{
    TextureProperties texture_properties = { 0 };
    texture_load(file_path, &texture_properties);
    extract_only_aplha_channel(&texture_properties);
    u32 icon_texture = texture_create(&texture_properties, GL_RGBA8, GL_RED);
    free(texture_properties.bytes);
    return icon_texture;
}

AABB ui_add_border(RenderingProperties* render, V3 position, V2 size)
{
    AABB border_aabb =
        quad(&render->vertices, position, size, border_color, 0.0f);
    ++render->index_count;
    return border_aabb;
}

f32 lerp_f32(const f32 a, const f32 b, const f32 t)
{
    return a + (t * (b - a));
}

void add_scroll_bar(V3 position, const f32 dimension_y, const f32 area_y,
                    const f32 total_height, const f32 item_height,
                    const f32 scroll_offset, RenderingProperties* render)
{
    const f32 scroll_bar_width = 5.0f;
    position.x -= scroll_bar_width;

    quad(&render->vertices, position, v2f(scroll_bar_width, area_y),
         high_light_color, 0.0f);
    render->index_count++;

    if (area_y < total_height)
    {
        const f32 high = 0.0f;
        const f32 low = area_y - (total_height + item_height);
        const f32 p = (scroll_offset - low) / (high - low);
        const V2 scroll_bar_dimensions =
            v2f(scroll_bar_width, area_y * (area_y / total_height));

        position.y =
            lerp_f32(dimension_y - scroll_bar_dimensions.height, position.y, p);

        quad(&render->vertices, position, scroll_bar_dimensions, lighter_color,
             0.0f);
        render->index_count++;
    }
}

const char* get_file_extension(const char* path, const u32 path_length)
{
    for (i32 i = path_length - 1; i >= 0; --i)
    {
        char current_char = path[i];
        if (current_char == '\\' || current_char == '/')
        {
            return NULL;
        }
        else if (current_char == '.')
        {
            return path + i;
        }
    }
    return NULL;
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
    u8 pixels[4] = { UINT8_MAX, UINT8_MAX, UINT8_MAX, UINT8_MAX };
    texture_properties.bytes = pixels;
    u32 default_texture_red =
        texture_create(&texture_properties, GL_RGBA8, GL_RED);
    u32 default_texture =
        texture_create(&texture_properties, GL_RGBA8, GL_RGBA);

    u32 file_icon_texture = load_icon_as_only_red("res/icons/files.png");
    u32 folder_icon_texture = load_icon_as_only_red("res/icons/folder.png");
    u32 close_icon_texture = load_icon_as_only_red("res/icons/close.png");
    u32 arrow_icon_texture = load_icon_as_only_red("res/icons/arrow.png");

    u32 main_shader = shader_create("./res/shaders/vertex.glsl",
                                    "./res/shaders/font_fragment.glsl");

    u32 preview_shader = shader_create("./res/shaders/vertex.glsl",
                                       "./res/shaders/fragment.glsl");
    ftic_assert(main_shader);
    ftic_assert(preview_shader);

    int location = glGetUniformLocation(preview_shader, "textures");

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
    array_create(&rendering_properties, 2);
    u32 main_textures[] = { default_texture_red, font_texture,
                            file_icon_texture,   folder_icon_texture,
                            close_icon_texture,  arrow_icon_texture };
    array_push(&rendering_properties,
               rendering_properties_init(
                   main_shader, main_textures, static_array_size(main_textures),
                   index_buffer_id, &vertex_buffer_layout, 100 * 4));

    u32 preview_textures[10] = { default_texture };
    array_push(&rendering_properties,
               rendering_properties_init(preview_shader, preview_textures,
                                         static_array_size(preview_textures),
                                         index_buffer_id, &vertex_buffer_layout,
                                         100 * 4));

    RenderingProperties* main_render = rendering_properties.data;
    RenderingProperties* preview_render = rendering_properties.data + 1;
    preview_render->texture_count = 1;

    ThreadQueue thread_queue = { 0 };
    thread_init(100000, 8, &thread_queue);

    SafeFileArray file_array = { 0 };
    safe_array_create(&file_array, 10);
    SafeFileArray folder_array = { 0 };
    safe_array_create(&folder_array, 10);

    DirectoryArray directory_history = { 0 };
    array_create(&directory_history, 10);
    // const char* dir = "C:\\Users\\linus\\dev\\*";
    const char* dir = "C:\\*";
    DirectoryPage page = { 0 };
    page.directory = platform_get_directory(dir, (u32)strlen(dir));
    array_push(&directory_history, page);
    DirectoryPage* current_directory = array_back(&directory_history);

    Event* key_event = event_subscribe(KEY);
    Event* mouse_move = event_subscribe(MOUSE_MOVE);
    Event* mouse_button = event_subscribe(MOUSE_BUTTON);
    Event* mouse_wheel = event_subscribe(MOUSE_WHEEL);

    // HashTableU64Char items_selected = hash_table_create_u64_char(100,
    // u64_hash_function);
    //
    // TODO(Linus): Make a set for this instead

    SelectedItemValues selected_item_values = { 0 };
    array_create(&selected_item_values.paths, 10);
    array_create(&selected_item_values.names, 10);
    selected_item_values.selected_items =
        hash_table_create_char_u32(100, hash_murmur);

    CharArray search_buffer = { 0 };
    array_create(&search_buffer, 30);
    b8 search_bar_hit = false;
    f64 search_blinking_time = 0.4f;

    CharPtrArray pasted_paths = { 0 };
    array_create(&pasted_paths, 10);

    f32 offset = 0.0f;
    f32 scroll_offset = 0.0f;

    u32 running_id = 0;
    u32 last_running_id = 0;

    f64 last_time = platform_get_time();
    f64 delta_time = 0.0f;
    MVP mvp = { 0 };
    mvp.view = m4d();
    mvp.model = m4d();
    enable_gldebugging();
    glEnable(GL_BLEND);
    glEnable(GL_SCISSOR_TEST);
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
        glClearColor(clear_color.r, clear_color.g, clear_color.b,
                     clear_color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        rendering_properties_clear(main_render);
        rendering_properties_clear(preview_render);

        main_render->scissor.size = dimensions;

        if (mouse_button->activated &&
            mouse_button->mouse_button_event.action == 0 &&
            mouse_button->mouse_button_event.key == FTIC_RIGHT_BUTTON)
        {
            reset_selected_items(&selected_item_values);
        }

        if (selected_item_values.paths.size)
        {
            const MouseButtonEvent* event = &mouse_button->mouse_button_event;

            if (is_ctrl_and_key_pressed(key_event, FTIC_KEY_C))
            {
                platform_copy_to_clipboard(&selected_item_values.paths);
            }
            else if (selected_item_values.paths.size == 1 &&
                     is_ctrl_and_key_pressed(key_event, FTIC_KEY_D))
            {
                if (preview_render->texture_count > 1)
                {
                    texture_delete(
                        preview_render
                            ->textures[--preview_render->texture_count]);
                }
                const char* path = selected_item_values.paths.data[0];
                const char* extension =
                    get_file_extension(path, (u32)strlen(path));
                if (!strcmp(extension, ".png") || !strcmp(extension, ".jpg"))
                {
                    texture_load(path, &texture_properties);
                    ftic_assert(texture_properties.bytes);
                    u32 texture =
                        texture_create(&texture_properties, GL_RGBA8, GL_RGBA);
                    preview_render->textures[preview_render->texture_count++] =
                        texture;
                    free(texture_properties.bytes);
                    reset_selected_items(&selected_item_values);
                }
            }
            else if (key_event->activated && key_event->key_event.action == 1 &&
                     key_event->key_event.key == FTIC_KEY_ESCAPE)
            {
                platform_delete_files(&selected_item_values.paths);
                reload_directory(current_directory);
            }
            else if (!key_event->key_event.ctrl_pressed &&
                     mouse_button->activated && event->action == 0 &&
                     event->key == FTIC_LEFT_BUTTON)
            {
                reset_selected_items(&selected_item_values);
            }
        }
        if (is_ctrl_and_key_pressed(key_event, FTIC_KEY_V))
        {
            platform_paste_from_clipboard(&pasted_paths);
            if (pasted_paths.size)
            {
                platform_paste_to_directory(
                    &pasted_paths, current_directory->directory.parent);
                for (u32 i = 0; i < pasted_paths.size; ++i)
                {
                    free(pasted_paths.data[i]);
                }
                pasted_paths.size = 0;
                reset_selected_items(&selected_item_values);
                reload_directory(current_directory);
            }
        }

        V3 starting_position = v3f(150.0f, 0.0f, 0.0f);
        AABB rect = ui_add_border(main_render, starting_position,
                                  v2f(border_width, dimensions.y));

        V3 search_bar_position = v3f(dimensions.x * 0.6f, 10.0f, 0.0f);

        V3 parent_directory_path_position =
            v3f(rect.min.x + border_width, 30.0f, 0.0f);
        V3 text_starting_position = parent_directory_path_position;

        text_starting_position.x += 10.0f;
        text_starting_position.y += font.pixel_height;

        const f32 width =
            ((search_bar_position.x - 10.0f) - text_starting_position.x);

        AABB directory_aabb = {
            .min = v2_v3(text_starting_position),
            .size = v2f(width, dimensions.y - text_starting_position.y),
        };
        text_starting_position.y += current_directory->scroll_offset;

        const f32 scale = 1.0f;
        const f32 padding_top = 2.0f;
        const f32 quad_height = scale * font.pixel_height + padding_top * 5.0f;

        DirectoryItemListReturnValue main_list_return_value =
            directory_item_list(
                &current_directory->directory.sub_directories,
                &current_directory->directory.files, scale, &font, padding_top,
                quad_height, width - 5.0f, dimensions.y, mouse_move,
                mouse_button, &text_starting_position, &selected_item_values,
                main_render, &directory_history, &current_directory);

        const f32 back_drop_height =
            parent_directory_path_position.y + font.pixel_height;
        quad(&main_render->vertices,
             v3f(parent_directory_path_position.x, 0.0f, 0.0f),
             v2f(width + 10.0f, back_drop_height), high_light_color, 0.0f);
        main_render->index_count++;

        parent_directory_path_position.x += 10.0f;

        main_render->index_count += text_generation(
            font.chars, current_directory->directory.parent, 1.0f,
            parent_directory_path_position, scale, font.pixel_height, NULL,
            NULL, &main_render->vertices);

        AABB right_border_aabb = ui_add_border(
            main_render,
            v3f(directory_aabb.min.x + directory_aabb.size.x, 0.0f, 0.0f),
            v2f(border_width, dimensions.y));

        // Search bar
        const f32 search_bar_width = 250.0f;
        const f32 search_bar_input_text_padding = 10.0f;
        AABB search_bar_aabb = {
            .min = v2_v3(search_bar_position),
            .size = v2f(search_bar_width, 40.0f),
        };

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
                        offset = 0.0f;
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
        V3 search_input_position = search_text_position;

        V3 search_result_position = search_bar_position;
        search_result_position.y += search_bar_aabb.size.y + 20.0f;

        AABB search_result_aabb = {
            .min = v2_v3(search_result_position),
        };
        search_result_aabb.size = v2_sub(dimensions, search_result_aabb.min);
        search_result_position.y += scroll_offset;

        DirectoryItemListReturnValue search_list_return_value = { 0 };
        if (file_array.array.size || folder_array.array.size)
        {
            const f32 search_result_width =
                (dimensions.x - search_result_position.x) - 10.0f;

            platform_mutex_lock(&file_array.mutex);
            platform_mutex_lock(&folder_array.mutex);

            search_list_return_value = directory_item_list(
                &folder_array.array, &file_array.array, scale, &font,
                padding_top, quad_height, search_result_width, dimensions.y,
                mouse_move, mouse_button, &search_result_position,
                &selected_item_values, main_render, &directory_history,
                &current_directory);

            platform_mutex_unlock(&folder_array.mutex);
            platform_mutex_unlock(&file_array.mutex);
        }

        AABB side_under_border_aabb = {
            .min = v2f(right_border_aabb.min.x,
                       search_bar_aabb.min.y + search_bar_aabb.size.y + 10.0f),
            .size = v2f(dimensions.x - right_border_aabb.min.x, border_width),
        };

        quad(&main_render->vertices,
             v3f(side_under_border_aabb.min.x + border_width, 0.0f, 0.0f),
             v2f(side_under_border_aabb.size.x, side_under_border_aabb.min.y),
             clear_color, 0.0f);
        main_render->index_count++;

        render_input(&font, search_buffer.data, search_buffer.size, scale,
                     search_input_position, search_bar_position.y + 10.0f,
                     search_bar_hit, delta_time, &search_blinking_time,
                     main_render);

        quad_with_border(&main_render->vertices, &main_render->index_count,
                         border_color, v3_v2(search_bar_aabb.min),
                         search_bar_aabb.size, border_width, 0.0f);

        ui_add_border(main_render, v3_v2(side_under_border_aabb.min),
                      side_under_border_aabb.size);

        V3 back_button_position = v3f(10.0f, 10.0f, 0.0f);

        AABB button_aabb = {
            .min = v2_v3(back_button_position),
            .size = v2i(search_bar_aabb.size.y),
        };

        V4 button_color = clear_color;

        if (directory_history.size > 1 &&
            collision_point_in_aabb(mouse_position, &button_aabb))
        {
            const MouseButtonEvent* event = &mouse_button->mouse_button_event;
            if (mouse_button->activated && event->action == 0 &&
                event->key == FTIC_LEFT_BUTTON)
            {
                platform_reset_directory(
                    &array_back(&directory_history)->directory);
                directory_history.size -= 1;
                current_directory = array_back(&directory_history);
                current_directory->scroll_offset = 0.0f;
                reset_selected_items(&selected_item_values);
                reload_directory(current_directory);
            }
            else
            {
                button_color = high_light_color;
            }
            main_list_return_value.hit = true;
        }
        quad(&main_render->vertices, back_button_position, button_aabb.size,
             button_color, 0.0f);
        main_render->index_count++;

        back_button_position.x += 9.0f;
        back_button_position.y += 10.0f;

        const V4 back_icon_color =
            directory_history.size == 1 ? border_color : v4ic(1.0f);

        quad(&main_render->vertices, back_button_position, v2i(20.0f),
             back_icon_color, 5.0f);
        main_render->index_count++;

        V3 back_button_border_position =
            v3f(0.0f, side_under_border_aabb.min.y, 0.0f);
        ui_add_border(main_render, back_button_border_position,
                      v2f(rect.min.x, border_width));

        V3 scroll_bar_position =
            v3f(right_border_aabb.min.x, directory_aabb.min.y, 0.0f);
        f32 area_y = directory_aabb.size.y;
        f32 total_height = main_list_return_value.count * quad_height;
        add_scroll_bar(scroll_bar_position, dimensions.y, area_y, total_height,
                       quad_height, current_directory->scroll_offset,
                       main_render);

        scroll_bar_position =
            v3f(dimensions.width, search_result_aabb.min.y - 8.0f, 0.0f);
        area_y = search_result_aabb.size.y + 8.0f;
        total_height = search_list_return_value.count * quad_height;
        add_scroll_bar(scroll_bar_position, dimensions.y, area_y, total_height,
                       quad_height, scroll_offset, main_render);

        if (main_list_return_value.hit || search_list_return_value.hit)
        {
            platform_change_cursor(platform, FTIC_HAND_CURSOR);
        }
        else
        {
            platform_change_cursor(platform, FTIC_NORMAL_CURSOR);
        }

        if (preview_render->texture_count > 1)
        {
            V2 image_dimensions = v2f((f32)texture_properties.width,
                                      (f32)texture_properties.height);

            const V2 total_preview_dimensions = v2_s_sub(dimensions, 40.0f);
            const V2 ratios =
                v2_div(total_preview_dimensions, image_dimensions);
            f32 scale_factor =
                ratios.width < ratios.height ? ratios.width : ratios.height;
            V2 preview_dimensions = { 0 };
            if (scale_factor < 1.0f)
            {
                v2_s_multi_equal(&image_dimensions, scale_factor);
            }
            preview_dimensions = image_dimensions;
            v2_s_add_equal(&preview_dimensions, border_width * 2.0f);

            V3 preview_position = v3_v2(v2_s_multi(dimensions, 0.5f));
            v3_sub_equal(&preview_position,
                         v3_v2(v2_s_multi(preview_dimensions, 0.5f)));

            AABB* scissor = &preview_render->scissor;
            scissor->min = v2_v3(preview_position);
            scissor->min.y = dimensions.y - scissor->min.y;
            scissor->min.y -= preview_dimensions.y;
            scissor->size = preview_dimensions;
            scissor->size.width += border_width;

            AABB preview_aabb = quad_with_border(&preview_render->vertices,
                             &preview_render->index_count, border_color,
                             preview_position, preview_dimensions, border_width,
                             0.0f);
            v3_add_equal(&preview_position, v3_v2(v2i(border_width)));
            v2_s_sub_equal(&preview_dimensions, border_width * 2.0f);

            quad(&preview_render->vertices, preview_position,
                 preview_dimensions, clear_color, 0.0f);
            preview_render->index_count++;

            quad(&preview_render->vertices, preview_position,
                 preview_dimensions, v4i(1.0f), 1.0f);
            preview_render->index_count++;

            const MouseButtonEvent* event = &mouse_button->mouse_button_event;
            if (!collision_point_in_aabb(mouse_position, &preview_aabb) &&
                mouse_button->activated && event->action == 0 &&
                event->key == FTIC_LEFT_BUTTON)
            {
                if (preview_render->texture_count > 1)
                {
                    texture_delete(
                        preview_render
                            ->textures[--preview_render->texture_count]);
                }
            }
        }

        rendering_properties_check_and_grow_buffers(main_render, &index_array);
        rendering_properties_check_and_grow_buffers(preview_render,
                                                    &index_array);

        buffer_set_sub_data(main_render->vertex_buffer_id, GL_ARRAY_BUFFER, 0,
                            sizeof(Vertex) * main_render->vertices.size,
                            main_render->vertices.data);

        buffer_set_sub_data(preview_render->vertex_buffer_id, GL_ARRAY_BUFFER,
                            0, sizeof(Vertex) * preview_render->vertices.size,
                            preview_render->vertices.data);

        if (mouse_wheel->activated)
        {
            if (collision_point_in_aabb(mouse_position, &search_result_aabb))
            {
                set_scroll_offset(search_list_return_value.count, quad_height,
                                  search_result_aabb.size.y, mouse_wheel,
                                  &offset);
            }
            else if (collision_point_in_aabb(mouse_position, &directory_aabb))
            {
                set_scroll_offset(main_list_return_value.count, quad_height,
                                  directory_aabb.size.y, mouse_wheel,
                                  &current_directory->offset);
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
    texture_delete(default_texture_red);
    shader_destroy(main_shader);
    shader_destroy(preview_shader);
    platform_opengl_clean(platform);
    threads_destroy(&thread_queue);
    platform_shut_down(platform);
}
