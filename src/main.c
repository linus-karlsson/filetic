#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
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
#include "util.h"
#include "ui.h"
#include "application.h"

#define COPY_OPTION_INDEX 0
#define PASTE_OPTION_INDEX 1
#define DELETE_OPTION_INDEX 2
#define ADD_TO_QUICK_OPTION_INDEX 3
#define PROPERTIES_OPTION_INDEX 4

#define icon_1_co(width, height)                                               \
    {                                                                          \
        .x = 0.0f / width,                                                     \
        .y = 0.0f / height,                                                    \
        .z = 128.0f / width,                                                   \
        .w = 128.0f / height,                                                  \
    };

global const V4 file_icon_co = icon_1_co(512.0f, 128.0f);
global const V4 arrow_back_icon_co = icon_1_co(384.0f, 128.0f);

#define icon_2_co(width, height)                                               \
    {                                                                          \
        .x = 128.0f / width,                                                   \
        .y = 0.0f / height,                                                    \
        .z = 256.0f / width,                                                   \
        .w = 128.0f / height,                                                  \
    };
global const V4 folder_icon_co = icon_2_co(512.0f, 128.0f);
global const V4 arrow_up_icon_co = icon_2_co(384.0f, 128.0f);

#define icon_3_co(width, height)                                               \
    {                                                                          \
        .x = 256.0f / width,                                                   \
        .y = 0.0f / height,                                                    \
        .z = 384.0f / width,                                                   \
        .w = 128.0f / height,                                                  \
    };
global const V4 pdf_icon_co = icon_3_co(512.0f, 128.0f);
global const V4 arrow_right_icon_co = icon_3_co(384.0f, 128.0f);

#define icon_4_co(width, height)                                               \
    {                                                                          \
        .x = 384.0f / width,                                                   \
        .y = 0.0f / height,                                                    \
        .z = 512.0f / width,                                                   \
        .w = 128.0f / height,                                                  \
    };
global const V4 png_icon_co = icon_4_co(512.0f, 128.0f);

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

typedef struct DirectoryItemList
{
    DirectoryItemArray* items;
    f64 pulse_x;
} DirectoryItemList;

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

typedef struct SearchPage
{
    InputBuffer input;
    SafeFileArray search_result_file_array;
    SafeFileArray search_result_folder_array;

    u32 running_id;
    u32 last_running_id;
} SearchPage;

typedef enum DebugLogLevel
{
    NONE,
    HIGH,
    MEDIUM,
    LOW,
    NOTIFICATION,
} DebugLogLevel;

global DebugLogLevel g_debug_log_level = HIGH;

typedef enum ItemType
{
    TYPE_UNKNOWN,
    TYPE_FOLDER,
    TYPE_FILE,
} ItemType;

typedef struct DirectoryItemListReturnValue
{
    ItemType type;
    u32 count;
    f32 total_height;
    i32 hit_index;
    b8 hit;
} DirectoryItemListReturnValue;

typedef struct MainDropDownSelectionData
{
    DirectoryPage* directory;
    DirectoryItemArray* quick_access;
    const CharPtrArray* selected_paths;
} MainDropDownSelectionData;

typedef struct SuggestionSelectionData
{
    CharArray* parent_directory;
    DirectoryItemArray items;
    i32* cursor_index;
    i32* tab_index;
    b8 change_directory;
} SuggestionSelectionData;

typedef struct DropDownMenu2
{
    f32 x;
    i32 tab_index;
    V2 position;
    AABB aabb;
    CharPtrArray options;
    RenderingProperties* render;
    u32* index_count;
    b8 (*menu_options_selection)(u32 index, b8 hit, b8 should_close,
                                 b8 item_clicked, V4* text_color, void* data);
} DropDownMenu2;

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

u32 get_path_length(const char* path, u32 path_length)
{
    for (i32 i = path_length - 1; i >= 0; --i)
    {
        const char current_char = path[i];
        if (current_char == '\\' || current_char == '/')
        {
            return i + 1;
        }
    }
    return 0;
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

void safe_add_directory_item(const DirectoryItem* item,
                             FindingCallbackAttribute* arguments,
                             SafeFileArray* safe_array)
{
    const char* name = item->name;
    char* path = item->path;
    if (string_contains_case_insensitive(name, arguments->string_to_match))
    {
        const u32 name_length = (u32)strlen(name);
        const u32 path_length = (u32)strlen(path);
        DirectoryItem copy = {
            .size = item->size,
            .path = string_copy(path, path_length, 2),
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
            string_copy(path, (u32)directory_name_length, 2);
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

void display_text_and_truncate_if_necissary(const FontTTF* font,
                                            const V2 position,
                                            const f32 total_width, char* text,
                                            u32* index_count,
                                            RenderingProperties* render)
{
    const u32 text_len = (u32)strlen(text);
    const i32 i = text_check_length_within_boundary(font->chars, text, text_len,
                                                    1.0f, total_width);
    const b8 too_long = i >= 3;
    char saved_name[4] = "...";
    if (too_long)
    {
        i32 j = i - 3;
        string_swap(text + j, saved_name); // Truncate
    }
    *index_count += text_generation(font->chars, text, 1.0f, position, 1.0f,
                                    font->pixel_height, NULL, NULL, NULL,
                                    &render->vertices);
    if (too_long)
    {
        memcpy(text + (i - 3), saved_name, sizeof(saved_name));
    }
}

b8 go_to_directory(char* path, u32 length, DirectoryHistory* directory_history)
{
    b8 result = false;
    char saved_chars[3];
    saved_chars[0] = path[length];
    path[length] = '\0';
    if (string_compare_case_insensitive(
            path, current_directory(directory_history)->directory.parent) &&
        platform_directory_exists(path))
    {
        path[length++] = '\\';
        saved_chars[1] = path[length];
        path[length++] = '*';
        saved_chars[2] = path[length];
        path[length] = '\0';
        DirectoryPage new_page = { 0 };
        new_page.directory = platform_get_directory(path, length);
        for (i32 i = directory_history->history.size - 1;
             i >= (i32)directory_history->current_index + 1; --i)
        {
            platform_reset_directory(
                &directory_history->history.data[i].directory);
        }
        directory_history->history.size = ++directory_history->current_index;
        array_push(&directory_history->history, new_page); // size + 1
        path[length--] = saved_chars[2];
        path[length--] = saved_chars[1];
        result = true;
    }
    path[length] = saved_chars[0];
    return result;
}

void open_folder(char* folder_path, DirectoryHistory* directory_history)
{
    if (strcmp(folder_path,
               current_directory(directory_history)->directory.parent) == 0)
    {
        return;
    }
    u32 length = (u32)strlen(folder_path);
    go_to_directory(folder_path, length, directory_history);
}

void display_full_path(const FontTTF* font, const V2 position, const V2 size,
                       const f32 padding, u32* index_count, char* path,
                       RenderingProperties* render)
{
    quad(&render->vertices, position, size, high_light_color, 0.0f);
    *index_count += 6;
    V2 text_position = v2_s_add(position, padding);
    text_position.y += font->pixel_height;
    display_text_and_truncate_if_necissary(font, text_position, size.width,
                                           path, index_count, render);
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

u64 u64_hash_function(const void* data, u32 len, u64 seed)
{
    return *(u64*)data;
}

void merge(DirectoryItem* array, u32 left, u32 mid, u32 right)
{
    u32 n1 = mid - left + 1;
    u32 n2 = right - mid;

    DirectoryItem* left_array =
        (DirectoryItem*)malloc(n1 * sizeof(DirectoryItem));
    DirectoryItem* right_array =
        (DirectoryItem*)malloc(n2 * sizeof(DirectoryItem));

    for (u32 i = 0; i < n1; ++i)
    {
        left_array[i] = array[left + i];
    }
    for (u32 i = 0; i < n2; ++i)
    {
        right_array[i] = array[mid + 1 + i];
    }

    u32 i = 0;
    u32 j = 0;
    u32 k = left;
    while (i < n1 && j < n2)
    {
        if (string_compare_case_insensitive(left_array[i].name,
                                            right_array[j].name) <= 0)
        {
            array[k] = left_array[i++];
        }
        else
        {
            array[k] = right_array[j++];
        }
        ++k;
    }

    while (i < n1)
    {
        array[k++] = left_array[i++];
    }

    while (j < n2)
    {
        array[k++] = right_array[j++];
    }

    free(left_array);
    free(right_array);
}

void mergeSort(DirectoryItem* array, u32 left, u32 right)
{
    if (left < right)
    {
        u32 mid = left + (right - left) / 2;
        mergeSort(array, left, mid);
        mergeSort(array, mid + 1, right);
        merge(array, left, mid, right);
    }
}

void directory_sort_by_name(DirectoryItemArray* array)
{
    mergeSort(array->data, 0, array->size - 1);
}

void directory_flip_array(DirectoryItemArray* array)
{
    const i32 middle = array->size / 2;
    for (i32 i = 0, j = array->size - 1; i < middle; ++i, --j)
    {
        DirectoryItem temp = array->data[i];
        array->data[i] = array->data[j];
        array->data[j] = temp;
    }
}

void directory_sort_by_size(DirectoryItemArray* array)
{
    if (array->size <= 1) return;

    DirectoryItem* output =
        (DirectoryItem*)calloc(array->size, sizeof(DirectoryItem));
    u32 count[256] = { 0 };

    for (u32 shift = 0, s = 0; shift < 8; ++shift, s += 8)
    {
        memset(count, 0, sizeof(count));

        for (u32 i = 0; i < array->size; ++i)
        {
            count[(array->data[i].size >> s) & 0xff]++;
        }

        for (u32 i = 1; i < 256; ++i)
        {
            count[i] += count[i - 1];
        }

        for (i32 i = array->size - 1; i >= 0; --i)
        {
            u32 index = (array->data[i].size >> s) & 0xff;
            output[--count[index]] = array->data[i];
        }
        DirectoryItem* tmp = array->data;
        array->data = output;
        output = tmp;
    }
    free(output);
}

void sort_directory(DirectoryPage* directory_page)
{
    switch (directory_page->sort_by)
    {
        case SORT_NAME:
        {
            directory_sort_by_name(&directory_page->directory.sub_directories);
            directory_sort_by_name(&directory_page->directory.files);
            if (directory_page->sort_count == 2)
            {
                directory_flip_array(
                    &directory_page->directory.sub_directories);
                directory_flip_array(&directory_page->directory.files);
            }
            break;
        }
        case SORT_SIZE:
        {
            directory_sort_by_size(&directory_page->directory.files);
            if (directory_page->sort_count == 2)
            {
                directory_flip_array(&directory_page->directory.files);
            }
            break;
        }
        default: break;
    }
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
    sort_directory(directory_page);
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

b8 is_mouse_button_clicked(i32 button)
{
    const MouseButtonEvent* event = event_get_mouse_button_event();
    return event->activated && event->action == FTIC_RELEASE &&
           event->button == button;
}

void search_page_initialize(SearchPage* search_page)
{
    safe_array_create(&search_page->search_result_file_array, 10);
    safe_array_create(&search_page->search_result_folder_array, 10);
    search_page->input = ui_input_buffer_create();
    search_page->running_id = 0;
    search_page->last_running_id = 0;
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

b8 search_page_has_result(const SearchPage* search_page)
{
    return search_page->search_result_file_array.array.size > 0 ||
           search_page->search_result_folder_array.array.size > 0;
}

void rendering_properties_array_initialize(const FontTTF* font, u8* font_bitmap,
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
    u32 font_texture = texture_create(&texture_properties, GL_RGBA8, GL_RGBA);
    free(texture_properties.bytes);

    VertexBufferLayout vertex_buffer_layout = default_vertex_buffer_layout();

    u32 default_texture = create_default_texture();

    u32 file_icon_texture = load_icon("res/icons/icon_sheet.png");
    u32 arrow_icon_texture =
        load_icon_as_white("res/icons/arrow_sprite_sheet.png");

    u32 main_shader = shader_create("./res/shaders/vertex.glsl",
                                    "./res/shaders/fragment.glsl");

    u32 preview_shader = shader_create("./res/shaders/vertex.glsl",
                                       "./res/shaders/fragment.glsl");
    ftic_assert(main_shader);
    ftic_assert(preview_shader);

    RenderingPropertiesArray rendering_properties = { 0 };
    array_create(&rendering_properties, 2);

    const u32 main_texture_count = 4;
    U32Array main_textures = { 0 };
    array_create(&main_textures, main_texture_count);
    array_push(&main_textures, default_texture);
    array_push(&main_textures, font_texture);
    array_push(&main_textures, file_icon_texture);
    array_push(&main_textures, arrow_icon_texture);

    array_push(&rendering_properties,
               rendering_properties_initialize(main_shader, main_textures,
                                               &vertex_buffer_layout, 100 * 4,
                                               100 * 6));

    *array = rendering_properties;
}

void search_page_search(SearchPage* page, DirectoryHistory* directory_history,
                        ThreadTaskQueue* thread_task_queue)
{
    search_page_clear_search_result(page);

    running_callbacks[page->last_running_id] = false;

    if (page->input.buffer.size)
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

        const char* string_to_match = page->input.buffer.data;

        FindingCallbackAttribute* arguments = (FindingCallbackAttribute*)calloc(
            1, sizeof(FindingCallbackAttribute));
        arguments->thread_queue = thread_task_queue;
        arguments->file_array = &page->search_result_file_array;
        arguments->folder_array = &page->search_result_folder_array;
        arguments->start_directory = dir2;
        arguments->start_directory_length = (u32)parent_length;
        arguments->string_to_match = string_to_match;
        arguments->string_to_match_length = page->input.buffer.size;
        arguments->running_id = page->last_running_id;
        finding_callback(arguments);
    }
}

b8 main_drop_down_selection(u32 index, b8 hit, b8 should_close, b8 item_clicked,
                            V4* text_color, void* data)
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
                if (item_clicked && hit)
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
                if (item_clicked && hit)
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
                if (item_clicked && hit)
                {
                    platform_delete_files(arguments->selected_paths);
                    reload_directory(arguments->directory);
                    should_close = true;
                }
            }
            break;
        }
        case ADD_TO_QUICK_OPTION_INDEX:
        {
            if (arguments->selected_paths->size == 0)
            {
                *text_color = lighter_color;
            }
            else
            {
                if (item_clicked && hit)
                {
                    for (u32 i = 0; i < arguments->selected_paths->size; ++i)
                    {
                        char* path_temp = arguments->selected_paths->data[i];
                        if (!platform_directory_exists(path_temp))
                        {
                            continue;
                        }
                        b8 exist = false;
                        for (u32 j = 0; j < arguments->quick_access->size; ++j)
                        {
                            if (!strcmp(path_temp,
                                        arguments->quick_access->data[j].path))
                            {
                                exist = true;
                                break;
                            }
                        }
                        if (!exist)
                        {
                            const u32 length = (u32)strlen(path_temp);
                            char* path =
                                (char*)calloc(length + 1, sizeof(char));
                            memcpy(path, path_temp, length);
                            DirectoryItem item = {
                                .path = path,
                                .name = path + get_path_length(path, length),
                            };
                            array_push(arguments->quick_access, item);
                        }
                    }
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
                if (item_clicked && hit)
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

b8 load_preview_image(const char* path, V2* image_dimensions,
                      U32Array* textures)
{
    b8 result = false;
    const char* extension = file_get_extension(path, (u32)strlen(path));
    if (extension && (!strcmp(extension, ".png") || !strcmp(extension, ".jpg")))
    {
        if (textures->size > 0)
        {
            texture_delete(textures->data[--textures->size]);
        }
        TextureProperties texture_properties = { 0 };
        texture_load(path, &texture_properties);
        *image_dimensions =
            v2f((f32)texture_properties.width, (f32)texture_properties.height);

        ftic_assert(texture_properties.bytes);
        u32 texture = texture_create(&texture_properties, GL_RGBA8, GL_RGBA);
        array_push(textures, texture);
        free(texture_properties.bytes);
        result = true;
    }
    return result;
}

b8 check_and_load_image(const i32 index, V2* image_dimensions,
                        char** current_path, const DirectoryItemArray* files,
                        U32Array* textures)
{
    const char* path = files->data[index].path;
    if (load_preview_image(path, image_dimensions, textures))
    {
        free(*current_path);
        *current_path = string_copy(path, (u32)strlen(path), 0);
        return true;
    }
    return false;
}

V2 load_and_scale_preview_image(const ApplicationContext* application,
                                V2* image_dimensions, char** current_path,
                                const DirectoryItemArray* files,
                                U32Array* textures)
{
    b8 right_key = is_key_pressed_repeat(FTIC_KEY_RIGHT);
    b8 left_key = is_key_pressed_repeat(FTIC_KEY_LEFT);
    if (right_key || left_key)
    {
        for (i32 i = 0; i < (i32)files->size; ++i)
        {
            if (!string_compare_case_insensitive(*current_path,
                                                 files->data[i].path))
            {
                if (right_key)
                {
                    for (i32 count = 1; count <= (i32)files->size; ++count)
                    {
                        const i32 index = (i + count) % files->size;
                        if (check_and_load_image(index, image_dimensions,
                                                 current_path, files, textures))
                        {
                            break;
                        }
                    }
                }
                else
                {
                    for (i32 count = 1, index = i - count;
                         count <= (i32)files->size; ++count, --index)
                    {
                        if (index < 0) index = files->size - 1;
                        if (check_and_load_image(index, image_dimensions,
                                                 current_path, files, textures))
                        {
                            break;
                        }
                    }
                }
                break;
            }
        }
    }
    V2 image_dimensions_internal = *image_dimensions;

    const V2 total_preview_dimensions =
        v2_s_sub(application->dimensions, 40.0f);
    const V2 ratios =
        v2_div(total_preview_dimensions, image_dimensions_internal);
    f32 scale_factor =
        ratios.width < ratios.height ? ratios.width : ratios.height;
    if (scale_factor < 1.0f)
    {
        v2_s_multi_equal(&image_dimensions_internal, scale_factor);
    }
    return image_dimensions_internal;
}

void move_in_history(const i32 index_add,
                     SelectedItemValues* selected_item_values,
                     DirectoryHistory* directory_history)
{
    directory_history->current_index += index_add;
    DirectoryPage* current = current_directory(directory_history);
    current->scroll_offset = 0.0f;
    reset_selected_items(selected_item_values);
    reload_directory(current);
}

b8 can_go_up_one_directory(char* parent)
{
    for (; *parent; ++parent)
    {
        const char current_char = *parent;
        if (current_char == '\\' || current_char == '/')
        {
            return true;
        }
    }
    return false;
}

void go_up_one_directory(DirectoryHistory* directory_history)
{
    DirectoryPage* current = current_directory(directory_history);
    char* parent = current->directory.parent;
    u32 parent_length = get_path_length(parent, (u32)strlen(parent)) - 1;

    char* new_parent = (char*)calloc(parent_length + 1, sizeof(char));
    memcpy(new_parent, parent, parent_length);
    go_to_directory(new_parent, parent_length, directory_history);
    free(new_parent);
}

void add_move_in_history_button(const AABB* aabb, const V4 icon_co,
                                const b8 disable, const int mouse_button,
                                const i32 history_add,
                                SelectedItemValues* selected_item_values,
                                DirectoryHistory* directory_history)
{
    if (ui_window_add_icon_button(aabb->min, aabb->size, icon_co, 3.0f,
                                  disable))
    {
        move_in_history(history_add, selected_item_values, directory_history);
    }
    if (!disable && is_mouse_button_clicked(mouse_button))
    {
        move_in_history(history_add, selected_item_values, directory_history);
    }
}

b8 suggestion_selection(u32 index, b8 hit, b8 should_close, b8 item_clicked,
                        V4* text_color, void* data)
{
    if (hit)
    {
        SuggestionSelectionData* arguments = (SuggestionSelectionData*)data;
        if (item_clicked || *arguments->tab_index >= 0)
        {
            char* path = arguments->items.data[index].path;
            const u32 path_length = (u32)strlen(path);
            arguments->parent_directory->size = 0;
            for (u32 i = 0; i < path_length; ++i)
            {
                array_push(arguments->parent_directory, path[i]);
            }
            array_push(arguments->parent_directory, '\0');
            array_push(arguments->parent_directory, '\0');
            array_push(arguments->parent_directory, '\0');
            arguments->parent_directory->size -= 3;
            *arguments->cursor_index = arguments->parent_directory->size;
            arguments->change_directory = item_clicked;
            return item_clicked;
        }
    }
    return should_close;
}

void quick_access_load(DirectoryItemArray* array)
{
    FileAttrib file = file_read("saved/quick_access.txt");
    if (!file.buffer)
    {
        return;
    }
    CharArray line = { 0 };
    array_create(&line, 500);

    while (!file_end_of_file(&file))
    {
        file_line_read(&file, true, &line);
        if (*array_back(&line) == '\r')
        {
            *array_back(&line) = '\0';
            line.size--;
        }
        if (line.size && platform_directory_exists(line.data))
        {
            char* path = (char*)calloc(line.size + 1, sizeof(char));
            memcpy(path, line.data, line.size);
            DirectoryItem item = {
                .path = path,
                .name = path + get_path_length(path, line.size),
            };
            array_push(array, item);
        }
        line.size = 0;
    }
    free(line.data);
}

void quick_access_save(DirectoryItemArray* array)
{
    if (!array->size) return;

    u32 buffer_size = 0;
    u32* path_lengths = (u32*)calloc(array->size, sizeof(u32));
    for (u32 i = 0; i < array->size; ++i)
    {
        path_lengths[i] = (u32)strlen(array->data[i].path);
        buffer_size += path_lengths[i];
        buffer_size++;
    }

    char* buffer = (char*)calloc(buffer_size, sizeof(char));
    u32 buffer_offset = 0;
    for (u32 i = 0; i < array->size; ++i)
    {
        memcpy(buffer + buffer_offset, array->data[i].path, path_lengths[i]);
        buffer_offset += path_lengths[i];
        buffer[buffer_offset++] = '\n';
    }
    buffer[--buffer_offset] = '\0';

    file_write("saved/quick_access.txt", buffer, buffer_offset);

    free(path_lengths);
    free(buffer);
}

void look_for_dropped_files(DirectoryPage* current, const char* directory_path)
{
    const CharPtrArray* dropped_paths = event_get_drop_buffer();
    if (dropped_paths->size)
    {
        platform_paste_to_directory(dropped_paths,
                                    directory_path ? directory_path
                                                   : current->directory.parent);
        reload_directory(current);
    }
}

void drag_drop_callback(void* data)
{
    CharPtrArray* selected_paths = (CharPtrArray*)data;
    platform_start_drag_drop(selected_paths);
}

void set_sorting_buttons(const ApplicationContext* application,
                         const char* text, const AABB* aabb,
                         const SortBy sort_by, const b8 hover, u32* index_count,
                         DirectoryTab* tab, RenderingProperties* render)
{
    V4 name_button_color = clear_color;
    if (hover)
    {
        name_button_color = high_light_color;
        if (is_mouse_button_clicked(FTIC_MOUSE_BUTTON_LEFT))
        {
            DirectoryPage* current = current_directory(&tab->directory_history);
            if (current->sort_by != sort_by)
            {
                current->sort_by = sort_by;
                current->sort_count = 1;
            }
            else
            {
                current->sort_count++;
                if (current->sort_count > 2)
                {
                    current->sort_count = 0;
                    current->sort_by = SORT_NONE;
                }
            }
            reload_directory(current_directory(&tab->directory_history));
        }
    }
    quad(&render->vertices, aabb->min, aabb->size, name_button_color, 0.0f);
    *index_count += 6;
    quad_border(&render->vertices, index_count, aabb->min, aabb->size,
                border_color, border_width, 0.0f);
    const V2 name_text_position =
        v2f(aabb->min.x + 10.0f,
            aabb->min.y + application->font.pixel_height + 9.0f);
    *index_count += text_generation(
        application->font.chars, text, 1.0f, name_text_position, 1.0f,
        application->font.pixel_height, NULL, NULL, NULL, &render->vertices);
}

void show_search_result_window(SearchPage* page, const u32 window,
                               const f32 list_item_height,
                               DirectoryHistory* directory_history)
{
    ui_window_begin(window, true);
    {
        platform_mutex_lock(&page->search_result_file_array.mutex);
        platform_mutex_lock(&page->search_result_folder_array.mutex);

        V2 list_position = v2f(10.0f, 10.0f);
        i32 selected_item = -1;
        if (ui_window_add_folder_list(list_position, list_item_height,
                                      &page->search_result_folder_array.array,
                                      NULL, &selected_item))
        {
            open_folder(
                page->search_result_folder_array.array.data[selected_item].path,
                directory_history);
        }
        list_position.y +=
            list_item_height * page->search_result_folder_array.array.size;
        if (ui_window_add_file_list(list_position, list_item_height,
                                    &page->search_result_file_array.array, NULL,
                                    &selected_item))
        {
            platform_open_file(
                page->search_result_file_array.array.data[selected_item].path);
        }

        platform_mutex_unlock(&page->search_result_file_array.mutex);
        platform_mutex_unlock(&page->search_result_folder_array.mutex);
    }
    ui_window_end("Search result", false);
}

char* get_parent_directory_name(DirectoryPage* current)
{
    return current->directory.parent +
           get_path_length(current->directory.parent,
                           (u32)strlen(current->directory.parent));
}

b8 show_directory_window(const u32 window, const f32 list_item_height,
                         DirectoryTab* tab)
{
    DirectoryPage* current = current_directory(&tab->directory_history);
    ui_window_begin(window, true);
    {
        V2 list_position = v2f(10.0f, 10.0f);
        i32 selected_item = -1;
        if (ui_window_add_folder_list(list_position, list_item_height,
                                      &current->directory.sub_directories,
                                      &tab->selected_item_values,
                                      &selected_item))
        {
            open_folder(
                current->directory.sub_directories.data[selected_item].path,
                &tab->directory_history);
        }
        list_position.y +=
            list_item_height * current->directory.sub_directories.size;
        if (ui_window_add_file_list(list_position, list_item_height,
                                    &current->directory.files,
                                    &tab->selected_item_values, &selected_item))
        {
            platform_open_file(
                current->directory.files.data[selected_item].path);
        }
    }
    return ui_window_end(get_parent_directory_name(current), true);
}

#if 0
void render_node(DockNode* node, int depth)
{
    if (!node) return;
    for (int i = 0; i < depth; i++)
    {
        printf("  ");
    }
    if (node->type == NODE_LEAF)
    {
        printf("Leaf: %u (Size: %.2f, %.2f, Position: %.2f, %.2f)\n",
               node->window->id, node->window->size.width,
               node->window->size.height, node->window->position.x,
               node->window->position.y);
    }
    else
    {
        printf("Parent (%s):\n", node->split_axis == SPLIT_HORIZONTAL
                                     ? "Horizontal"
                                     : "Vertical");
        for (int i = 0; i < 2; i++)
        {
            render_node(node->children[i], depth + 1);
        }
    }
}
#endif

b8 drop_down_menu_add(DropDownMenu2* drop_down_menu,
                      const ApplicationContext* application, void* option_data)
{
    const u32 drop_down_item_count = drop_down_menu->options.size;
    if (drop_down_item_count == 0) return true;

    drop_down_menu->x += (f32)(application->delta_time * 2.0);

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
        &drop_down_menu->render->vertices, drop_down_menu->index_count,
        v2f(drop_down_menu->position.x - drop_down_border_width,
            drop_down_menu->position.y - drop_down_border_width),
        v2f(drop_down_width + border_extra_padding,
            current_y + border_extra_padding),
        lighter_color, lighter_color, drop_down_border_width, 0.0f);

    b8 item_clicked = false;

    b8 mouse_button_clicked = is_mouse_button_clicked(FTIC_MOUSE_BUTTON_1);

    if (is_key_pressed_repeat(FTIC_KEY_TAB))
    {
        if (event_get_key_event()->shift_pressed)
        {
            if (--drop_down_menu->tab_index < 0)
            {
                drop_down_menu->tab_index = drop_down_item_count - 1;
            }
        }
        else
        {
            drop_down_menu->tab_index++;
            drop_down_menu->tab_index %= drop_down_item_count;
        }
    }

    b8 enter_clicked = is_key_clicked(FTIC_KEY_ENTER);

    b8 any_drop_down_item_hit = false;
    b8 should_close = false;
    const f32 promt_item_text_padding = 5.0f;
    V2 promt_item_position = drop_down_menu->position;
    for (u32 i = 0; i < drop_down_item_count; ++i)
    {
        AABB drop_down_item_aabb = {
            .min = promt_item_position,
            .size = v2f(drop_down_width, drop_down_item_height * precent),
        };

        b8 drop_down_item_hit = drop_down_menu->tab_index == (i32)i;
        if (drop_down_item_hit) item_clicked = enter_clicked;
        if (collision_point_in_aabb(application->mouse_position,
                                    &drop_down_item_aabb))
        {
            if (drop_down_menu->tab_index == -1 ||
                event_get_mouse_move_event()->activated)
            {
                drop_down_item_hit = true;
                drop_down_menu->tab_index = -1;
                any_drop_down_item_hit = true;
                item_clicked = mouse_button_clicked;
            }
        }

        const V4 drop_down_color =
            drop_down_item_hit ? border_color : high_light_color;

        quad(&drop_down_menu->render->vertices, promt_item_position,
             drop_down_item_aabb.size, drop_down_color, 0.0f);
        *drop_down_menu->index_count += 6;

        V2 promt_item_text_position = promt_item_position;
        promt_item_text_position.y +=
            application->font.pixel_height + promt_item_text_padding + 3.0f;
        promt_item_text_position.x += promt_item_text_padding;

        V4 text_color = v4i(1.0f);
        should_close = drop_down_menu->menu_options_selection(
            i, drop_down_item_hit, should_close, item_clicked, &text_color,
            option_data);

        *drop_down_menu->index_count += text_generation_color(
            application->font.chars, drop_down_menu->options.data[i], 1.0f,
            promt_item_text_position, 1.0f,
            application->font.pixel_height * precent, text_color, NULL, NULL,
            NULL, &drop_down_menu->render->vertices);

        promt_item_position.y += drop_down_item_aabb.size.y;
    }
    return should_close ||
           ((!any_drop_down_item_hit) && mouse_button_clicked) ||
           is_key_clicked(FTIC_KEY_ESCAPE);
}

void get_suggestions(const FontTTF* font, const V2 position,
                     DirectoryPage* current,
                     InputBuffer* parent_directory_input,
                     DropDownMenu2* suggestions,
                     SuggestionSelectionData* suggestion_data)
{
    char* path = parent_directory_input->buffer.data;
    u32 current_directory_len =
        get_path_length(path, parent_directory_input->buffer.size);
    if (current_directory_len != 0) current_directory_len--;

    Directory directory = { 0 };
    char saved_chars[3];
    saved_chars[0] = path[current_directory_len];
    path[current_directory_len] = '\0';
    if (platform_directory_exists(path))
    {
        path[current_directory_len++] = '\\';
        saved_chars[1] = path[current_directory_len];
        path[current_directory_len++] = '*';
        saved_chars[2] = path[current_directory_len];
        path[current_directory_len] = '\0';
        directory = platform_get_directory(path, current_directory_len);
        path[current_directory_len--] = saved_chars[2];
        path[current_directory_len--] = saved_chars[1];

        suggestions->tab_index = -1;
    }
    path[current_directory_len] = saved_chars[0];

    for (u32 i = 0; i < directory.sub_directories.size; ++i)
    {
        DirectoryItem* dir_item = directory.sub_directories.data + i;
        dir_item->size = string_span_case_insensitive(
            dir_item->name, path + current_directory_len + 1);
    }
    directory_sort_by_size(&directory.sub_directories);
    directory_flip_array(&directory.sub_directories);

    suggestions->options.size = 0;
    suggestion_data->items.size = 0;
    const u32 item_count = min(directory.sub_directories.size, 6);
    for (u32 i = 0; i < item_count; ++i)
    {
        array_push(&suggestions->options,
                   directory.sub_directories.data[i].name);
        array_push(&suggestion_data->items, directory.sub_directories.data[i]);
    }

    const f32 x_advance =
        text_x_advance(font->chars, parent_directory_input->buffer.data,
                       parent_directory_input->buffer.size, 1.0f);

    suggestions->position =
        v2f(position.x + x_advance, position.y + font->pixel_height + 20.0f);
}

void set_input_buffer_to_current_directory(const char* path, InputBuffer* input)
{
    input->buffer.size = 0;
    const u32 parent_length = (u32)strlen(path);
    for (u32 i = 0; i < parent_length; ++i)
    {
        array_push(&input->buffer, path[i]);
    }
    array_push(&input->buffer, '\0');
    array_push(&input->buffer, '\0');
    array_push(&input->buffer, '\0');
    input->buffer.size -= 3;
    input->input_index = input->buffer.size;
}

int main(int argc, char** argv)
{
    ApplicationContext application = { 0 };
    u8* font_bitmap = application_initialize(&application);

    platform_init_drag_drop();

    ThreadQueue thread_queue = { 0 };
    thread_init(100000, 8, &thread_queue);

    SearchPage search_page = { 0 };
    search_page_initialize(&search_page);

    RenderingPropertiesArray rendering_properties = { 0 };
    rendering_properties_array_initialize(&application.font, font_bitmap,
                                          &rendering_properties);

    RenderingProperties* main_render = rendering_properties.data;
    u32 main_index_count = 0;

    U32Array windows = { 0 };
    array_create(&windows, 20);
    {
        ui_context_create();
        for (u32 i = 0; i < 20; ++i)
        {
            array_push(&windows, ui_window_create());
        }
    }

    u32 current_window_index = 0;
    u32 top_bar_window = windows.data[current_window_index++];
    u32 quick_access_window = windows.data[current_window_index++];
    u32 search_result_window = windows.data[current_window_index++];
    u32 preview_window = windows.data[current_window_index++];

    for (u32 i = 0; i < application.tabs.size; ++i)
    {
        application.tabs.data[i].window_id =
            windows.data[current_window_index++];
    }

    CharPtrArray pasted_paths = { 0 };
    array_create(&pasted_paths, 10);

    b8 right_clicked = false;

    DropDownMenu2 drop_down_menu = {
        .index_count = &main_index_count,
        .tab_index = -1,
        .menu_options_selection = main_drop_down_selection,
        .render = main_render,
    };
    char* options[] = {
        [COPY_OPTION_INDEX] = "Copy",
        [PASTE_OPTION_INDEX] = "Paste",
        [DELETE_OPTION_INDEX] = "Delete",
        [ADD_TO_QUICK_OPTION_INDEX] = "Add to quick",
        [PROPERTIES_OPTION_INDEX] = "Properties",
    };
    array_create(&drop_down_menu.options, static_array_size(options));
    array_push(&drop_down_menu.options, options[COPY_OPTION_INDEX]);
    array_push(&drop_down_menu.options, options[PASTE_OPTION_INDEX]);
    array_push(&drop_down_menu.options, options[DELETE_OPTION_INDEX]);
    array_push(&drop_down_menu.options, options[ADD_TO_QUICK_OPTION_INDEX]);
    array_push(&drop_down_menu.options, options[PROPERTIES_OPTION_INDEX]);

    V2 preview_image_dimensions = v2d();
    char* current_path = NULL;
    b8 show_preview = false;
    FileAttrib preview_file = { 0 };
    U32Array preview_textures = { 0 };
    array_create(&preview_textures, 10);

    DirectoryItemArray quick_access_folders = { 0 };
    array_create(&quick_access_folders, 10);
    quick_access_load(&quick_access_folders);

    InputBuffer parent_directory_input = ui_input_buffer_create();

    DropDownMenu2 suggestions = {
        .index_count = &main_index_count,
        .tab_index = -1,
        .menu_options_selection = suggestion_selection,
        .render = main_render,
    };
    array_create(&suggestions.options, 10);

    SuggestionSelectionData suggestion_data = {
        .parent_directory = &parent_directory_input.buffer,
        .tab_index = &suggestions.tab_index,
        .cursor_index = &parent_directory_input.input_index,
    };
    array_create(&suggestion_data.items, 6);
    AABBArray parent_directory_aabbs = { 0 };
    array_create(&parent_directory_aabbs, 10);

    const f32 search_bar_width = 250.0f;

    b8 activated = false;
    V2 last_mouse_position = v2d();
    f32 distance = 0.0f;

    U32Array free_window_ids = { 0 };
    array_create(&free_window_ids, 10);

    char* menu_options[] = { "Menu", "Windows" };
    CharPtrArray menu_values = { 0 };
    array_create(&menu_values, 10);
    array_push(&menu_values, menu_options[0]);
    array_push(&menu_values, menu_options[1]);

    enable_gldebugging();
    glEnable(GL_BLEND);
    glEnable(GL_MULTISAMPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    while (!window_should_close(application.window))
    {
        application_begin_frame(&application);

        DirectoryTab* tab = application.tabs.data + application.tab_index;

        main_index_count = 0;

        rendering_properties_array_clear(&rendering_properties);

        if (is_ctrl_and_key_pressed(FTIC_KEY_T))
        {
            array_push(&application.tabs, tab_add("C:\\*"));
            application.tab_index = application.tabs.size - 1;
            u32 window_id = 0;
            if (free_window_ids.size)
            {
                window_id = *array_back(&free_window_ids);
                free_window_ids.size--;
            }
            else
            {
                window_id = windows.data[current_window_index++];
            }
            array_back(&application.tabs)->window_id = window_id;
        }

        if (is_ctrl_and_key_range_pressed(FTIC_KEY_1, FTIC_KEY_9))
        {
            const u32 index = event_get_key_event()->key - FTIC_KEY_1;
            if (index < application.tabs.size)
            {
                application.tab_index = index;
            }
        }

        const MouseButtonEvent* mouse_button_event =
            event_get_mouse_button_event();
        const MouseMoveEvent* mouse_move_event = event_get_mouse_move_event();
        const KeyEvent* key_event = event_get_key_event();

        if (mouse_button_event->activated)
        {
            last_mouse_position = application.mouse_position;
            distance = 0.0f;
            activated = true;
        }

        if (mouse_button_event->action == FTIC_RELEASE)
        {
            activated = false;
        }

        if (activated && mouse_button_event->action == FTIC_PRESS &&
            mouse_move_event->activated &&
            tab->selected_item_values.paths.size > 0)
        {
            distance =
                v2_distance(last_mouse_position, application.mouse_position);
            if (distance >= 10.0f)
            {
                // platform_start_drag_drop(&tab->selected_item_values.paths);
                activated = false;
            }
        }
        b8 check_collision = true;

        if (check_collision &&
            (collision_point_in_aabb(application.mouse_position,
                                     &drop_down_menu.aabb) ||
             collision_point_in_aabb(application.mouse_position,
                                     &suggestions.aabb)))
        {
            check_collision = false;
        }

        if (tab->selected_item_values.paths.size)
        {
            if (is_ctrl_and_key_pressed(FTIC_KEY_C))
            {
                platform_copy_to_clipboard(&tab->selected_item_values.paths);
            }
            else if (!show_preview &&
                     tab->selected_item_values.paths.size == 1 &&
                     is_ctrl_and_key_pressed(FTIC_KEY_D))
            {
                const char* path = tab->selected_item_values.paths.data[0];

                show_preview = true;
                if (load_preview_image(path, &preview_image_dimensions,
                                       &preview_textures))
                {
                    current_path = string_copy(path, (u32)strlen(path), 0);
                    reset_selected_items(&tab->selected_item_values);
                }
                else
                {
                    preview_file = file_read(path);
                    show_preview = true;
                }
            }
            else if (key_event->activated && key_event->action == 1 &&
                     key_event->key == FTIC_KEY_ESCAPE)
            {
                // TODO: Not get called when you press escape in a drop down for
                // example
                // platform_delete_files(&tab->selected_item_values.paths);
                // reload_directory(
                //   current_directory(&application.directory_history));
            }
            else if (check_collision && !key_event->ctrl_pressed &&
                     mouse_button_event->activated &&
                     mouse_button_event->action == 0 &&
                     mouse_button_event->button == FTIC_MOUSE_BUTTON_1)
            {
                reset_selected_items(&tab->selected_item_values);
            }
        }
        if (is_ctrl_and_key_pressed(FTIC_KEY_V))
        {
            paste_in_directory(current_directory(&tab->directory_history));
        }
        if (is_mouse_button_clicked(FTIC_MOUSE_BUTTON_2))
        {
            right_clicked = true;
            drop_down_menu.position = application.mouse_position;
            drop_down_menu.position.x += 18.0f;
            drop_down_menu.x = 0.0f;
        }
        if (right_clicked)
        {
            MainDropDownSelectionData data = {
                .quick_access = &quick_access_folders,
                .directory = current_directory(&tab->directory_history),
                .selected_paths = &tab->selected_item_values.paths,
            };
            right_clicked =
                !drop_down_menu_add(&drop_down_menu, &application, &data);
        }
        else
        {
            drop_down_menu.aabb = (AABB){ 0 };
        }
        const char* directory_to_drop_in = NULL;
#if 0
        if (main_list_return_value.hit_index >= 0 &&
            main_list_return_value.type == TYPE_FOLDER)
        {
            directory_to_drop_in = current_directory(&tab->directory_history)
                                       ->directory.sub_directories
                                       .data[main_list_return_value.hit_index]
                                       .path;
        }
        else if (quick_access_list_return_value.hit_index >= 0 &&
                 quick_access_list_return_value.type == TYPE_FOLDER)
        {
            directory_to_drop_in =
                quick_access_folders
                    .data[quick_access_list_return_value.hit_index]
                    .path;
        }
#else
        look_for_dropped_files(current_directory(&tab->directory_history),
                               directory_to_drop_in);

        const f32 top_bar_height = 52.0f;
        const f32 top_bar_menu_height = 10.0f + application.font.pixel_height;

        AABB dock_space = { .min = v2f(0.0f,
                                       top_bar_height + top_bar_menu_height) };
        dock_space.size = v2_sub(application.dimensions, dock_space.min);
        ui_context_begin(application.dimensions, &dock_space,
                         application.delta_time, check_collision);
        {

            UiWindow* top_bar = ui_window_get(windows.data[0]);
            top_bar->position = v2d();
            top_bar->size = v2f(application.dimensions.width,
                                top_bar_height + top_bar_menu_height);
            top_bar->top_color = v4ic(0.2f);
            top_bar->bottom_color = v4ic(0.15f);

            ui_window_begin(top_bar_window, false);
            {
                f32 height = 0.0f;
                ui_window_add_menu_bar(&menu_values, &height);

                AABB button_aabb = { 0 };
                button_aabb.size = v2i(40.0f),
                button_aabb.min = v2f(10.0f, top_bar_menu_height + 5.0f);

                ui_window_row_begin(0.0f);

                b8 disable = tab->directory_history.current_index <= 0;
                add_move_in_history_button(&button_aabb, arrow_back_icon_co,
                                           disable, FTIC_MOUSE_BUTTON_4, -1,
                                           &tab->selected_item_values,
                                           &tab->directory_history);

                disable = tab->directory_history.history.size <=
                          tab->directory_history.current_index + 1;
                add_move_in_history_button(&button_aabb, arrow_right_icon_co,
                                           disable, FTIC_MOUSE_BUTTON_4, 1,
                                           &tab->selected_item_values,
                                           &tab->directory_history);

                disable = !can_go_up_one_directory(
                    current_directory(&tab->directory_history)
                        ->directory.parent);
                if (ui_window_add_icon_button(button_aabb.min, button_aabb.size,
                                              arrow_up_icon_co, 3.0f, disable))
                {
                    go_up_one_directory(&tab->directory_history);
                }

                button_aabb.min.x += ui_window_row_end() + 10.0f;
                button_aabb.size.x = (application.dimensions.width -
                                      (search_bar_width + 20.0f)) -
                                     button_aabb.min.x;

                b8 active_before = parent_directory_input.active;
                if (ui_window_add_input_field(button_aabb.min, button_aabb.size,
                                              &parent_directory_input))
                {
                    DirectoryPage* current =
                        current_directory(&tab->directory_history);
                    get_suggestions(&application.font, button_aabb.min, current,
                                    &parent_directory_input, &suggestions,
                                    &suggestion_data);
                }

                if (parent_directory_input.active)
                {
                    if (!active_before)
                    {
                        const char* path =
                            current_directory(&tab->directory_history)
                                ->directory.parent;
                        set_input_buffer_to_current_directory(
                            path, &parent_directory_input);
                    }
                    suggestion_data.change_directory = false;
                    drop_down_menu_add(&suggestions, &application,
                                       &suggestion_data);

                    if (suggestion_data.change_directory ||
                        is_key_clicked(FTIC_KEY_ENTER))
                    {
                        char* last_char =
                            array_back(&parent_directory_input.buffer);
                        char saved = *last_char;
                        b8 should_restore =
                            *last_char == '\\' || *last_char == '/';
                        if (should_restore)
                        {
                            *last_char = '\0';
                            parent_directory_input.buffer.size--;
                        }
                        if (!go_to_directory(parent_directory_input.buffer.data,
                                             parent_directory_input.buffer.size,
                                             &tab->directory_history))
                        {
                        }
                        if (should_restore)
                        {
                            *last_char = saved;
                            parent_directory_input.buffer.size++;
                        }
                        parent_directory_input.input_index =
                            parent_directory_input.buffer.size;
                        suggestions.tab_index = -1;
                        suggestions.options.size = 0;
                    }
                }
                else
                {
                    const char* path =
                        current_directory(&tab->directory_history)
                            ->directory.parent;
                    set_input_buffer_to_current_directory(
                        path, &parent_directory_input);
                }
                if (is_mouse_button_clicked(FTIC_MOUSE_BUTTON_LEFT))
                {
                    if (!collision_point_in_aabb(application.mouse_position,
                                                 &suggestions.aabb))
                    {
                        suggestions.aabb = (AABB){ 0 };
                        suggestions.x = 0;
                    }
                }

                button_aabb.min.x += button_aabb.size.x + 10.0f;
                button_aabb.size.x = search_bar_width;
                if (ui_window_add_input_field(button_aabb.min, button_aabb.size,
                                              &search_page.input))
                {
                    search_page_search(&search_page, &tab->directory_history,
                                       &thread_queue.task_queue);
                }
            }
            ui_window_end(NULL, false);

            const f32 list_item_height = application.font.pixel_height + 10.0f;

            ui_window_begin(quick_access_window, true);
            {
                V2 list_position = v2i(10.0f);
                i32 selected_item = -1;
                if (ui_window_add_folder_list(list_position, list_item_height,
                                              &quick_access_folders, NULL,
                                              &selected_item))
                {
                    open_folder(quick_access_folders.data[selected_item].path,
                                &tab->directory_history);
                }
            }
            char buffer[20] = { 0 };
            sprintf_s(buffer, 20, "%f", application.delta_time);
            ui_window_end(buffer, false);
            // ui_window_end("Quick access", false);

            show_search_result_window(&search_page, search_result_window,
                                      list_item_height,
                                      &tab->directory_history);

            if (show_preview)
            {
                if (preview_textures.size > 0)
                {
                    UiWindow* preview = ui_window_get(preview_window);

                    V2 image_dimensions = load_and_scale_preview_image(
                        &application, &preview_image_dimensions, &current_path,
                        &current_directory(&tab->directory_history)
                             ->directory.files,
                        &preview_textures);

                    preview->size = image_dimensions;
                    preview->position =
                        v2f(middle(application.dimensions.width,
                                   preview->size.width),
                            middle(application.dimensions.height,
                                   preview->size.height));

                    ui_window_begin(windows.data[3], false);
                    {
                        show_preview = !ui_window_set_overlay();
                        ui_window_add_image(v2d(), image_dimensions,
                                            preview_textures.data[0]);
                    }
                    ui_window_end(NULL, false);
                }
                else
                {
                    UiWindow* preview = ui_window_get(windows.data[3]);
                    preview->size =
                        v2f(500.0f, application.dimensions.height * 0.9f);
                    preview->position =
                        v2f(middle(application.dimensions.width,
                                   preview->size.width),
                            middle(application.dimensions.height,
                                   preview->size.height));

                    ui_window_begin(windows.data[3], false);
                    {
                        show_preview = !ui_window_set_overlay();
                        ui_window_add_text(v2f(10.0f, 10.0f),
                                           (char*)preview_file.buffer);
                    }
                    ui_window_end(NULL, false);
                }
            }
            else
            {
                if (preview_textures.size > 0)
                {
                    texture_delete(
                        preview_textures.data[--preview_textures.size]);
                }
                if (preview_file.buffer)
                {
                    free(preview_file.buffer);
                    preview_file = (FileAttrib){ 0 };
                }
            }

            for (u32 i = 0; i < application.tabs.size; ++i)
            {
                const u32 window_id = application.tabs.data[i].window_id;
                if (ui_window_in_focus() == window_id)
                {
                    if (i != application.tab_index)
                    {
                        reset_selected_items(&tab->selected_item_values);
                    }
                    application.tab_index = i;
                }
                if (show_directory_window(window_id, list_item_height,
                                          application.tabs.data + i))
                {
                    DirectoryTab tab_to_remove = application.tabs.data[i];
                    for (u32 j = i; j < application.tabs.size - 1; ++j)
                    {
                        application.tabs.data[j] = application.tabs.data[j + 1];
                    }
                    tab_clear(&tab_to_remove);
                    application.tabs.size--;
                    array_push(&free_window_ids, window_id);

                    // NOTE: this is probably always true
                    if (i == application.tab_index)
                    {
                        application.tab_index =
                            (application.tab_index + 1) % application.tabs.size;
                    }
                    --i;
                }
            }

            b8 search_result_open = search_page_has_result(&search_page) &&
                                    search_page.input.buffer.size;
            if (!search_result_open)
            {
                search_page_clear_search_result(&search_page);
            }
        }
        ui_context_end();

        rendering_properties_check_and_grow_buffers(main_render,
                                                    main_index_count);

        buffer_set_sub_data(main_render->vertex_buffer_id, GL_ARRAY_BUFFER, 0,
                            sizeof(Vertex) * main_render->vertices.size,
                            main_render->vertices.data);

        AABB whole_screen_scissor = { .size = application.dimensions };

        rendering_properties_begin_draw(main_render, &application.mvp);
        rendering_properties_draw(0, main_index_count, &whole_screen_scissor);
        rendering_properties_end_draw(main_render);
#endif

        application_end_frame(&application);
    }
    memset(running_callbacks, 0, sizeof(running_callbacks));
    if (current_path) free(current_path);

    ui_context_destroy();
    quick_access_save(&quick_access_folders);

    for (u32 i = 0; i < rendering_properties.size; ++i)
    {
        RenderingProperties* rp = rendering_properties.data + i;
        buffer_delete(rp->vertex_buffer_id);
        buffer_delete(rp->index_buffer_id);
        for (u32 j = 0; j < rp->textures.size; ++j)
        {
            texture_delete(rp->textures.data[i]);
        }
        free(rp->textures.data);
    }
    // TODO: Cleanup of all

    threads_destroy(&thread_queue);
    platform_uninit_drag_drop();
    application_uninitialize(&application);
}
