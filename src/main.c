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
#include "util.h"

#define COPY_OPTION_INDEX 0
#define PASTE_OPTION_INDEX 1
#define DELETE_OPTION_INDEX 2
#define ADD_TO_QUICK_OPTION_INDEX 3
#define PROPERTIES_OPTION_INDEX 4

global const V4 clear_color = {
    .r = 0.1f,
    .g = 0.1f,
    .b = 0.1f,
    .a = 1.0f,
};
global const V4 high_light_color = {
    .r = 0.2f,
    .g = 0.2f,
    .b = 0.2f,
    .a = 1.0f,
};

global const V4 border_color = {
    .r = 0.35f,
    .g = 0.35f,
    .b = 0.35f,
    .a = 1.0f,
};

global const V4 lighter_color = {
    .r = 0.55f,
    .g = 0.55f,
    .b = 0.55f,
    .a = 1.0f,
};
global const V4 bright_color = {
    .r = 0.7f,
    .g = 0.7f,
    .b = 0.7f,
    .a = 1.0f,
};
global const V4 secondary_color = {
    .r = 0.0f,
    .g = 0.59f,
    .b = 1.0f,
    .a = 1.0f,
};

global const f32 border_width = 1.0f;

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

typedef struct U32Array
{
    u32 size;
    u32 capacity;
    u32* data;
} U32Array;

typedef struct DirectoryItemList
{
    DirectoryItemArray* items;
    f64 pulse_x;
} DirectoryItemList;

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
    f64 pulse_x;
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

typedef struct DirectoryHistory
{
    u32 current_index;
    DirectoryArray history;
} DirectoryHistory;

DirectoryPage* current_directory(DirectoryHistory* history)
{
    return history->history.data + history->current_index;
}

typedef struct DirectoryTab
{
    DirectoryHistory directory_history;
    SelectedItemValues selected_item_values;
} DirectoryTab;

typedef struct DirectoryTabArray
{
    u32 size;
    u32 capacity;
    DirectoryTab* data;
} DirectoryTabArray;

typedef struct ApplicationContext
{
    GLFWwindow* window;
    FontTTF font;

    u32 tab_index;
    DirectoryTabArray tabs;

    Event* key_event;
    Event* mouse_button;
    Event* mouse_wheel;
    Event* mouse_move;

    V2 dimensions;
    V2 mouse_position;

    f64 last_time;
    f64 delta_time;
    MVP mvp;

    f64 last_moved_time;
} ApplicationContext;

typedef struct SearchPage
{
    f32 offset;
    f32 scroll_offset;
    f64 search_blinking_time;
    f64 result_pulse_x;

    AABB search_bar_aabb;
    AABB search_result_aabb;

    CharArray search_buffer;
    SafeFileArray search_result_file_array;
    SafeFileArray search_result_folder_array;
    RenderingProperties* render;

    ScrollBar scroll_bar;

    u32 running_id;
    u32 last_running_id;

    i32 search_bar_cursor_index;
    b8 search_bar_hit;
} SearchPage;

typedef struct DropDownMenu
{
    f32 x;
    i32 tab_index;
    V2 position;
    AABB aabb;
    CharPtrArray options;
    RenderingProperties* render;
    b8 (*menu_options_selection)(u32 index, b8 hit, b8 should_close,
                                 b8 item_clicked, V4* text_color, void* data);
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

typedef struct DirectoryItemListReturnValue
{
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
    DirectoryItemArray* items;
    i32* cursor_index;
    i32* tab_index;
    b8 change_directory;
} SuggestionSelectionData;

void set_gldebug_log_level(DebugLogLevel level);
void opengl_log_message(GLenum source, GLenum type, GLuint id, GLenum severity,
                        GLsizei length, const GLchar* message,
                        const void* userParam);
void enable_gldebugging();
void log_f32(const char* message, const f32 value);
void log_u64(const char* message, const u64 value);
f32 ease_out_elastic(const f32 x);
const char* get_file_extension(const char* path, const u32 path_length);
void parse_all_subdirectories(const char* start_directory, const u32 length);
b8 string_contains(const char* string, const u32 string_length,
                   const char* value, const u32 value_length);
b8 string_contains_case_insensitive(const char* string, const char* value);
char* copy_string(const char* string, const u32 string_length,
                  const u32 extra_length);
void safe_add_directory_item(const DirectoryItem* item,
                             FindingCallbackAttribute* arguments,
                             SafeFileArray* safe_array);
void finding_callback(void* data);
void find_matching_string(const char* start_directory, const u32 length,
                          const char* string_to_match,
                          const u32 string_to_match_length);
void clear_search_result(SafeFileArray* files);
void search_page_clear_search_result(SearchPage* page);
RenderingProperties rendering_properties_init(
    u32 shader_id, u32* textures, u32 texture_count, u32 index_buffer_id,
    const VertexBufferLayout* vertex_buffer_layout, u32 vertex_buffer_size);
void rendering_properties_draw(const RenderingProperties* rendering_properties,
                               const MVP* mvp);
void rendering_properties_check_and_grow_buffers(
    RenderingProperties* rendering_properties, IndexArray* index_array);
void rendering_properties_clear(RenderingProperties* rendering_properties);
void rendering_properties_array_clear(
    RenderingPropertiesArray* rendering_properties);
void format_file_size(u64 size_in_bytes, char* output, size_t output_size);
void swap_strings(char* first, char* second);
b8 directory_item(const ApplicationContext* appliction, b8 hit, i32 index,
                  V2 starting_position, const f32 padding_top, f32 width,
                  const f32 height, const f32 icon_index,
                  V4 texture_coordinates, const b8 check_collision,
                  const f64 pulse_x, DirectoryItem* item,
                  SelectedItemValues* selected_item_values, i32* hit_index,
                  RenderingProperties* render);
b8 item_in_view(const f32 position_y, const f32 height,
                const f32 window_height);
void check_and_open_file(const Event* mouse_button_event, const b8 hit,
                         const DirectoryItem* current_files,
                         const i32 file_index);
b8 go_to_directory(char* path, u32 length, DirectoryHistory* directory_history);
void check_and_open_folder(const Event* mouse_button_event, const b8 hit,
                           const i32 index,
                           const DirectoryItem* current_folders,
                           DirectoryHistory* directory_history);
DirectoryItemListReturnValue item_list(
    const ApplicationContext* application, const DirectoryItemArray* items,
    const f32 icon_index, const V4 texture_coordinates,
    const b8 check_collision, V2 starting_position, const f32 item_width,
    const f32 item_height, const f32 padding, const f64 pulse_x,
    SelectedItemValues* selected_item_values, RenderingProperties* render);
DirectoryItemListReturnValue folder_item_list(
    const ApplicationContext* application, const DirectoryItemArray* folders,
    const b8 check_collision, V2 starting_position, const f32 item_width,
    const f32 item_height, const f32 padding, const f64 pulse_x,
    SelectedItemValues* selected_item_values, RenderingProperties* render,
    DirectoryHistory* directory_history);
DirectoryItemListReturnValue files_item_list(
    const ApplicationContext* application, const DirectoryItemArray* files,
    const b8 check_collision, V2 starting_position, const f32 item_width,
    const f32 item_height, const f32 padding, const f64 pulse_x,
    SelectedItemValues* selected_item_values, RenderingProperties* render);
DirectoryItemListReturnValue directory_item_list(
    const ApplicationContext* application,
    const DirectoryItemArray* sub_directories, const DirectoryItemArray* files,
    const b8 check_collision, V2 starting_position, const f32 item_width,
    const f32 item_height, const f32 padding, const f64 pulse_x,
    SelectedItemValues* selected_item_values, RenderingProperties* render,
    DirectoryHistory* directory_history);
f32 clampf32_low(f32 value, f32 low);
f32 clampf32_high(f32 value, f32 high);
void extract_only_aplha_channel(TextureProperties* texture_properties);
u32 render_input(const FontTTF* font, const char* text, const u32 text_length,
                 const f32 scale, const V2 text_position, const f32 cursor_y,
                 const b8 active, const Event* key_event, i32 input_index,
                 const f64 delta_time, f64* time, AABBArray* aabbs,
                 RenderingProperties* render);
void set_scroll_offset(const u32 item_count, const f32 item_height,
                       const f32 area_y, const Event* mouse_wheel_event,
                       f32* offset);
f32 smooth_scroll(const f64 delta_time, const f32 offset, f32 scroll_offset);
u64 u64_hash_function(const void* data, u32 len, u64 seed);
void reset_selected_items(SelectedItemValues* selected_item_values);
void reload_directory(DirectoryPage* directory_page);
b8 is_ctrl_and_key_pressed(const Event* event, i32 key);
b8 is_key_clicked(const Event* event, i32 key);
b8 is_key_pressed_repeat(const Event* event, i32 key);
u32 load_icon_as_only_red(const char* file_path);
u32 load_icon_as_white(const char* file_path);
u32 load_icon(const char* file_path);
AABB ui_add_border(RenderingProperties* render, V2 position, V2 size);
void scroll_bar_add(ScrollBar* scroll_bar, V2 position,
                    const Event* mouse_button, const V2 mouse_position,
                    const f32 dimension_y, const f32 area_y,
                    const f32 item_height, f32 total_height, f32* scroll_offset,
                    f32* offset, RenderingProperties* render);
void paste_in_directory(DirectoryPage* current_directory);
b8 is_mouse_button_clicked(const Event* event, i32 button);
u8* application_init(ApplicationContext* application);
void search_page_init(SearchPage* search_page);
void rendering_properties_array_init(const u32 index_buffer_id,
                                     const FontTTF* font, u8* font_bitmap,
                                     RenderingPropertiesArray* array);
void application_begin_frame(ApplicationContext* application);
void application_end_frame(ApplicationContext* application);
f64 application_get_last_mouse_move_time(const ApplicationContext* appliction);
void search_page_parse_key_buffer(SearchPage* page, const f32 search_bar_width,
                                  const FontTTF* font,
                                  const b8 reload_search_result,
                                  DirectoryHistory* directory_history,
                                  ThreadTaskQueue* thread_task_queue);
b8 erase_char(const Event* key_event, i32* cursor_index, CharArray* buffer);
DirectoryItemListReturnValue
search_page_update(SearchPage* page, const ApplicationContext* application,
                   const b8 check_collision, const V2 search_bar_position,
                   DirectoryHistory* directory_history,
                   ThreadTaskQueue* thread_task_queue,
                   SelectedItemValues* selected_item_values);
b8 main_drop_down_selection(u32 index, b8 hit, b8 should_close, b8 item_clicked,
                            V4* text_color, void* data);
b8 drop_down_menu_add(DropDownMenu* drop_down_menu,
                      const ApplicationContext* application, void* option_data);
void open_preview(V2 image_dimensions, const ApplicationContext* application,
                  RenderingProperties* render);
b8 button_arrow_add(V2 position, const AABB* button_aabb,
                    const V4 texture_coordinates, const b8 check_collision,
                    const b8 extra_condition, const V2 mouse_position,
                    RenderingProperties* render);
void move_in_history(const i32 index_add,
                     SelectedItemValues* selected_item_values,
                     DirectoryHistory* directory_history);
b8 can_go_up_one_directory(char* parent);
u32 get_path_length(const char* path, u32 path_length);
void go_up_one_directory(DirectoryHistory* directory_history);
b8 button_move_in_history_add(const AABB* button_aabb, const b8 check_collision,
                              const b8 extra_condition, const V2 mouse_position,
                              const Event* mouse_button_event,
                              const int mouse_button, const i32 history_add,
                              const V4 icon_co,
                              SelectedItemValues* selected_item_values,
                              DirectoryHistory* directory_history,
                              RenderingProperties* render);
void directory_sort_by_size(DirectoryItemArray* array);
void directory_flip_array(DirectoryItemArray* array);
b8 suggestion_selection(u32 index, b8 hit, b8 should_close, b8 item_clicked,
                        V4* text_color, void* data);

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

void log_u64(const char* message, const u64 value)
{
    char buffer[100] = { 0 };
    sprintf_s(buffer, 100, "%s%llu", message, value);
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

char* copy_string(const char* string, const u32 string_length,
                  const u32 extra_length)
{
    char* result =
        (char*)calloc(string_length + extra_length + 1, sizeof(char));
    memcpy(result, string, string_length);
    return result;
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

void display_text_and_truncate_if_necissary(const FontTTF* font,
                                            const V2 position,
                                            const f32 total_width, char* text,
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
        swap_strings(text + j, saved_name); // Truncate
    }
    render->index_count += text_generation(font->chars, text, 1.0f, position,
                                           1.0f, font->pixel_height, NULL, NULL,
                                           NULL, &render->vertices);
    if (too_long)
    {
        memcpy(text + (i - 3), saved_name, sizeof(saved_name));
    }
}

b8 directory_item(const ApplicationContext* appliction, b8 hit, i32 index,
                  V2 starting_position, const f32 padding_top, f32 width,
                  const f32 height, const f32 icon_index,
                  V4 texture_coordinates, const b8 check_collision,
                  const f64 pulse_x, DirectoryItem* item,
                  SelectedItemValues* selected_item_values, i32* hit_index,
                  RenderingProperties* render)
{
    AABB aabb = { .min = starting_position, .size = v2f(width, height) };

    u32* check_if_selected = hash_table_get_char_u32(
        &selected_item_values->selected_items, item->path);
    b8 selected = check_if_selected ? true : false;

    b8 this_hit = false;
    if (check_collision &&
        collision_point_in_aabb(appliction->mouse_position, &aabb))
    {
        if (!hit)
        {
            *hit_index = index;
            this_hit = true;
            hit = true;
        }
        const MouseButtonEvent* event =
            &appliction->mouse_button->mouse_button_event;
        if (!check_if_selected && appliction->mouse_button->activated &&
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

    V4 color = clear_color;
    V4 left_side_color = clear_color;
    if (this_hit || selected)
    {
        // TODO: Checks selected 3 times
        const V4 end_color = selected ? v4ic(0.45f) : v4ic(0.3f);
        color = v4_lerp(selected ? v4ic(0.5f) : border_color, end_color,
                        ((f32)sin(pulse_x) + 1.0f) / 2.0f);

        starting_position.x += 8.0f;
        width -= 6.0f;
    }

    // Backdrop
    quad_gradiant_l_r(&render->vertices, aabb.min, aabb.size, color,
                      clear_color, 0.0f);
    render->index_count++;

    if (v4_equal(texture_coordinates, file_icon_co))
    {
        const char* extension =
            get_file_extension(item->name, (u32)strlen(item->name));
        if (extension)
        {
            if (!strcmp(extension, ".png"))
            {
                texture_coordinates = png_icon_co;
            }
            else if (!strcmp(extension, ".pdf"))
            {
                texture_coordinates = pdf_icon_co;
            }
        }
    }
    // Icon
    aabb =
        quad_co(&render->vertices,
                v2f(starting_position.x + 5.0f, starting_position.y + 3.0f),
                v2f(20.0f, 20.0f), v4i(1.0f), texture_coordinates, icon_index);
    render->index_count++;

    V2 text_position = v2_s_add(starting_position, padding_top);

    text_position.x += aabb.size.x;

    f32 x_advance = 0.0f;
    if (item->size) // NOTE(Linus): Add the size text to the right side
    {
        char buffer[100] = { 0 };
        format_file_size(item->size, buffer, 100);
        x_advance = text_x_advance(appliction->font.chars, buffer,
                                   (u32)strlen(buffer), 1.0f);
        V2 size_text_position = text_position;
        size_text_position.x = starting_position.x + width - x_advance - 5.0f;
        render->index_count += text_generation(
            appliction->font.chars, buffer, 1.0f, size_text_position, 1.0f,
            appliction->font.pixel_height, NULL, NULL, NULL, &render->vertices);
    }
    display_text_and_truncate_if_necissary(
        &appliction->font, text_position,
        (width - aabb.size.x - padding_top - x_advance - 10.0f), item->name,
        render);
    return hit;
}

b8 item_in_view(const f32 position_y, const f32 height, const f32 window_height)
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

b8 go_to_directory(char* path, u32 length, DirectoryHistory* directory_history)
{
    b8 result = false;
    char saved_chars[3];
    saved_chars[0] = path[length];
    path[length] = '\0';
    if (!string_compare_case_insensitive(
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

void check_and_open_folder(const Event* mouse_button_event, const b8 hit,
                           const i32 index,
                           const DirectoryItem* current_folders,
                           DirectoryHistory* directory_history)
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
            go_to_directory(path, length, directory_history);
        }
    }
}

DirectoryItemListReturnValue
item_list(const ApplicationContext* application,
          const DirectoryItemArray* items, const f32 icon_index,
          const V4 texture_coordinates, const b8 check_collision,
          V2 starting_position, const f32 item_width, const f32 item_height,
          const f32 padding, const f64 pulse_x,
          SelectedItemValues* selected_item_values, RenderingProperties* render)
{
    double x, y;
    window_get_mouse_position(application->window, &x, &y);
    V2 mouse_position = v2f((f32)x, (f32)y);

    const f32 starting_y = starting_position.y;
    b8 hit = false;
    i32 hit_index = -1;
    for (i32 i = 0; i < (i32)items->size; ++i)
    {
        if (item_in_view(starting_position.y, item_height,
                         application->dimensions.y))
        {
            hit = directory_item(application, hit, i, starting_position,
                                 application->font.pixel_height + padding,
                                 item_width, item_height, icon_index,
                                 texture_coordinates, check_collision, pulse_x,
                                 &items->data[i], selected_item_values,
                                 &hit_index, render);

#if 0
            quad_gradiant_l_r(&render->vertices,
                              v2f(starting_position.x,
                                  starting_position.y + item_height - 1.0f),
                              v2f(item_width, 1.0f), border_color,
                              high_light_color, 0.0f);
            render->index_count++;
#endif
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

DirectoryItemListReturnValue folder_item_list(
    const ApplicationContext* application, const DirectoryItemArray* folders,
    const b8 check_collision, V2 starting_position, const f32 item_width,
    const f32 item_height, const f32 padding, const f64 pulse_x,
    SelectedItemValues* selected_item_values, RenderingProperties* render,
    DirectoryHistory* directory_history)
{
    DirectoryItemListReturnValue list_return =
        item_list(application, folders, 2.0f, folder_icon_co, check_collision,
                  starting_position, item_width, item_height, padding, pulse_x,
                  selected_item_values, render);

    check_and_open_folder(application->mouse_button, list_return.hit,
                          list_return.hit_index, folders->data,
                          directory_history);
    return list_return;
}

DirectoryItemListReturnValue files_item_list(
    const ApplicationContext* application, const DirectoryItemArray* files,
    const b8 check_collision, V2 starting_position, const f32 item_width,
    const f32 item_height, const f32 padding, const f64 pulse_x,
    SelectedItemValues* selected_item_values, RenderingProperties* render)
{
    DirectoryItemListReturnValue list_return =
        item_list(application, files, 2.0f, file_icon_co, check_collision,
                  starting_position, item_width, item_height, padding, pulse_x,
                  selected_item_values, render);

    check_and_open_file(application->mouse_button, list_return.hit, files->data,
                        list_return.hit_index);

    return list_return;
}

void display_full_path(const FontTTF* font, const V2 position, const V2 size,
                       const f32 padding, char* path,
                       RenderingProperties* render)
{
    quad(&render->vertices, position, size, high_light_color, 0.0f);
    render->index_count++;
    V2 text_position = v2_s_add(position, padding);
    text_position.y += font->pixel_height;
    display_text_and_truncate_if_necissary(font, text_position, size.width,
                                           path, render);
}

DirectoryItemListReturnValue directory_item_list(
    const ApplicationContext* application,
    const DirectoryItemArray* sub_directories, const DirectoryItemArray* files,
    const b8 check_collision, V2 starting_position, const f32 item_width,
    const f32 item_height, const f32 padding, const f64 pulse_x,
    SelectedItemValues* selected_item_values, RenderingProperties* render,
    DirectoryHistory* directory_history)
{
    b8 should_show_full_path =
        application_get_last_mouse_move_time(application) >= 1.0f;

    DirectoryItemListReturnValue folder_list_return = folder_item_list(
        application, sub_directories, check_collision, starting_position,
        item_width, item_height, padding, pulse_x, selected_item_values, render,
        directory_history);

    /*
    if (should_show_full_path && folder_list_return.hit_index > 0)
    {
        // TODO: Top item can't be shown.
        u32 it = folder_list_return.hit_index - 1;
        V2 position =
            v2f(starting_position.x, starting_position.y + (item_height * it));

        display_full_path(
            &application->font, position, v2f(item_width, item_height), padding,
            sub_directories->data[folder_list_return.hit_index].path, render);
    }
    */

    starting_position.y += folder_list_return.total_height;

    DirectoryItemListReturnValue files_list_return = files_item_list(
        application, files, check_collision, starting_position, item_width,
        item_height, padding, pulse_x, selected_item_values, render);

    if (should_show_full_path && files_list_return.hit_index > 0)
    {
    }

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

u32 render_input(const FontTTF* font, const char* text, const u32 text_length,
                 const f32 scale, const V2 text_position, const f32 cursor_y,
                 const b8 active, const Event* key_event, i32 input_index,
                 const f64 delta_time, f64* time, AABBArray* aabbs,
                 RenderingProperties* render)
{
    // Blinking cursor
    if (active)
    {
        if (input_index < 0)
        {
            input_index = text_length;
        }
        else
        {
            input_index = min((i32)text_length, input_index);
        }
        if (is_key_pressed_repeat(key_event, FTIC_KEY_LEFT))
        {
            input_index = max(input_index - 1, 0);
            *time = 0.4f;
        }
        if (is_key_pressed_repeat(key_event, FTIC_KEY_RIGHT))
        {
            input_index = min(input_index + 1, (i32)text_length);
            *time = 0.4f;
        }
        *time += delta_time;
        if (*time >= 0.4f)
        {
            const f32 x_advance =
                text_x_advance(font->chars, text, input_index, scale);

            quad(&render->vertices,
                 v2f(text_position.x + x_advance + 1.0f, cursor_y + 2.0f),
                 v2f(2.0f, 16.0f), v4i(1.0f), 0.0f);
            render->index_count++;

            *time = *time >= 0.8f ? 0 : *time;
        }
    }
    if (text_length)
    {
        render->index_count += text_generation(
            font->chars, text, 1.0f, text_position, scale, font->line_height,
            NULL, NULL, aabbs, &render->vertices);
    }
    return input_index;
}

void set_scroll_offset(const u32 item_count, const f32 item_height,
                       const f32 area_y, const Event* mouse_wheel_event,
                       f32* offset)
{
    const f32 total_height = item_count * item_height;
    const f32 y_offset = mouse_wheel_event->mouse_wheel_event.y_offset * 100.0f;
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

u64 u64_hash_function(const void* data, u32 len, u64 seed)
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

b8 is_ctrl_and_key_range_pressed(const Event* event, i32 key_low, i32 key_high)
{
    return event->activated && event->key_event.action == 1 &&
           event->key_event.ctrl_pressed &&
           (closed_interval(key_low, event->key_event.key, key_high));
}

b8 is_key_clicked(const Event* event, i32 key)
{
    return event->activated && event->key_event.action == FTIC_RELEASE &&
           event->key_event.key == key;
}

b8 is_key_pressed_repeat(const Event* event, i32 key)
{
    return event->activated &&
           (event->key_event.action == FTIC_PRESS ||
            event->key_event.action == FTIC_REPEAT) &&
           event->key_event.key == key;
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

u32 load_icon_as_white(const char* file_path)
{
    TextureProperties texture_properties = { 0 };
    texture_load(file_path, &texture_properties);
    const u32 size = texture_properties.height * texture_properties.width;
    const u32 size_in_bytes = size * 4;
    for (u32 i = 0; i < size_in_bytes; i += 4)
    {
        texture_properties.bytes[i] = UINT8_MAX;
        texture_properties.bytes[i + 1] = UINT8_MAX;
        texture_properties.bytes[i + 2] = UINT8_MAX;
    }
    u32 icon_texture = texture_create(&texture_properties, GL_RGBA8, GL_RGBA);
    free(texture_properties.bytes);
    return icon_texture;
}

u32 load_icon(const char* file_path)
{
    TextureProperties texture_properties = { 0 };
    texture_load(file_path, &texture_properties);
    u32 icon_texture = texture_create(&texture_properties, GL_RGBA8, GL_RGBA);
    free(texture_properties.bytes);
    return icon_texture;
}

AABB ui_add_border(RenderingProperties* render, V2 position, V2 size)
{
    AABB border_aabb =
        quad(&render->vertices, position, size, border_color, 0.0f);
    render->index_count++;
    return border_aabb;
}

void scroll_bar_add(ScrollBar* scroll_bar, V2 position,
                    const Event* mouse_button, const V2 mouse_position,
                    const f32 dimension_y, const f32 area_y,
                    const f32 item_height, f32 total_height, f32* scroll_offset,
                    f32* offset, RenderingProperties* render)
{
    const f32 scroll_bar_width = 8.0f;
    position.x -= scroll_bar_width;

    quad(&render->vertices, position, v2f(scroll_bar_width, area_y),
         high_light_color, 0.0f);
    render->index_count++;

    total_height += item_height;
    if (area_y < total_height)
    {
        const f32 initial_y = position.y;
        const f32 high = 0.0f;
        const f32 low = area_y - total_height;
        const f32 p = (*scroll_offset - low) / (high - low);
        const V2 scroll_bar_dimensions =
            v2f(scroll_bar_width, area_y * (area_y / total_height));

        position.y =
            lerp_f32(dimension_y - scroll_bar_dimensions.height, initial_y, p);

        AABB scroll_bar_aabb = {
            .min = position,
            .size = scroll_bar_dimensions,
        };

        b8 collided = collision_point_in_aabb(mouse_position, &scroll_bar_aabb);

        if (collided || scroll_bar->dragging)
        {
            quad(&render->vertices, position, scroll_bar_dimensions,
                 bright_color, 0.0f);
            render->index_count++;
        }
        else
        {
            quad_gradiant_t_b(&render->vertices, position,
                              scroll_bar_dimensions, lighter_color, v4ic(0.45f),
                              0.0f);
            render->index_count++;
        }

        if (mouse_button->activated && collided)
        {
            scroll_bar->dragging = true;
            scroll_bar->mouse_pointer_offset =
                scroll_bar_aabb.min.y - mouse_position.y;
        }
        if (mouse_button->mouse_button_event.action == FTIC_RELEASE)
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
    return event->activated &&
           event->mouse_button_event.action == FTIC_RELEASE &&
           event->mouse_button_event.button == button;
}

DirectoryTab tab_add()
{
    DirectoryHistory directory_history = { 0 };
    array_create(&directory_history.history, 10);

    const char* dir = "C:\\*";
    DirectoryPage page = { 0 };
    page.directory = platform_get_directory(dir, (u32)strlen(dir));
    array_push(&directory_history.history, page);

    // TODO(Linus): Make a set for this instead
    SelectedItemValues selected_item_values = { 0 };
    array_create(&selected_item_values.paths, 10);
    selected_item_values.selected_items =
        hash_table_create_char_u32(100, hash_murmur);

    return (DirectoryTab){
        .directory_history = directory_history,
        .selected_item_values = selected_item_values,
    };
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
    const f32 pixel_height = 16;
    const u32 bitmap_size = width_atlas * height_atlas;
    u8* font_bitmap_temp = (u8*)calloc(bitmap_size, sizeof(u8));
    init_ttf_atlas(width_atlas, height_atlas, pixel_height, 96, 32,
                   "res/fonts/arial.ttf", font_bitmap_temp, &application->font);

    // Puts the red channel in the alpha.
    u8* font_bitmap = (u8*)malloc(bitmap_size * 4 * sizeof(u8));
    memset(font_bitmap, UINT8_MAX, bitmap_size * 4 * sizeof(u8));
    for (u32 i = 0, j = 3; i < bitmap_size; ++i, j += 4)
    {
        font_bitmap[j] = font_bitmap_temp[i];
    }

    application->key_event = event_subscribe(KEY);
    application->mouse_button = event_subscribe(MOUSE_BUTTON);
    application->mouse_wheel = event_subscribe(MOUSE_WHEEL);
    application->mouse_move = event_subscribe(MOUSE_MOVE);

    array_create(&application->tabs, 10);
    application->tab_index = 0;

    array_push(&application->tabs, tab_add());

    application->last_time = platform_get_time();
    application->delta_time = 0.0f;
    application->mvp = (MVP){ 0 };
    application->mvp.view = m4d();
    application->mvp.model = m4d();

    application->last_moved_time = window_get_time();

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
    u32 font_texture = texture_create(&texture_properties, GL_RGBA8, GL_RGBA);
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
    u32 default_texture =
        texture_create(&texture_properties, GL_RGBA8, GL_RGBA);

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
    u32* main_textures = (u32*)calloc(main_texture_count, sizeof(u32));
    main_textures[0] = default_texture;
    main_textures[1] = font_texture;
    main_textures[2] = file_icon_texture;
    main_textures[3] = arrow_icon_texture;

    array_push(&rendering_properties,
               rendering_properties_init(main_shader, main_textures,
                                         main_texture_count, index_buffer_id,
                                         &vertex_buffer_layout, 100 * 4));

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
    double x, y;
    window_get_mouse_position(application->window, &x, &y);
    application->mouse_position = v2f((f32)x, (f32)y);
    if (application->mouse_move->activated)
    {
        application->last_moved_time = window_get_time();
    }

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

f64 application_get_last_mouse_move_time(const ApplicationContext* appliction)
{
    return window_get_time() - appliction->last_moved_time;
}

void add_from_key_buffer(const FontTTF* font, const f32 width, i32* input_index,
                         CharArray* buffer)
{
    const CharArray* key_buffer = event_get_key_buffer();
    for (u32 i = 0; i < key_buffer->size; ++i)
    {
        char current_char = key_buffer->data[i];
        if (closed_interval(0, (current_char - 32), 96))
        {
            array_push(buffer, current_char);
            f32 x_advance =
                text_x_advance(font->chars, buffer->data, buffer->size, 1.0f);
            buffer->size--;
            if (x_advance >= width)
            {
                return;
            }
            array_push(buffer, '\0');
            for (i32 j = buffer->size - 1; j > *input_index; --j)
            {
                buffer->data[j] = buffer->data[j - 1];
            }
            buffer->data[(*input_index)++] = current_char;
            array_push(buffer, '\0');
            array_push(buffer, '\0');
            array_push(buffer, '\0');
            buffer->size -= 3;
        }
    }
}

void search_page_parse_key_buffer(SearchPage* page, const f32 search_bar_width,
                                  const FontTTF* font,
                                  const b8 reload_search_result,
                                  DirectoryHistory* directory_history,
                                  ThreadTaskQueue* thread_task_queue)
{
    add_from_key_buffer(font, search_bar_width, &page->search_bar_cursor_index,
                        &page->search_buffer);
    const CharArray* key_buffer = event_get_key_buffer();
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

b8 erase_char(const Event* key_event, i32* cursor_index, CharArray* buffer)
{
    if (key_event->activated &&
        (key_event->key_event.action == FTIC_PRESS ||
         key_event->key_event.action == FTIC_REPEAT) &&
        (key_event->key_event.key == FTIC_KEY_BACKSPACE ||
         (key_event->key_event.ctrl_pressed &&
          key_event->key_event.key == FTIC_KEY_H)))
    {
        if ((*cursor_index) > 0)
        {
            (*cursor_index)--;
            for (u32 i = *cursor_index; i < buffer->size; ++i)
            {
                buffer->data[i] = buffer->data[i + 1];
            }
            *array_back(buffer) = '\0';
            buffer->size--;
            return true;
        }
    }
    return false;
}

void search_page_top_bar_update(SearchPage* page,
                                const ApplicationContext* application,
                                const b8 check_collision,
                                const V2 search_bar_position,
                                const u32 item_count,
                                DirectoryHistory* directory_history,
                                ThreadTaskQueue* thread_task_queue)
{
    const f32 search_bar_input_text_padding = 10.0f;
    const f32 scale = 1.0f;
    const f32 padding_top = 2.0f;
    const f32 quad_height =
        scale * application->font.pixel_height + padding_top * 5.0f;

    V2 search_result_position = search_bar_position;
    search_result_position.y += page->search_bar_aabb.size.y + 20.0f;
    const f32 search_page_header_size_y = search_result_position.y;

    if (application->mouse_button->activated)
    {
        if (check_collision &&
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
        b8 reload_search_result =
            erase_char(application->key_event, &page->search_bar_cursor_index,
                       &page->search_buffer);
        search_page_parse_key_buffer(page,
                                     page->search_bar_aabb.size.width -
                                         (search_bar_input_text_padding * 2),
                                     &application->font, reload_search_result,
                                     directory_history, thread_task_queue);
    }

    V2 search_input_position = page->search_bar_aabb.min;
    search_input_position.x += search_bar_input_text_padding;
    search_input_position.y += scale * application->font.pixel_height + 8.0f;

    quad(&page->render->vertices, page->search_bar_aabb.min,
         page->search_bar_aabb.size, high_light_color, 0.0f);
    page->render->index_count++;

    // Search bar
    page->search_bar_cursor_index = render_input(
        &application->font, page->search_buffer.data, page->search_buffer.size,
        scale, search_input_position, search_bar_position.y + 10.0f,
        page->search_bar_hit, application->key_event,
        page->search_bar_cursor_index, application->delta_time,
        &page->search_blinking_time, NULL, page->render);

    quad_border_rounded(&page->render->vertices, &page->render->index_count,
                        page->search_bar_aabb.min, page->search_bar_aabb.size,
                        border_color, border_width, 0.4f, 3, 0.0f);

    const V2 scroll_bar_position = v2f(application->dimensions.width,
                                       page->search_result_aabb.min.y - 8.0f);
    const f32 area_y = page->search_result_aabb.size.y + 8.0f;
    const f32 total_height = item_count * quad_height;
    scroll_bar_add(&page->scroll_bar, scroll_bar_position,
                   application->mouse_button, application->mouse_position,
                   application->dimensions.y, area_y, total_height, quad_height,
                   &page->scroll_offset, &page->offset, page->render);
}

DirectoryItemListReturnValue
search_page_update(SearchPage* page, const ApplicationContext* application,
                   const b8 check_collision, const V2 search_bar_position,
                   DirectoryHistory* directory_history,
                   ThreadTaskQueue* thread_task_queue,
                   SelectedItemValues* selected_item_values)
{
    const f32 search_bar_width = 250.0f;
    page->search_bar_aabb = (AABB){
        .min = v2f(application->dimensions.width - (search_bar_width + 10.0f),
                   search_bar_position.y),
        .size = v2f(search_bar_width, 40.0f),
    };

    const f32 scale = 1.0f;
    const f32 padding_top = 2.0f;
    const f32 quad_height =
        scale * application->font.pixel_height + padding_top * 5.0f;

    V2 search_result_position = search_bar_position;
    search_result_position.y += page->search_bar_aabb.size.y + 20.0f;
    const f32 search_page_header_size_y = search_result_position.y;

    page->search_result_aabb = (AABB){
        .min = search_result_position,
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

        page->result_pulse_x += application->delta_time * 6.0f;

        search_list_return_value = directory_item_list(
            application, &page->search_result_folder_array.array,
            &page->search_result_file_array.array, check_collision,
            search_result_position, search_result_width, quad_height,
            padding_top, page->result_pulse_x, selected_item_values,
            page->render, directory_history);

        platform_mutex_unlock(&page->search_result_folder_array.mutex);
        platform_mutex_unlock(&page->search_result_file_array.mutex);
    }

    return search_list_return_value;
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

b8 drop_down_menu_add(DropDownMenu* drop_down_menu,
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
        &drop_down_menu->render->vertices, &drop_down_menu->render->index_count,
        v2f(drop_down_menu->position.x - drop_down_border_width,
            drop_down_menu->position.y - drop_down_border_width),
        v2f(drop_down_width + border_extra_padding,
            current_y + border_extra_padding),
        lighter_color, lighter_color, drop_down_border_width, 0.0f);

    b8 item_clicked = false;

    b8 mouse_button_clicked =
        is_mouse_button_clicked(application->mouse_button, FTIC_MOUSE_BUTTON_1);

    if (is_key_pressed_repeat(application->key_event, FTIC_KEY_TAB))
    {
        if (application->key_event->key_event.shift_pressed)
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

    b8 enter_clicked = is_key_clicked(application->key_event, FTIC_KEY_ENTER);

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
                application->mouse_move->activated)
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
        drop_down_menu->render->index_count++;

        V2 promt_item_text_position = promt_item_position;
        promt_item_text_position.y +=
            application->font.pixel_height + promt_item_text_padding + 3.0f;
        promt_item_text_position.x += promt_item_text_padding;

        V4 text_color = v4i(1.0f);
        should_close = drop_down_menu->menu_options_selection(
            i, drop_down_item_hit, should_close, item_clicked, &text_color,
            option_data);

        drop_down_menu->render->index_count += text_generation_color(
            application->font.chars, drop_down_menu->options.data[i], 1.0f,
            promt_item_text_position, 1.0f,
            application->font.pixel_height * precent, text_color, NULL, NULL,
            NULL, &drop_down_menu->render->vertices);

        promt_item_position.y += drop_down_item_aabb.size.y;
    }
    return should_close ||
           ((!any_drop_down_item_hit) && mouse_button_clicked) ||
           is_key_clicked(application->key_event, FTIC_KEY_ESCAPE);
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

    V2 preview_position = v2_s_multi(application->dimensions, 0.5f);
    v2_sub_equal(&preview_position, v2_s_multi(preview_dimensions, 0.5f));

    AABB* scissor = &render->scissor;
    scissor->min = preview_position;
    scissor->min.y = application->dimensions.y - scissor->min.y;
    scissor->min.y -= preview_dimensions.y;
    scissor->size = preview_dimensions;
    scissor->size.width += border_width;
    scissor->size.height += border_width;

    AABB preview_aabb = quad_border_gradiant(
        &render->vertices, &render->index_count, preview_position,
        preview_dimensions, bright_color, lighter_color, border_width, 0.0f);
    v2_s_add_equal(&preview_position, border_width);
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

b8 button_arrow_add(V2 position, const AABB* button_aabb,
                    const V4 texture_coordinates, const b8 check_collision,
                    const b8 extra_condition, const V2 mouse_position,
                    RenderingProperties* render)
{
    V4 button_color = v4ic(1.0f);
    b8 collision = false;
    if (extra_condition)
    {
        if (check_collision &&
            collision_point_in_aabb(mouse_position, button_aabb))
        {
            collision = true;
        }
    }
    else
    {
        button_color = border_color;
    }

    if (collision)
    {
        quad(&render->vertices, position, button_aabb->size, high_light_color,
             0.0f);
        render->index_count++;
    }

    position.x += 9.0f;
    position.y += 10.0f;

    quad_co(&render->vertices, position, v2i(20.0f), button_color,
            texture_coordinates, 3.0f);
    render->index_count++;

    return extra_condition && collision;
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

b8 button_move_in_history_add(const AABB* button_aabb, const b8 check_collision,
                              const b8 extra_condition, const V2 mouse_position,
                              const Event* mouse_button_event,
                              const int mouse_button, const i32 history_add,
                              const V4 icon_co,
                              SelectedItemValues* selected_item_values,
                              DirectoryHistory* directory_history,
                              RenderingProperties* render)
{
    b8 result = false;
    if (button_arrow_add(button_aabb->min, button_aabb, icon_co,
                         check_collision, extra_condition, mouse_position,
                         render))
    {
        if (is_mouse_button_clicked(mouse_button_event, FTIC_MOUSE_BUTTON_LEFT))
        {
            move_in_history(history_add, selected_item_values,
                            directory_history);
        }
        result = true;
    }
    if (check_collision && extra_condition &&
        is_mouse_button_clicked(mouse_button_event, mouse_button))
    {
        move_in_history(history_add, selected_item_values, directory_history);
    }
    return result;
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

b8 suggestion_selection(u32 index, b8 hit, b8 should_close, b8 item_clicked,
                        V4* text_color, void* data)
{
    if (hit)
    {
        SuggestionSelectionData* arguments = (SuggestionSelectionData*)data;
        if (item_clicked || *arguments->tab_index >= 0)
        {
            char* path = arguments->items->data[index].path;
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

f32 middle(const f32 area_size, const f32 object_size)
{
    return (area_size * 0.5f) - (object_size * 0.5f);
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

    CharPtrArray pasted_paths = { 0 };
    array_create(&pasted_paths, 10);

    b8 right_clicked = false;

    DropDownMenu drop_down_menu = {
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

    V2 image_dimensions = v2d();

    ScrollBar main_scroll_bar = { 0 };
    b8 search_scroll_bar_dragging = false;

    DirectoryItemArray quick_access_folders = { 0 };
    array_create(&quick_access_folders, 10);
    quick_access_load(&quick_access_folders);
    f64 quick_access_pulse_x = 0.0f;

    b8 parent_directory_clicked = false;
    f64 parent_directory_clicked_time = 0.4f;
    i32 input_index = -1;
    CharArray parent_directory = { 0 };
    array_create(&parent_directory, 100);
    DropDownMenu suggestions = {
        .tab_index = -1,
        .menu_options_selection = suggestion_selection,
        .render = main_render,
    };
    array_create(&suggestions.options, 10);
    SuggestionSelectionData suggestion_data = {
        .parent_directory = &parent_directory,
        .items = NULL,
        .tab_index = &suggestions.tab_index,
        .cursor_index = &input_index,
    };
    AABBArray parent_directory_aabbs = { 0 };
    array_create(&parent_directory_aabbs, 10);

    V2 starting_position = v2f(150.0f, 0.0f);
    const f32 search_bar_width = 250.0f;
    f32 search_result_width = search_bar_width + 10.0f;

    i32 side_resize = -1;
    b8 resize_dragging = false;
    f32 resize_offset = 0.0f;

    enable_gldebugging();
    glEnable(GL_BLEND);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_MULTISAMPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    while (!window_should_close(application.window))
    {
        if (is_ctrl_and_key_pressed(application.key_event, FTIC_KEY_T))
        {
            array_push(&application.tabs, tab_add());
            application.tab_index = application.tabs.size - 1;
        }

        if (is_ctrl_and_key_range_pressed(application.key_event, FTIC_KEY_1,
                                          FTIC_KEY_9))
        {
            const u32 index = application.key_event->key_event.key - FTIC_KEY_1;
            if (index < application.tabs.size)
            {
                application.tab_index = index;
            }
        }

        DirectoryTab* tab = application.tabs.data + application.tab_index;
        application_begin_frame(&application);

        rendering_properties_array_clear(&rendering_properties);
        main_render->scissor.size = application.dimensions;

        b8 check_collision = preview_render->texture_count == 1;
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
            const MouseButtonEvent* event =
                &application.mouse_button->mouse_button_event;

            if (is_ctrl_and_key_pressed(application.key_event, FTIC_KEY_C))
            {
                platform_copy_to_clipboard(&tab->selected_item_values.paths);
            }
            else if (tab->selected_item_values.paths.size == 1 &&
                     is_ctrl_and_key_pressed(application.key_event, FTIC_KEY_D))
            {
                if (preview_render->texture_count > 1)
                {
                    texture_delete(
                        preview_render
                            ->textures[--preview_render->texture_count]);
                }
                const char* path = tab->selected_item_values.paths.data[0];
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
                    reset_selected_items(&tab->selected_item_values);
                }
            }
            else if (application.key_event->activated &&
                     application.key_event->key_event.action == 1 &&
                     application.key_event->key_event.key == FTIC_KEY_ESCAPE)
            {
                // TODO: Not get called when you press escape in a drop down for
                // example
                // platform_delete_files(&tab->selected_item_values.paths);
                // reload_directory(
                //   current_directory(&application.directory_history));
            }
            else if (check_collision &&
                     !application.key_event->key_event.ctrl_pressed &&
                     application.mouse_button->activated &&
                     event->action == 0 && event->button == FTIC_MOUSE_BUTTON_1)
            {
                reset_selected_items(&tab->selected_item_values);
            }
        }
        if (is_ctrl_and_key_pressed(application.key_event, FTIC_KEY_V))
        {
            paste_in_directory(current_directory(&tab->directory_history));
        }

        const f32 top_bar_height = 60.0f;
        const f32 tab_height = 45.0f;

        AABB rect = quad(
            &main_render->vertices, v2f(starting_position.x, top_bar_height),
            v2f(border_width, application.dimensions.y - top_bar_height),
            border_color, 0.0f);
        main_render->index_count++;

        AABB right_border_aabb = {
            .min = v2f(application.dimensions.x - search_result_width - 10.0f,
                       top_bar_height),
            .size =
                v2f(border_width, application.dimensions.y - top_bar_height),
        };

        AABB resize_aabb = rect;
        resize_aabb.min.x -= 2.0f;
        resize_aabb.size.x += 4.0f;

        b8 left_side_resize =
            collision_point_in_aabb(application.mouse_position, &resize_aabb);

        resize_aabb = right_border_aabb;
        resize_aabb.min.x -= 2.0f;
        resize_aabb.size.x += 4.0f;
        b8 right_side_resize =
            collision_point_in_aabb(application.mouse_position, &resize_aabb);

        if (left_side_resize || right_side_resize)
        {
            window_set_cursor(application.window, FTIC_RESIZE_H_CURSOR);
            if (application.mouse_button->activated)
            {
                if (left_side_resize)
                {
                    resize_offset = rect.min.x - application.mouse_position.x;
                    side_resize = 0;
                }
                else
                {
                    resize_offset =
                        right_border_aabb.min.x - application.mouse_position.x;
                    side_resize = 1;
                }
                resize_dragging = true;
            }
        }
        else if (!resize_dragging)
        {
            window_set_cursor(application.window, FTIC_NORMAL_CURSOR);
        }
        if (application.mouse_button->mouse_button_event.action == FTIC_RELEASE)
        {
            resize_dragging = false;
        }
        if (resize_dragging)
        {
            if (side_resize == 0)
            {
                starting_position.x =
                    application.mouse_position.x + resize_offset;

                starting_position.x = max(150.0f, starting_position.x);
                starting_position.x =
                    min(right_border_aabb.min.x - 200.0f, starting_position.x);
            }
            else
            {
                search_result_width = application.dimensions.x -
                                      application.mouse_position.x +
                                      resize_offset;
                search_result_width -= 10.0f;
                search_result_width = max(150.0f, search_result_width);
                search_result_width =
                    min(application.dimensions.x - rect.min.x - 200.0f,
                        search_result_width);
            }
            check_collision = false;
        }
        V2 search_bar_position = v2f(
            application.dimensions.x - search_result_width,
            middle(top_bar_height, search_page.search_bar_aabb.size.height));

        const f32 back_drop_height = 40.0f;

        V2 parent_directory_path_position =
            v2f(150.0f, middle(top_bar_height, back_drop_height));

        V2 text_starting_position;
        text_starting_position.x = rect.min.x + border_width + 10.0f;
        text_starting_position.y = top_bar_height + border_width;
        text_starting_position.y += tab_height * (application.tabs.size > 1);

        const f32 width =
            ((search_bar_position.x - 10.0f) - text_starting_position.x);

        AABB directory_aabb = {
            .min = text_starting_position,
            .size =
                v2f(width, application.dimensions.y - text_starting_position.y),
        };
        text_starting_position.y += 8.0f;

        AABB side_under_border_aabb = {
            .min = v2f(0.0f, top_bar_height),
            .size = v2f(application.dimensions.x, border_width),
        };

        const f32 scale = 1.0f;
        const f32 padding_top = 2.0f;
        const f32 quad_height =
            scale * application.font.pixel_height + padding_top * 5.0f;

        quick_access_pulse_x += application.delta_time * 6.0f;
        DirectoryItemListReturnValue quick_access_list_return_value =
            folder_item_list(&application, &quick_access_folders,
                             check_collision,
                             v2f(10.0f, side_under_border_aabb.min.y + 8.0f),
                             rect.min.x - 10.0f, quad_height, padding_top,
                             quick_access_pulse_x, &tab->selected_item_values,
                             main_render, &tab->directory_history);

        text_starting_position.y +=
            current_directory(&tab->directory_history)->scroll_offset;

        DirectoryItemListReturnValue main_list_return_value;
        {
            DirectoryPage* current = current_directory(&tab->directory_history);
            current->pulse_x += application.delta_time * 6.0f;

            main_list_return_value = directory_item_list(
                &application, &current->directory.sub_directories,
                &current->directory.files, check_collision,
                text_starting_position, width - 10.0f, quad_height, padding_top,
                current->pulse_x, &tab->selected_item_values, main_render,
                &tab->directory_history);
        }

        AABB parent_directory_path_aabb = { 0 };
        parent_directory_path_aabb.min = parent_directory_path_position;
        parent_directory_path_aabb.size =
            v2f(search_page.search_bar_aabb.min.x -
                    parent_directory_path_aabb.min.x - 10.0f,
                back_drop_height);

        if (is_mouse_button_clicked(application.mouse_button,
                                    FTIC_MOUSE_BUTTON_LEFT))
        {
            if (collision_point_in_aabb(application.mouse_position,
                                        &parent_directory_path_aabb))
            {
                if (!parent_directory_clicked)
                {
                    parent_directory_clicked_time = 0.4f;
                    parent_directory.size = 0;
                    const char* path =
                        current_directory(&tab->directory_history)
                            ->directory.parent;
                    const u32 parent_length = (u32)strlen(path);
                    for (u32 i = 0; i < parent_length; ++i)
                    {
                        array_push(&parent_directory, path[i]);
                    }
                    array_push(&parent_directory, '\0');
                    array_push(&parent_directory, '\0');
                    array_push(&parent_directory, '\0');
                    parent_directory.size -= 3;
                    parent_directory_clicked = true;
                }
            }
            else if (!collision_point_in_aabb(application.mouse_position,
                                              &suggestions.aabb))
            {
                suggestions.aabb = (AABB){ 0 };
                suggestions.x = 0;
                parent_directory_clicked = false;
                parent_directory_clicked_time = 0.4f;
                parent_directory.size = 0;
            }
        }

        DirectoryItemListReturnValue search_list_return_value =
            search_page_update(&search_page, &application, check_collision,
                               search_bar_position, &tab->directory_history,
                               &thread_queue.task_queue,
                               &tab->selected_item_values);

        quad_gradiant_t_b(&main_render->vertices, v2d(),
                          v2f(application.dimensions.width, top_bar_height),
                          v4ic(0.16f), v4ic(0.14f), 0.0f);
        main_render->index_count++;

        search_page_top_bar_update(
            &search_page, &application, check_collision, search_bar_position,
            search_list_return_value.count, &tab->directory_history,
            &thread_queue.task_queue);

        quad(&main_render->vertices, parent_directory_path_aabb.min,
             parent_directory_path_aabb.size, high_light_color, 0.0f);
        main_render->index_count++;

        quad_border_rounded(&main_render->vertices, &main_render->index_count,
                            parent_directory_path_aabb.min,
                            parent_directory_path_aabb.size, border_color,
                            border_width, 0.4f, 3, 0.0f);

        parent_directory_path_position.y += application.font.pixel_height;
        parent_directory_path_position.y +=
            middle(back_drop_height, application.font.pixel_height) - 3.0f;
        parent_directory_path_position.x += 10.0f;

        quad(&main_render->vertices, right_border_aabb.min,
             right_border_aabb.size, border_color, 0.0f);
        main_render->index_count++;

        AABB button_aabb = {
            .min = v2i(10.0f),
            .size = v2i(search_page.search_bar_aabb.size.y),
        };
        button_aabb.min.y = middle(top_bar_height, button_aabb.size.height);

        main_list_return_value.hit |= button_move_in_history_add(
            &button_aabb, check_collision,
            tab->directory_history.current_index > 0,
            application.mouse_position, application.mouse_button,
            FTIC_MOUSE_BUTTON_4, -1, arrow_back_icon_co,
            &tab->selected_item_values, &tab->directory_history, main_render);

        button_aabb.min.x += button_aabb.size.x;

        b8 extra_condition = tab->directory_history.history.size >
                             tab->directory_history.current_index + 1;
        main_list_return_value.hit |= button_move_in_history_add(
            &button_aabb, check_collision, extra_condition,
            application.mouse_position, application.mouse_button,
            FTIC_MOUSE_BUTTON_5, 1, arrow_right_icon_co,
            &tab->selected_item_values, &tab->directory_history, main_render);

        button_aabb.min.x += button_aabb.size.x;

        if (button_arrow_add(button_aabb.min, &button_aabb, arrow_up_icon_co,
                             check_collision,
                             can_go_up_one_directory(
                                 current_directory(&tab->directory_history)
                                     ->directory.parent),
                             application.mouse_position, main_render))
        {
            if (is_mouse_button_clicked(application.mouse_button,
                                        FTIC_MOUSE_BUTTON_LEFT))
            {
                go_up_one_directory(&tab->directory_history);
            }
        }

        V2 back_button_border_position =
            v2f(0.0f, side_under_border_aabb.min.y);

        quad(&main_render->vertices, side_under_border_aabb.min,
             side_under_border_aabb.size, border_color, 0.0f);
        main_render->index_count++;

        V2 scroll_bar_position =
            v2f(right_border_aabb.min.x, directory_aabb.min.y);
        f32 area_y = directory_aabb.size.y;
        f32 total_height = main_list_return_value.count * quad_height;
        scroll_bar_add(
            &main_scroll_bar, scroll_bar_position, application.mouse_button,
            application.mouse_position, application.dimensions.y, area_y,
            total_height, quad_height,
            &current_directory(&tab->directory_history)->scroll_offset,
            &current_directory(&tab->directory_history)->offset, main_render);

        if (is_mouse_button_clicked(application.mouse_button,
                                    FTIC_MOUSE_BUTTON_2))
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

        const u32 tab_count = application.tabs.size;
        if (tab_count > 1)
        {
            V2 tab_position =
                v2f(rect.min.x + border_width, top_bar_height + border_width);

            const f32 area_width = right_border_aabb.min.x - tab_position.x;
            const f32 max_tab_width = 200.0f;
            const f32 padding = 5.0f;
            const f32 tab_width =
                min(max_tab_width,
                    ((area_width - 5.0f) - (padding * tab_count)) / tab_count);

            quad(&main_render->vertices, tab_position,
                 v2f(area_width, tab_height), clear_color, 0.0f);
            main_render->index_count++;

            quad(&main_render->vertices,
                 v2f(tab_position.x, tab_position.y + tab_height),
                 v2f(area_width, border_width), border_color, 0.0f);
            main_render->index_count++;

            tab_position.y += 5.0f;
            tab_position.x += 5.0f;

            for (u32 i = 0; i < tab_count; ++i)
            {
                AABB tab_aabb = {
                    .min = tab_position,
                    .size = v2f(tab_width, tab_height - 5.0f),
                };

                V4 tab_color = high_light_color;
                if (collision_point_in_aabb(application.mouse_position,
                                            &tab_aabb))
                {
                    tab_color = v4ic(0.3f);
                    if (is_mouse_button_clicked(application.mouse_button,
                                                FTIC_MOUSE_BUTTON_LEFT))
                    {
                        application.tab_index = i;
                    }
                }

                if (application.tab_index == i)
                {
                    tab_color = v4ic(0.4f);
                }

                quad(&main_render->vertices, tab_aabb.min, tab_aabb.size,
                     tab_color, 0.0f);
                main_render->index_count++;
                quad_border_rounded(&main_render->vertices,
                                    &main_render->index_count, tab_aabb.min,
                                    tab_aabb.size, lighter_color, border_width,
                                    0.4f, 3, 0.0f);

                char* parent = current_directory(
                                   &application.tabs.data[i].directory_history)
                                   ->directory.parent;
                main_render->index_count += text_generation(
                    application.font.chars,
                    parent + get_path_length(parent, (u32)strlen(parent)), 1.0f,
                    v2f(tab_position.x + 10.0f,
                        tab_position.y + application.font.pixel_height + 9.0f),
                    scale, application.font.pixel_height, NULL, NULL, NULL,
                    &main_render->vertices);

                tab_position.x += tab_width + padding;
            }
        }

        /*
        if (quick_access_list_return_value.hit || main_list_return_value.hit ||
            search_list_return_value.hit)
        {
            window_set_cursor(application.window, FTIC_HAND_CURSOR);
        }
        */

        if (parent_directory_clicked)
        {
            b8 backspace_pressed = erase_char(application.key_event,
                                              &input_index, &parent_directory);
            if (backspace_pressed)
            {
                suggestions.tab_index = -1;
            }
            b8 reload_results = backspace_pressed;
            add_from_key_buffer(&application.font,
                                parent_directory_path_aabb.size.x, &input_index,
                                &parent_directory);
            const CharArray* key_buffer = event_get_key_buffer();
            reload_results |= key_buffer->size > 0;

            if (reload_results)
            {

                DirectoryPage* current =
                    current_directory(&tab->directory_history);

                char* path = parent_directory.data;
                u32 current_directory_len =
                    get_path_length(path, parent_directory.size) - 1;

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
                    directory =
                        platform_get_directory(path, current_directory_len);
                    path[current_directory_len--] = saved_chars[2];
                    path[current_directory_len--] = saved_chars[1];

                    suggestions.tab_index = -1;
                }
                path[current_directory_len] = saved_chars[0];

                // TODO: Make it not case sensitive
                for (u32 i = 0; i < directory.sub_directories.size; ++i)
                {
                    DirectoryItem* dir_item =
                        directory.sub_directories.data + i;
                    dir_item->size = string_span_case_insensitive(
                        dir_item->name, path + current_directory_len);
                }
                directory_sort_by_size(&directory.sub_directories);
                directory_flip_array(&directory.sub_directories);

                suggestions.options.size = 0;
                const u32 item_count = min(directory.sub_directories.size, 6);
                for (u32 i = 0; i < item_count; ++i)
                {
                    array_push(&suggestions.options,
                               directory.sub_directories.data[i].name);
                }
                suggestion_data.items = &directory.sub_directories;

                const f32 x_advance = text_x_advance(
                    application.font.chars, parent_directory.data,
                    parent_directory.size, scale);

                suggestions.position =
                    v2f(parent_directory_path_position.x + x_advance,
                        parent_directory_path_position.y + 5.0f);
            }
            suggestion_data.change_directory = false;
            if (drop_down_menu_add(&suggestions, &application,
                                   &suggestion_data))
            {
            }

            input_index = render_input(
                &application.font, parent_directory.data, parent_directory.size,
                scale, parent_directory_path_position,
                parent_directory_path_position.y -
                    application.font.pixel_height,
                true, application.key_event, input_index,
                application.delta_time, &parent_directory_clicked_time,
                &parent_directory_aabbs, main_render);
            if (suggestion_data.change_directory ||
                is_key_clicked(application.key_event, FTIC_KEY_ENTER))
            {
                char* last_char = array_back(&parent_directory);
                char saved = *last_char;
                b8 should_restore = *last_char == '\\' || *last_char == '/';
                if (should_restore)
                {
                    *last_char = '\0';
                    parent_directory.size--;
                }
                if (!go_to_directory(parent_directory.data,
                                     parent_directory.size,
                                     &tab->directory_history))
                {
                }
                if (should_restore)
                {
                    *last_char = saved;
                    parent_directory.size++;
                }
                input_index = parent_directory.size;
                suggestions.tab_index = -1;
                suggestions.options.size = 0;
            }
        }
        else
        {
            parent_directory_aabbs.size = 0;
            main_render->index_count += text_generation(
                application.font.chars,
                current_directory(&tab->directory_history)->directory.parent,
                1.0f, parent_directory_path_position, scale,
                application.font.pixel_height, NULL, NULL,
                &parent_directory_aabbs, &main_render->vertices);
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

        DirectoryPage* current = current_directory(&tab->directory_history);
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

    quick_access_save(&quick_access_folders);

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
