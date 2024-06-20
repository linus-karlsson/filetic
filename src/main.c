#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb/stb_truetype.h>
#include <math.h>

#include "define.h"
#include "platform/platform.h"
#include "ftic_window.h"
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

#define COPY_OPTION_INDEX 0
#define PASTE_OPTION_INDEX 1
#define DELETE_OPTION_INDEX 2
#define PROPERTIES_OPTION_INDEX 3

global const V4 clear_color = { .r = 0.1f, .g = 0.1f, .b = 0.1f, .a = 1.0f };
global const V4 high_light_color = {
    .r = 0.2f, .g = 0.2f, .b = 0.2f, .a = 1.0f
};
global const V4 border_color = {
    .r = 0.35f, .g = 0.35f, .b = 0.35f, .a = 1.0f
};
global const V4 lighter_color = {
    .r = 0.55f, .g = 0.55f, .b = 0.55f, .a = 1.0f
};
global const V4 bright_color = { .r = 0.7f, .g = 0.7f, .b = 0.7f, .a = 1.0f };
global const f32 border_width = 2.0f;

global const f32 PI = 3.141592653589f;

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

typedef struct U32Array
{
    u32 size;
    u32 capacity;
    u32* data;
} U32Array;

// TODO(Linus): find a better way to do this
typedef struct SelectedItemValues
{
    CharPtrArray paths;
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

typedef struct ScrollBar
{
    f32 mouse_pointer_offset;
    b8 dragging;
} ScrollBar;

typedef struct ApplicationContext
{
    GLFWwindow* window;
    FontTTF font;

    DirectoryArray directory_history;

    Event* key_event;
    Event* mouse_move;
    Event* mouse_button;
    Event* mouse_wheel;

    V2 dimensions;
    V2 mouse_position;

    f64 last_time;
    f64 delta_time;
    MVP mvp;
} ApplicationContext;

DirectoryPage* current_directory(DirectoryArray* history)
{
    return array_back(history);
}

typedef struct SearchPage
{
    f32 offset;
    f32 scroll_offset;
    f64 search_blinking_time;

    AABB search_bar_aabb;
    AABB search_result_aabb;

    CharArray search_buffer;
    SafeFileArray search_result_file_array;
    SafeFileArray search_result_folder_array;
    RenderingProperties* render;

    ScrollBar scroll_bar;

    u32 running_id;
    u32 last_running_id;

    b8 search_bar_hit;
} SearchPage;

typedef struct DropDownMenu
{
    f32 x;
    V3 position;
    AABB aabb;
    u32 item_count;
    CharPtrArray options;
    RenderingProperties* render;
    b8 (*menu_options_selection)(u32 index, b8 hit, b8 should_close,
                                 b8 mouse_button_clicked, V4* text_color,
                                 void* data);
} DropDownMenu;

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

void log_f32(const char* message, const f32 value)
{
    char buffer[100] = { 0 };
    sprintf_s(buffer, 100, "%s%f", message, value);
    log_message(buffer, strlen(buffer));
}

f32 ease_out_elastic(const f32 x)
{
    if (x == 0 || x == 1)
    {
        return x;
    }
    const f32 c4 = (2.0f * PI) / 3.0f;

    return powf(2.0f, -10.0f * x) * sinf((x * 10.0f - 0.75f) * c4) + 1.0f;
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

void search_page_clear_search_result(SearchPage* page)
{
    clear_search_result(&page->search_result_file_array);
    clear_search_result(&page->search_result_folder_array);
}

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

void rendering_properties_array_clear(
    RenderingPropertiesArray* rendering_properties)
{
    for (u32 i = 0; i < rendering_properties->size; ++i)
    {
        rendering_properties_clear(&rendering_properties->data[i]);
    }
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

void swap_strings(char* first, char* second)
{
    char temp[4] = { 0 };
    const size_t length = sizeof(temp);
    memcpy(temp, first, length);
    memcpy(first, second, length);
    memcpy(second, temp, length);
}

b8 directory_item(b8 hit, i32 index, const V3 starting_position,
                  const f32 padding_top, const f32 height, const f32 width,
                  const FontTTF* font, const Event* mouse_button_event,
                  const Event* mouse_move_event, const f32 icon_index,
                  const b8 check_colission, DirectoryItem* item,
                  SelectedItemValues* selected_item_values, i32* hit_index,
                  RenderingProperties* render)
{
    AABB aabb = { .min = v2_v3(starting_position), .size = v2f(width, height) };

    u32* check_if_selected = hash_table_get_char_u32(
        &selected_item_values->selected_items, item->path);
    b8 selected = check_if_selected ? true : false;

    b8 this_hit = false;
    V2 mouse_position = v2f(mouse_move_event->mouse_move_event.position_x,
                            mouse_move_event->mouse_move_event.position_y);
    if (check_colission && collision_point_in_aabb(mouse_position, &aabb))
    {
        if (!hit)
        {
            *hit_index = index;
            this_hit = true;
            hit = true;
        }
        const MouseButtonEvent* event = &mouse_button_event->mouse_button_event;
        if (!check_if_selected && mouse_button_event->activated &&
            event->action == FTIC_RELEASE &&
            (event->button == FTIC_MOUSE_BUTTON_1 ||
             event->button == FTIC_MOUSE_BUTTON_2))
        {
            if (!check_if_selected)
            {
                const u32 path_length = (u32)strlen(item->path);
                char* path = (char*)calloc(path_length + 3, sizeof(char));
                memcpy(path, item->path, path_length);
                hash_table_insert_char_u32(
                    &selected_item_values->selected_items, path, 1);
                array_push(&selected_item_values->paths, path);
                selected = true;
            }
        }
    }
    const V4 color = (this_hit || selected) ? border_color : clear_color;
    // Backdrop
    quad(&render->vertices, starting_position, aabb.size, color, 0.0f);
    render->index_count += 1;

    // Icon
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
            text_x_advance(font->chars, buffer, (u32)strlen(buffer), 1.0f);
        V3 size_text_position = text_position;
        size_text_position.x = starting_position.x + width - x_advance - 5.0f;
        render->index_count +=
            text_generation(font->chars, buffer, 1.0f, size_text_position, 1.0f,
                            font->pixel_height, NULL, NULL, &render->vertices);
    }

    const u32 text_len = (u32)strlen(item->name);
    const i32 i = text_check_length_within_boundary(
        font->chars, item->name, text_len, 1.0f,
        (width - aabb.size.x - padding_top - x_advance));
    const b8 too_long = i >= 3;
    char saved_name[4] = "...";
    if (too_long)
    {
        i32 j = i - 3;
        swap_strings(item->name + j, saved_name); // Truncate
    }
    render->index_count +=
        text_generation(font->chars, item->name, 1.0f, text_position, 1.0f,
                        font->pixel_height, NULL, NULL, &render->vertices);
    if (too_long)
    {
        memcpy(item->name + (i - 3), saved_name, sizeof(saved_name));
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
        mouse_button_event->mouse_button_event.button == FTIC_MOUSE_BUTTON_1)
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
                           DirectoryArray* directory_history)
{
    if (mouse_button_event->mouse_button_event.double_clicked &&
        mouse_button_event->mouse_button_event.button == FTIC_MOUSE_BUTTON_1)
    {
        if (hit)
        {
            char* path = current_folders[index].path;
            if (strcmp(path, current_directory(directory_history)
                                 ->directory.parent) == 0)
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
        }
    }
}

typedef struct DirectoryItemListReturnValue
{
    u32 count;
    f32 total_height;
    u32 hit_index;
    b8 hit;
} DirectoryItemListReturnValue;

DirectoryItemListReturnValue
item_list(const ApplicationContext* application,
          const DirectoryItemArray* items, const f32 icon_index,
          const b8 check_collision, V3 starting_position, const f32 item_width,
          const f32 item_height, const f32 padding,
          SelectedItemValues* selected_item_values, RenderingProperties* render)
{
    const f32 starting_y = starting_position.y;
    b8 hit = false;
    i32 hit_index = -1;
    for (i32 i = 0; i < (i32)items->size; ++i)
    {
        if (item_in_view(starting_position.y, item_height,
                         application->dimensions.y))
        {
            hit = directory_item(
                hit, i, starting_position,
                application->font.pixel_height + padding, item_height,
                item_width, &application->font, application->mouse_button,
                application->mouse_move, icon_index, check_collision,
                &items->data[i], selected_item_values, &hit_index, render);
        }
        starting_position.y += item_height;
    }
    return (DirectoryItemListReturnValue){
        .count = items->size,
        .total_height = starting_position.y - starting_y,
        .hit_index = hit_index,
        .hit = hit,
    };
}

DirectoryItemListReturnValue
folder_item_list(const ApplicationContext* application,
                 const DirectoryItemArray* folders, const b8 check_collision,
                 V3 starting_position, const f32 item_width,
                 const f32 item_height, const f32 padding,
                 SelectedItemValues* selected_item_values,
                 RenderingProperties* render, DirectoryArray* directory_history)
{
    DirectoryItemListReturnValue list_return = item_list(
        application, folders, 3.0f, check_collision, starting_position,
        item_width, item_height, padding, selected_item_values, render);

    check_and_open_folder(application->mouse_button, list_return.hit,
                          list_return.hit_index, folders->data,
                          directory_history);
    return list_return;
}

DirectoryItemListReturnValue files_item_list(
    const ApplicationContext* application, const DirectoryItemArray* files,
    const b8 check_collision, V3 starting_position, const f32 item_width,
    const f32 item_height, const f32 padding,
    SelectedItemValues* selected_item_values, RenderingProperties* render)
{
    DirectoryItemListReturnValue list_return = item_list(
        application, files, 2.0f, check_collision, starting_position,
        item_width, item_height, padding, selected_item_values, render);

    check_and_open_file(application->mouse_button, list_return.hit, files->data,
                        list_return.hit_index);

    return list_return;
}

DirectoryItemListReturnValue directory_item_list(
    const ApplicationContext* application,
    const DirectoryItemArray* sub_directories, const DirectoryItemArray* files,
    const b8 check_collision, V3 starting_position, const f32 item_width,
    const f32 item_height, const f32 padding,
    SelectedItemValues* selected_item_values, RenderingProperties* render,
    DirectoryArray* directory_history)
{
    DirectoryItemListReturnValue folder_list_return =
        folder_item_list(application, sub_directories, check_collision,
                         starting_position, item_width, item_height, padding,
                         selected_item_values, render, directory_history);

    starting_position.y += folder_list_return.total_height;

    DirectoryItemListReturnValue files_list_return = files_item_list(
        application, files, check_collision, starting_position, item_width,
        item_height, padding, selected_item_values, render);

    return (DirectoryItemListReturnValue){
        .count = folder_list_return.count + files_list_return.count,
        .total_height =
            folder_list_return.total_height + files_list_return.total_height,
        .hit = (folder_list_return.hit || files_list_return.hit),
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
    const f32 y_offset = mouse_wheel_event->mouse_wheel_event.y_offset * 100.0f;
    log_f32("Y: ", y_offset);
    *offset += y_offset;
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
    for (u32 i = 0; i < selected_item_values->paths.size; ++i)
    {
        free(selected_item_values->paths.data[i]);
    }
    hash_table_clear_char_u32(&selected_item_values->selected_items);
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

b8 is_ctrl_and_key_pressed(const Event* event, i32 key)
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

void scroll_bar_add(ScrollBar* scroll_bar, V3 position,
                    const Event* mouse_button, const V2 mouse_position,
                    const f32 dimension_y, const f32 area_y,
                    const f32 total_height, const f32 item_height,
                    f32* scroll_offset, f32* offset,
                    RenderingProperties* render)
{
    const f32 scroll_bar_width = 8.0f;
    position.x -= scroll_bar_width;

    quad(&render->vertices, position, v2f(scroll_bar_width, area_y),
         high_light_color, 0.0f);
    render->index_count++;

    if (area_y < total_height)
    {
        const f32 initial_y = position.y;
        const f32 high = 0.0f;
        const f32 low = area_y - (total_height + item_height);
        const f32 p = (*scroll_offset - low) / (high - low);
        const V2 scroll_bar_dimensions =
            v2f(scroll_bar_width, area_y * (area_y / total_height));

        position.y =
            lerp_f32(dimension_y - scroll_bar_dimensions.height, initial_y, p);

        AABB scroll_bar_aabb = {
            .min = v2_v3(position),
            .size = scroll_bar_dimensions,
        };

        b8 collided = collision_point_in_aabb(mouse_position, &scroll_bar_aabb);

        if (collided || scroll_bar->dragging)
        {
            quad(&render->vertices, position, scroll_bar_dimensions, v4ic(0.8f),
                 0.0f);
            render->index_count++;
        }
        else
        {
            quad_gradiant_t_b(&render->vertices, position,
                              scroll_bar_dimensions, bright_color, v4ic(0.45f),
                              0.0f);
            render->index_count++;
        }

        if (mouse_button->activated && collided)
        {
            scroll_bar->dragging = true;
            scroll_bar->mouse_pointer_offset =
                scroll_bar_aabb.min.y - mouse_position.y;
        }
        if (mouse_button->mouse_button_event.action == 0)
        {
            scroll_bar->dragging = false;
        }

        if (scroll_bar->dragging)
        {
            const f32 end_position_y =
                dimension_y - scroll_bar_dimensions.height;

            f32 new_y = mouse_position.y + scroll_bar->mouse_pointer_offset;

            if (new_y < initial_y)
            {
                scroll_bar->mouse_pointer_offset =
                    scroll_bar_aabb.min.y - mouse_position.y;
                new_y = initial_y;
            }
            if (new_y > end_position_y)
            {
                scroll_bar->mouse_pointer_offset =
                    scroll_bar_aabb.min.y - mouse_position.y;
                new_y = end_position_y;
            }

            const f32 offset_p =
                (new_y - initial_y) / (end_position_y - initial_y);

            f32 lerp = lerp_f32(high, low, offset_p);

            lerp = clampf32_high(lerp, high);
            lerp = clampf32_low(lerp, low);
            *scroll_offset = lerp;
            *offset = lerp;
        }
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

void paste_in_directory(DirectoryPage* current_directory)
{
    CharPtrArray pasted_paths = { 0 };
    array_create(&pasted_paths, 10);
    platform_paste_from_clipboard(&pasted_paths);
    if (pasted_paths.size)
    {
        platform_paste_to_directory(&pasted_paths,
                                    current_directory->directory.parent);
        for (u32 i = 0; i < pasted_paths.size; ++i)
        {
            free(pasted_paths.data[i]);
        }
        reload_directory(current_directory);
    }
    free(pasted_paths.data);
}

b8 is_mouse_button_clicked(const Event* event, i32 button)
{
    return event->activated && event->mouse_button_event.action == 0 &&
           event->mouse_button_event.button == button;
}

u8* application_init(ApplicationContext* application)
{
#if 0
    platform_init("FileTic", 1000, 600, &application->platform);
    platform_opengl_init(application->platform);
    if (!gladLoadGL())
    {
        const char* message = "Could not load glad!";
        log_error_message(message, strlen(message));
        ftic_assert(false);
    }
#else
    application->window = window_init("FileTic", 1250, 800);
#endif
    event_init(application->window);

    application->font = (FontTTF){ 0 };
    const i32 width_atlas = 512;
    const i32 height_atlas = 512;
    const f32 pixel_height = 20;
    u8* font_bitmap = (u8*)calloc(width_atlas * height_atlas, sizeof(u8));
    init_ttf_atlas(width_atlas, height_atlas, pixel_height, 96, 32,
                   "res/fonts/arial.ttf", font_bitmap, &application->font);

    application->key_event = event_subscribe(KEY);
    application->mouse_move = event_subscribe(MOUSE_MOVE);
    application->mouse_button = event_subscribe(MOUSE_BUTTON);
    application->mouse_wheel = event_subscribe(MOUSE_WHEEL);

    application->directory_history = (DirectoryArray){ 0 };
    array_create(&application->directory_history, 10);

    const char* dir = "C:\\*";
    DirectoryPage page = { 0 };
    page.directory = platform_get_directory(dir, (u32)strlen(dir));
    array_push(&application->directory_history, page);

    application->last_time = platform_get_time();
    application->delta_time = 0.0f;
    application->mvp = (MVP){ 0 };
    application->mvp.view = m4d();
    application->mvp.model = m4d();

    return font_bitmap;
}

void search_page_init(SearchPage* search_page)
{
    search_page->search_result_file_array = (SafeFileArray){ 0 };
    safe_array_create(&search_page->search_result_file_array, 10);
    search_page->search_result_folder_array = (SafeFileArray){ 0 };
    safe_array_create(&search_page->search_result_folder_array, 10);

    search_page->search_buffer = (CharArray){ 0 };
    array_create(&search_page->search_buffer, 30);
    search_page->search_bar_hit = false;
    search_page->search_blinking_time = 0.4f;

    search_page->offset = 0.0f;
    search_page->scroll_offset = 0.0f;

    search_page->running_id = 0;
    search_page->last_running_id = 0;
}

void rendering_properties_array_init(const u32 index_buffer_id,
                                     const FontTTF* font, u8* font_bitmap,
                                     RenderingPropertiesArray* array)
{
    const i32 width_atlas = 512;
    const i32 height_atlas = 512;
    const f32 pixel_height = 16;
    TextureProperties texture_properties = {
        .width = width_atlas,
        .height = height_atlas,
        .bytes = font_bitmap,
    };
    u32 font_texture = texture_create(&texture_properties, GL_RGBA8, GL_RED);
    free(texture_properties.bytes);

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

    RenderingPropertiesArray rendering_properties = { 0 };
    array_create(&rendering_properties, 2);

    u32 main_texturess[] = { default_texture_red, font_texture,
                             file_icon_texture,   folder_icon_texture,
                             close_icon_texture,  arrow_icon_texture };

    u32* main_textures =
        (u32*)calloc(static_array_size(main_texturess), sizeof(u32));
    main_textures[0] = default_texture_red;
    main_textures[1] = font_texture;
    main_textures[2] = file_icon_texture;
    main_textures[3] = folder_icon_texture;
    main_textures[4] = close_icon_texture;
    main_textures[5] = arrow_icon_texture;

    array_push(&rendering_properties,
               rendering_properties_init(main_shader, main_textures,
                                         static_array_size(main_texturess),
                                         index_buffer_id, &vertex_buffer_layout,
                                         100 * 4));

    u32* preview_textures = (u32*)calloc(2, sizeof(u32));
    preview_textures[0] = default_texture;
    array_push(&rendering_properties,
               rendering_properties_init(preview_shader, preview_textures, 1,
                                         index_buffer_id, &vertex_buffer_layout,
                                         100 * 4));
    *array = rendering_properties;
}

void application_begin_frame(ApplicationContext* application)
{
    int width, height;
    window_get_size(application->window, &width, &height);
    application->dimensions = v2f((f32)width, (f32)height);
    application->mouse_position =
        v2f(application->mouse_move->mouse_move_event.position_x,
            application->mouse_move->mouse_move_event.position_y);

    glViewport(0, 0, width, height);
    glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    application->mvp.projection =
        ortho(0.0f, application->dimensions.width,
              application->dimensions.height, 0.0f, -1.0f, 1.0f);
}

void application_end_frame(ApplicationContext* application)
{
    window_swap(application->window);
    event_poll();

    f64 now = window_get_time();
    application->delta_time = now - application->last_time;
    const u32 target_milliseconds = 8;
    const u64 curr_milliseconds = (u64)(application->delta_time * 1000.0f);
    if (target_milliseconds > curr_milliseconds)
    {
        const u64 milli_to_sleep =
            (u64)(target_milliseconds - curr_milliseconds);
        platform_sleep(milli_to_sleep);
        now = window_get_time();
        application->delta_time = now - application->last_time;
    }
    application->last_time = now;
}

void search_page_parse_key_buffer(SearchPage* page, const f32 search_bar_width,
                                  const FontTTF* font,
                                  const b8 reload_search_result,
                                  DirectoryArray* directory_history,
                                  ThreadTaskQueue* thread_task_queue)
{
    const CharArray* key_buffer = event_get_key_buffer();
    for (u32 i = 0; i < key_buffer->size; ++i)
    {
        char current_char = key_buffer->data[i];
        if (closed_interval(0, (current_char - 32), 96))
        {
            array_push(&page->search_buffer, current_char);
            f32 x_advance =
                text_x_advance(font->chars, page->search_buffer.data,
                               page->search_buffer.size, 1.0f);
            if (x_advance >= search_bar_width)
            {
                *array_back(&page->search_buffer) = 0;
                --page->search_buffer.size;
            }
        }
    }
    if (key_buffer->size || reload_search_result)
    {
        search_page_clear_search_result(page);

        running_callbacks[page->last_running_id] = false;

        if (page->search_buffer.size)
        {
            page->last_running_id = page->running_id;
            running_callbacks[page->running_id++] = true;
            page->running_id %= 100;

            const char* parent =
                current_directory(directory_history)->directory.parent;
            size_t parent_length = strlen(parent);
            char* dir2 = (char*)calloc(parent_length + 3, sizeof(char));
            memcpy(dir2, parent, parent_length);
            dir2[parent_length++] = '\\';
            dir2[parent_length++] = '*';

            const char* string_to_match = page->search_buffer.data;

            FindingCallbackAttribute* arguments =
                (FindingCallbackAttribute*)calloc(
                    1, sizeof(FindingCallbackAttribute));
            arguments->thread_queue = thread_task_queue;
            arguments->file_array = &page->search_result_file_array;
            arguments->folder_array = &page->search_result_folder_array;
            arguments->start_directory = dir2;
            arguments->start_directory_length = (u32)parent_length;
            arguments->string_to_match = string_to_match;
            arguments->string_to_match_length = page->search_buffer.size;
            arguments->running_id = page->last_running_id;
            finding_callback(arguments);
            page->offset = 0.0f;
        }
    }
}

DirectoryItemListReturnValue
search_page_update(SearchPage* page, const ApplicationContext* application,
                   const b8 check_colission, const V3 search_bar_position,
                   const f32 search_page_header_start_x,
                   DirectoryArray* directory_history,
                   ThreadTaskQueue* thread_task_queue,
                   SelectedItemValues* selected_item_values)
{
    const f32 search_bar_width = 250.0f;
    const f32 search_bar_input_text_padding = 10.0f;
    page->search_bar_aabb = (AABB){
        .min = v2_v3(search_bar_position),
        .size = v2f(search_bar_width, 40.0f),
    };

    const f32 scale = 1.0f;
    const f32 padding_top = 2.0f;
    const f32 quad_height =
        scale * application->font.pixel_height + padding_top * 5.0f;

    V3 search_input_position = search_bar_position;
    search_input_position.x += search_bar_input_text_padding;
    search_input_position.y += scale * application->font.pixel_height + 8.0f;

    V3 search_result_position = search_bar_position;
    search_result_position.y += page->search_bar_aabb.size.y + 20.0f;
    const f32 search_page_header_size_y = search_result_position.y;

    page->search_result_aabb = (AABB){
        .min = v2_v3(search_result_position),
    };
    page->search_result_aabb.size =
        v2_sub(application->dimensions, page->search_result_aabb.min);
    search_result_position.y += page->scroll_offset;

    // Rendered first to go behind search bar when scrolling
    DirectoryItemListReturnValue search_list_return_value = { 0 };
    if (page->search_result_file_array.array.size ||
        page->search_result_folder_array.array.size)
    {
        const f32 search_result_width =
            (application->dimensions.x - search_result_position.x) - 10.0f;

        platform_mutex_lock(&page->search_result_file_array.mutex);
        platform_mutex_lock(&page->search_result_folder_array.mutex);

        search_list_return_value = directory_item_list(
            application, &page->search_result_folder_array.array,
            &page->search_result_file_array.array, check_colission,
            search_result_position, search_result_width, quad_height,
            padding_top, selected_item_values, page->render, directory_history);

        platform_mutex_unlock(&page->search_result_folder_array.mutex);
        platform_mutex_unlock(&page->search_result_file_array.mutex);
    }

    if (application->mouse_button->activated)
    {
        if (check_colission &&
            collision_point_in_aabb(application->mouse_position,
                                    &page->search_bar_aabb))
        {
            page->search_bar_hit = true;
        }
        else
        {
            page->search_bar_hit = false;
            page->search_blinking_time = 0.4f;
        }
    }

    if (page->search_bar_hit)
    {
        b8 reload_search_result = false;
        if (application->key_event->activated &&
            (application->key_event->key_event.action == FTIC_PRESS ||
             application->key_event->key_event.action == FTIC_REPEAT) &&
            application->key_event->key_event.key == FTIC_KEY_BACKSPACE)
        {
            if (page->search_buffer.size)
            {
                *array_back(&page->search_buffer) = 0;
                --page->search_buffer.size;
                reload_search_result = true;
            }
        }
        search_page_parse_key_buffer(
            page, search_bar_width - (search_bar_input_text_padding * 2),
            &application->font, reload_search_result, directory_history,
            thread_task_queue);
    }

    quad(&page->render->vertices, v3f(search_page_header_start_x, 0.0f, 0.0f),
         v2f(application->dimensions.x - search_page_header_start_x,
             search_page_header_size_y),
         clear_color, 0.0f);
    page->render->index_count++;

    // Search bar
    render_input(&application->font, page->search_buffer.data,
                 page->search_buffer.size, scale, search_input_position,
                 search_bar_position.y + 10.0f, page->search_bar_hit,
                 application->delta_time, &page->search_blinking_time,
                 page->render);

    quad_border_gradiant(&page->render->vertices, &page->render->index_count,
                         v3_v2(page->search_bar_aabb.min),
                         page->search_bar_aabb.size, bright_color, border_color,
                         border_width, 0.0f);

    const V3 scroll_bar_position =
        v3f(application->dimensions.width,
            page->search_result_aabb.min.y - 8.0f, 0.0f);
    const f32 area_y = page->search_result_aabb.size.y + 8.0f;
    const f32 total_height = search_list_return_value.count * quad_height;
    scroll_bar_add(&page->scroll_bar, scroll_bar_position,
                   application->mouse_button, application->mouse_position,
                   application->dimensions.y, area_y, total_height, quad_height,
                   &page->scroll_offset, &page->offset, page->render);

    return search_list_return_value;
}

typedef struct MainDropDownSelectionData
{
    DirectoryPage* directory;
    const CharPtrArray* selected_paths;
} MainDropDownSelectionData;

b8 main_drop_down_selection(u32 index, b8 hit, b8 should_close,
                            b8 mouse_button_clicked, V4* text_color, void* data)
{
    MainDropDownSelectionData* arguments = (MainDropDownSelectionData*)data;
    switch (index)
    {
        case COPY_OPTION_INDEX:
        {
            if (arguments->selected_paths->size == 0)
            {
                *text_color = lighter_color;
            }
            else
            {
                if (mouse_button_clicked && hit)
                {
                    platform_copy_to_clipboard(arguments->selected_paths);
                    should_close = true;
                }
            }
            break;
        }
        case PASTE_OPTION_INDEX:
        {
            if (platform_clipboard_is_empty())
            {
                *text_color = lighter_color;
            }
            else
            {
                if (mouse_button_clicked && hit)
                {
                    paste_in_directory(arguments->directory);
                    should_close = true;
                }
            }
            break;
        }
        case DELETE_OPTION_INDEX:
        {
            if (arguments->selected_paths->size == 0)
            {
                *text_color = lighter_color;
            }
            else
            {
                if (mouse_button_clicked && hit)
                {
                    platform_delete_files(arguments->selected_paths);
                    reload_directory(arguments->directory);
                    should_close = true;
                }
            }
            break;
        }
        case PROPERTIES_OPTION_INDEX:
        {
            if (arguments->selected_paths->size == 0)
            {
                *text_color = lighter_color;
            }
            else
            {
                if (mouse_button_clicked && hit)
                {
                    platform_show_properties(
                        10, 10, arguments->selected_paths->data[0]);
                    // reload_directory(arguments->directory);
                    should_close = true;
                }
            }
            break;
        }
        default: break;
    }
    return should_close;
}

b8 drop_down_menu_add(DropDownMenu* drop_down_menu,
                      const ApplicationContext* application, void* option_data)
{
    drop_down_menu->x += (f32)(application->delta_time * 2.0);

    const u32 drop_down_item_count = drop_down_menu->options.size;
    const f32 drop_down_item_height = 40.0f;
    const f32 end_height = drop_down_item_count * drop_down_item_height;
    const f32 current_y = ease_out_elastic(drop_down_menu->x) * end_height;
    const f32 precent = current_y / end_height;
    const f32 drop_down_width = 200.0f;
    const f32 drop_down_border_width = 1.0f;
    const f32 border_extra_padding = drop_down_border_width * 2.0f;
    const f32 drop_down_outer_padding = 10.0f + border_extra_padding;
    const V2 end_position = v2f(
        drop_down_menu->position.x + drop_down_width + drop_down_outer_padding,
        drop_down_menu->position.y + end_height + drop_down_outer_padding);

    f32 diff = end_position.y - application->dimensions.y;
    if (diff > 0)
    {
        drop_down_menu->position.y -= diff;
    }
    diff = end_position.x - application->dimensions.x;
    if (diff > 0)
    {
        drop_down_menu->position.x -= diff;
    }

    drop_down_menu->aabb = quad_border_gradiant(
        &drop_down_menu->render->vertices, &drop_down_menu->render->index_count,
        v3f(drop_down_menu->position.x - drop_down_border_width,
            drop_down_menu->position.y - drop_down_border_width, 0.0f),
        v2f(drop_down_width + border_extra_padding,
            current_y + border_extra_padding),
        v4i(1.0f), lighter_color, drop_down_border_width, 0.0f);

    b8 mouse_button_clicked =
        is_mouse_button_clicked(application->mouse_button, FTIC_MOUSE_BUTTON_1);

    b8 any_drop_down_item_hit = false;
    b8 should_close = false;
    const f32 promt_item_text_padding = 5.0f;
    V3 promt_item_position = drop_down_menu->position;
    for (u32 i = 0; i < drop_down_item_count; ++i)
    {
        AABB drop_down_item_aabb = {
            .min = v2_v3(promt_item_position),
            .size = v2f(drop_down_width, drop_down_item_height * precent),
        };

        b8 drop_down_item_hit = false;
        if (collision_point_in_aabb(application->mouse_position,
                                    &drop_down_item_aabb))
        {
            drop_down_item_hit = true;
            any_drop_down_item_hit = true;
        }
        const V4 drop_down_color =
            drop_down_item_hit ? border_color : high_light_color;

        quad(&drop_down_menu->render->vertices, promt_item_position,
             drop_down_item_aabb.size, drop_down_color, 0.0f);
        drop_down_menu->render->index_count++;

        V3 promt_item_text_position = promt_item_position;
        promt_item_text_position.y +=
            application->font.pixel_height + promt_item_text_padding + 3.0f;
        promt_item_text_position.x += promt_item_text_padding;

        V4 text_color = v4i(1.0f);
        should_close = drop_down_menu->menu_options_selection(
            i, drop_down_item_hit, should_close, mouse_button_clicked,
            &text_color, option_data);

        drop_down_menu->render->index_count += text_generation_color(
            application->font.chars, drop_down_menu->options.data[i], 1.0f,
            promt_item_text_position, 1.0f,
            application->font.pixel_height * precent, text_color, NULL, NULL,
            &drop_down_menu->render->vertices);

        promt_item_position.y += drop_down_item_aabb.size.y;
    }
    return should_close || ((!any_drop_down_item_hit) & mouse_button_clicked);
}

void open_preview(V2 image_dimensions, const ApplicationContext* application,
                  RenderingProperties* render)
{
    const V2 total_preview_dimensions =
        v2_s_sub(application->dimensions, 40.0f);
    const V2 ratios = v2_div(total_preview_dimensions, image_dimensions);
    f32 scale_factor =
        ratios.width < ratios.height ? ratios.width : ratios.height;
    V2 preview_dimensions = { 0 };
    if (scale_factor < 1.0f)
    {
        v2_s_multi_equal(&image_dimensions, scale_factor);
    }
    preview_dimensions = image_dimensions;
    v2_s_add_equal(&preview_dimensions, border_width * 2.0f);

    V3 preview_position = v3_v2(v2_s_multi(application->dimensions, 0.5f));
    v3_sub_equal(&preview_position,
                 v3_v2(v2_s_multi(preview_dimensions, 0.5f)));

    AABB* scissor = &render->scissor;
    scissor->min = v2_v3(preview_position);
    scissor->min.y = application->dimensions.y - scissor->min.y;
    scissor->min.y -= preview_dimensions.y;
    scissor->size = preview_dimensions;
    scissor->size.width += border_width;

    AABB preview_aabb =
        quad_border(&render->vertices, &render->index_count, preview_position,
                    preview_dimensions, lighter_color, border_width, 0.0f);
    v3_add_equal(&preview_position, v3_v2(v2i(border_width)));
    v2_s_sub_equal(&preview_dimensions, border_width * 2.0f);

    quad(&render->vertices, preview_position, preview_dimensions, clear_color,
         0.0f);
    render->index_count++;

    quad(&render->vertices, preview_position, preview_dimensions, v4i(1.0f),
         1.0f);
    render->index_count++;

    const MouseButtonEvent* event =
        &application->mouse_button->mouse_button_event;
    if (!collision_point_in_aabb(application->mouse_position, &preview_aabb) &&
        application->mouse_button->activated && event->action == 0 &&
        event->button == FTIC_MOUSE_BUTTON_1)
    {
        if (render->texture_count > 1)
        {
            texture_delete(render->textures[--render->texture_count]);
        }
        rendering_properties_clear(render);
    }
}

char* file_to_listen_to;

void listen_to_directory_change(void* data)
{
}

int main(int argc, char** argv)
{
    ApplicationContext application = { 0 };
    u8* font_bitmap = application_init(&application);

    ThreadQueue thread_queue = { 0 };
    thread_init(100000, 8, &thread_queue);

    SearchPage search_page = { 0 };
    search_page_init(&search_page);

    IndexArray index_array = { 0 };
    array_create(&index_array, 100 * 6);
    generate_indicies(&index_array, 0, index_array.capacity * 6);
    u32 index_buffer_id = index_buffer_create(
        index_array.data, index_array.size, sizeof(u32), GL_STATIC_DRAW);
    free(index_array.data);

    RenderingPropertiesArray rendering_properties = { 0 };
    rendering_properties_array_init(index_buffer_id, &application.font,
                                    font_bitmap, &rendering_properties);
    RenderingProperties* main_render = rendering_properties.data;
    RenderingProperties* preview_render = rendering_properties.data + 1;

    search_page.render = main_render;

    // TODO(Linus): Make a set for this instead
    SelectedItemValues selected_item_values = { 0 };
    array_create(&selected_item_values.paths, 10);
    selected_item_values.selected_items =
        hash_table_create_char_u32(100, hash_murmur);

    CharPtrArray pasted_paths = { 0 };
    array_create(&pasted_paths, 10);

    b8 right_clicked = false;

    DropDownMenu drop_down_menu = {
        .position = v3d(),
        .menu_options_selection = main_drop_down_selection,
        .render = main_render,
    };
    char* options[] = {
        [COPY_OPTION_INDEX] = "Copy",
        [PASTE_OPTION_INDEX] = "Paste",
        [DELETE_OPTION_INDEX] = "Delete",
        [PROPERTIES_OPTION_INDEX] = "Properties",
    };
    array_create(&drop_down_menu.options, static_array_size(options));
    array_push(&drop_down_menu.options, options[COPY_OPTION_INDEX]);
    array_push(&drop_down_menu.options, options[PASTE_OPTION_INDEX]);
    array_push(&drop_down_menu.options, options[DELETE_OPTION_INDEX]);
    array_push(&drop_down_menu.options, options[PROPERTIES_OPTION_INDEX]);

    V2 image_dimensions = v2d();

    ScrollBar main_scroll_bar = { 0 };
    b8 search_scroll_bar_dragging = false;

    DirectoryItemArray quick_access_folders = { 0 };
    array_create(&quick_access_folders, 10);

    enable_gldebugging();
    glEnable(GL_BLEND);
    glEnable(GL_SCISSOR_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    while (!window_should_close(application.window))
    {
        application_begin_frame(&application);

        rendering_properties_array_clear(&rendering_properties);
        main_render->scissor.size = application.dimensions;

        b8 check_colission = preview_render->texture_count == 1;
        if (check_colission &&
            collision_point_in_aabb(application.mouse_position,
                                    &drop_down_menu.aabb))
        {
            check_colission = false;
        }

        if (selected_item_values.paths.size)
        {
            const MouseButtonEvent* event =
                &application.mouse_button->mouse_button_event;

            if (is_ctrl_and_key_pressed(application.key_event, FTIC_KEY_C))
            {
                platform_copy_to_clipboard(&selected_item_values.paths);
            }
            else if (selected_item_values.paths.size == 1 &&
                     is_ctrl_and_key_pressed(application.key_event, FTIC_KEY_D))
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
                if (extension &&
                    (!strcmp(extension, ".png") || !strcmp(extension, ".jpg")))
                {
                    TextureProperties texture_properties = { 0 };
                    texture_load(path, &texture_properties);
                    image_dimensions = v2f((f32)texture_properties.width,
                                           (f32)texture_properties.height);

                    ftic_assert(texture_properties.bytes);
                    u32 texture =
                        texture_create(&texture_properties, GL_RGBA8, GL_RGBA);
                    preview_render->textures[preview_render->texture_count++] =
                        texture;
                    free(texture_properties.bytes);
                    reset_selected_items(&selected_item_values);
                }
            }
            else if (application.key_event->activated &&
                     application.key_event->key_event.action == 1 &&
                     application.key_event->key_event.key == FTIC_KEY_ESCAPE)
            {
                platform_delete_files(&selected_item_values.paths);
                reload_directory(
                    current_directory(&application.directory_history));
            }
            else if (check_colission &&
                     !application.key_event->key_event.ctrl_pressed &&
                     application.mouse_button->activated &&
                     event->action == 0 && event->button == FTIC_MOUSE_BUTTON_1)
            {
                reset_selected_items(&selected_item_values);
            }
        }
        if (is_ctrl_and_key_pressed(application.key_event, FTIC_KEY_V))
        {
            paste_in_directory(
                current_directory(&application.directory_history));
        }

        V3 starting_position = v3f(150.0f, 0.0f, 0.0f);
        AABB rect =
            quad_gradiant_t_b(&main_render->vertices, starting_position,
                              v2f(border_width, application.dimensions.y),
                              bright_color, border_color, 0.0f);
        ++main_render->index_count;

        V3 search_bar_position =
            v3f(application.dimensions.x * 0.6f, 10.0f, 0.0f);

        V3 parent_directory_path_position =
            v3f(rect.min.x + border_width, 30.0f, 0.0f);
        V3 text_starting_position = parent_directory_path_position;

        text_starting_position.x += 10.0f;
        text_starting_position.y += application.font.pixel_height;

        const f32 width =
            ((search_bar_position.x - 10.0f) - text_starting_position.x);

        AABB directory_aabb = {
            .min = v2_v3(text_starting_position),
            .size =
                v2f(width, application.dimensions.y - text_starting_position.y),
        };
        text_starting_position.y +=
            current_directory(&application.directory_history)->scroll_offset;

        const f32 scale = 1.0f;
        const f32 padding_top = 2.0f;
        const f32 quad_height =
            scale * application.font.pixel_height + padding_top * 5.0f;

        DirectoryItemListReturnValue main_list_return_value =
            directory_item_list(
                &application,
                &current_directory(&application.directory_history)
                     ->directory.sub_directories,
                &current_directory(&application.directory_history)
                     ->directory.files,
                check_colission, text_starting_position, width - 10.0f,
                quad_height, padding_top, &selected_item_values, main_render,
                &application.directory_history);

        const f32 back_drop_height =
            parent_directory_path_position.y + application.font.pixel_height;
        quad(&main_render->vertices,
             v3f(parent_directory_path_position.x, 0.0f, 0.0f),
             v2f(width + 10.0f, back_drop_height), high_light_color, 0.0f);
        main_render->index_count++;

        parent_directory_path_position.x += 10.0f;

        main_render->index_count += text_generation(
            application.font.chars,
            current_directory(&application.directory_history)->directory.parent,
            1.0f, parent_directory_path_position, scale,
            application.font.pixel_height, NULL, NULL, &main_render->vertices);

        AABB right_border_aabb = quad_gradiant_t_b(
            &main_render->vertices,
            v3f(directory_aabb.min.x + directory_aabb.size.x, 0.0f, 0.0f),
            v2f(border_width, application.dimensions.y), bright_color,
            border_color, 0.0f);
        ++main_render->index_count;

        AABB side_under_border_aabb = {
            .min = v2f(right_border_aabb.min.x,
                       search_page.search_bar_aabb.min.y +
                           search_page.search_bar_aabb.size.y + 10.0f),
            .size = v2f(application.dimensions.x - right_border_aabb.min.x,
                        border_width),
        };

        V4 side_under_border_start_color =
            v4_lerp(bright_color, border_color,
                    side_under_border_aabb.min.y / right_border_aabb.size.y);

        // Search bar

        DirectoryItemListReturnValue search_list_return_value =
            search_page_update(&search_page, &application, check_colission,
                               search_bar_position,
                               side_under_border_aabb.min.x + border_width,
                               &application.directory_history,
                               &thread_queue.task_queue, &selected_item_values);

        quad_gradiant_l_r(&main_render->vertices,
                          v3_v2(side_under_border_aabb.min),
                          side_under_border_aabb.size,
                          side_under_border_start_color, border_color, 0.0f);
        ++main_render->index_count;

        V3 back_button_position = v3f(10.0f, 10.0f, 0.0f);

        AABB button_aabb = {
            .min = v2_v3(back_button_position),
            .size = v2i(search_page.search_bar_aabb.size.y),
        };

        V4 button_color = clear_color;

        if (check_colission && application.directory_history.size > 1 &&
            collision_point_in_aabb(application.mouse_position, &button_aabb))
        {
            const MouseButtonEvent* event =
                &application.mouse_button->mouse_button_event;
            if (application.mouse_button->activated && event->action == 0 &&
                event->button == FTIC_MOUSE_BUTTON_1)
            {
                platform_reset_directory(
                    &current_directory(&application.directory_history)
                         ->directory);
                application.directory_history.size -= 1;
                DirectoryPage* current =
                    current_directory(&application.directory_history);
                current->scroll_offset = 0.0f;
                reset_selected_items(&selected_item_values);
                reload_directory(current);
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
            application.directory_history.size == 1 ? border_color : v4ic(1.0f);

        quad(&main_render->vertices, back_button_position, v2i(20.0f),
             back_icon_color, 5.0f);
        main_render->index_count++;

        V3 back_button_border_position =
            v3f(0.0f, side_under_border_aabb.min.y, 0.0f);

        quad_gradiant_l_r(&main_render->vertices, back_button_border_position,
                          v2f(rect.min.x, border_width), bright_color,
                          side_under_border_start_color, 0.0f);
        ++main_render->index_count;

        V3 scroll_bar_position =
            v3f(right_border_aabb.min.x, directory_aabb.min.y, 0.0f);
        f32 area_y = directory_aabb.size.y;
        f32 total_height = main_list_return_value.count * quad_height;
        scroll_bar_add(
            &main_scroll_bar, scroll_bar_position, application.mouse_button,
            application.mouse_position, application.dimensions.y, area_y,
            total_height, quad_height,
            &current_directory(&application.directory_history)->scroll_offset,
            &current_directory(&application.directory_history)->offset,
            main_render);

        if (is_mouse_button_clicked(application.mouse_button,
                                    FTIC_MOUSE_BUTTON_2))
        {
            right_clicked = true;
            drop_down_menu.position = v3_v2(application.mouse_position);
            drop_down_menu.position.x += 18.0f;
            drop_down_menu.x = 0.0f;
        }
        if (right_clicked)
        {
            MainDropDownSelectionData data = {
                .directory = current_directory(&application.directory_history),
                .selected_paths = &selected_item_values.paths,
            };
            right_clicked =
                !drop_down_menu_add(&drop_down_menu, &application, &data);
        }
        else
        {
            drop_down_menu.aabb = (AABB){ 0 };
        }

        if (main_list_return_value.hit || search_list_return_value.hit)
        {
            // platform_change_cursor(application.platform, FTIC_HAND_CURSOR);
        }
        else
        {
            // platform_change_cursor(application.platform, FTIC_NORMAL_CURSOR);
        }

        if (preview_render->texture_count > 1)
        {
            open_preview(image_dimensions, &application, preview_render);
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

        DirectoryPage* current =
            current_directory(&application.directory_history);
        if (!search_page.scroll_bar.dragging && !main_scroll_bar.dragging)
        {
            if (application.mouse_wheel->activated)
            {
                if (collision_point_in_aabb(application.mouse_position,
                                            &search_page.search_result_aabb))
                {
                    set_scroll_offset(
                        search_list_return_value.count, quad_height,
                        search_page.search_result_aabb.size.y,
                        application.mouse_wheel, &search_page.offset);
                }
                else if (collision_point_in_aabb(application.mouse_position,
                                                 &directory_aabb))
                {
                    set_scroll_offset(main_list_return_value.count, quad_height,
                                      directory_aabb.size.y,
                                      application.mouse_wheel,
                                      &current->offset);
                }
            }
            current->scroll_offset =
                smooth_scroll(application.delta_time, current->offset,
                              current->scroll_offset);
            search_page.scroll_offset =
                smooth_scroll(application.delta_time, search_page.offset,
                              search_page.scroll_offset);
        }
        for (u32 i = 0; i < rendering_properties.size; ++i)
        {
            rendering_properties_draw(&rendering_properties.data[i],
                                      &application.mvp);
        }
        application_end_frame(&application);
    }
    memset(running_callbacks, 0, sizeof(running_callbacks));
    for (u32 i = 0; i < rendering_properties.size; ++i)
    {
        RenderingProperties* rp = rendering_properties.data + i;
        buffer_delete(rp->vertex_buffer_id);
        buffer_delete(rp->index_buffer_id);
        for (u32 j = 0; j < rp->texture_count; ++j)
        {
            texture_delete(rp->textures[i]);
        }
        free(rp->textures);
    }
    // TODO: Cleanup of all
    threads_destroy(&thread_queue);
}
