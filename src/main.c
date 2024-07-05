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

u32 load_icon_as_only_red(const char* file_path)
{
    TextureProperties texture_properties = { 0 };
    texture_load(file_path, &texture_properties);
    extract_only_aplha_channel(&texture_properties);
    u32 icon_texture = texture_create(&texture_properties, GL_RGBA8, GL_RED);
    free(texture_properties.bytes);
    return icon_texture;
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

void add_move_in_history_button(const AABB* aabb, const V4 icon_co,
                                const b8 disable, const int mouse_button,
                                const i32 history_add,
                                SelectedItemValues* selected_item_values,
                                DirectoryHistory* directory_history)
{
    if (ui_window_add_icon_button(aabb->min, aabb->size, icon_co, 3.0f,
                                  disable))
    {
        directory_move_in_history(history_add, selected_item_values,
                                  directory_history);
    }
    if (!disable && is_mouse_button_clicked(mouse_button))
    {
        directory_move_in_history(history_add, selected_item_values,
                                  directory_history);
    }
}

void look_for_dropped_files(DirectoryPage* current, const char* directory_path)
{
    const CharPtrArray* dropped_paths = event_get_drop_buffer();
    if (dropped_paths->size)
    {
        platform_paste_to_directory(dropped_paths,
                                    directory_path ? directory_path
                                                   : current->directory.parent);
        directory_reload(current);
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
            DirectoryPage* current = directory_current(&tab->directory_history);
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
            directory_reload(directory_current(&tab->directory_history));
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
            directory_open_folder(
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
    DirectoryPage* current = directory_current(&tab->directory_history);
    ui_window_begin(window, true);
    {
        V2 list_position = v2f(10.0f, 10.0f);
        i32 selected_item = -1;
        if (ui_window_add_folder_list(list_position, list_item_height,
                                      &current->directory.sub_directories,
                                      &tab->selected_item_values,
                                      &selected_item))
        {
            directory_open_folder(
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
    ApplicationContext app = { 0 };
    u8* font_bitmap = application_initialize(&app);

    V2 preview_image_dimensions = v2d();
    char* current_path = NULL;
    b8 show_preview = false;
    FileAttrib preview_file = { 0 };
    U32Array preview_textures = { 0 };
    array_create(&preview_textures, 10);

    AABBArray parent_directory_aabbs = { 0 };
    array_create(&parent_directory_aabbs, 10);

    const f32 search_bar_width = 250.0f;
    b8 activated = false;
    V2 last_mouse_position = v2d();
    f32 distance = 0.0f;

    char* menu_options[] = { "Menu", "Windows" };
    CharPtrArray menu_values = { 0 };
    array_create(&menu_values, 10);
    array_push(&menu_values, menu_options[0]);
    array_push(&menu_values, menu_options[1]);

    enable_gldebugging();
    glEnable(GL_BLEND);
    glEnable(GL_MULTISAMPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    while (!window_should_close(app.window))
    {
        application_begin_frame(&app);

        DirectoryTab* tab = app.tabs.data + app.tab_index;

        const MouseButtonEvent* mouse_button_event =
            event_get_mouse_button_event();
        const MouseMoveEvent* mouse_move_event = event_get_mouse_move_event();
        const KeyEvent* key_event = event_get_key_event();

        if (mouse_button_event->activated)
        {
            last_mouse_position = app.mouse_position;
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
            distance = v2_distance(last_mouse_position, app.mouse_position);
            if (distance >= 10.0f)
            {
                // platform_start_drag_drop(&tab->selected_item_values.paths);
                activated = false;
            }
        }
        b8 check_collision = true;

        if (check_collision &&
            (collision_point_in_aabb(app.mouse_position,
                                     &app.context_menu.aabb) ||
             collision_point_in_aabb(app.mouse_position,
                                     &app.suggestions.aabb)))
        {
            check_collision = false;
        }

        if (tab->selected_item_values.paths.size)
        {
            if (is_ctrl_and_key_pressed(FTIC_KEY_C))
            {
                // TODO: this is used in input fields
                //platform_copy_to_clipboard(&tab->selected_item_values.paths);
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
                    directory_reset_selected_items(&tab->selected_item_values);
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
                // directory_reload(
                //   directory_current(&application.directory_history));
            }
            else if (check_collision && !key_event->ctrl_pressed &&
                     mouse_button_event->activated &&
                     mouse_button_event->action == 0 &&
                     mouse_button_event->button == FTIC_MOUSE_BUTTON_1)
            {
                directory_reset_selected_items(&tab->selected_item_values);
            }
        }
        if (is_ctrl_and_key_pressed(FTIC_KEY_V))
        {
            // TODO: this is used in input fields
            //directory_paste_in_directory(
             //   directory_current(&tab->directory_history));
        }
        if (is_mouse_button_clicked(FTIC_MOUSE_BUTTON_2))
        {
            app.context_menu_open = true;
            app.context_menu.position = app.mouse_position;
            app.context_menu.position.x += 18.0f;
            app.context_menu.x = 0.0f;
        }
        if (app.context_menu_open)
        {
            MainDropDownSelectionData data = {
                .quick_access = &app.quick_access_folders,
                .directory = directory_current(&tab->directory_history),
                .selected_paths = &tab->selected_item_values.paths,
            };
            app.context_menu_open =
                !drop_down_menu_add(&app.context_menu, &app, &data);
        }
        else
        {
            app.context_menu.aabb = (AABB){ 0 };
        }
        const char* directory_to_drop_in = NULL;
#if 0
        if (main_list_return_value.hit_index >= 0 &&
            main_list_return_value.type == TYPE_FOLDER)
        {
            directory_to_drop_in = directory_current(&tab->directory_history)
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
        look_for_dropped_files(directory_current(&tab->directory_history),
                               directory_to_drop_in);

        const f32 top_bar_height = 52.0f;
        const f32 top_bar_menu_height = 10.0f + app.font.pixel_height;

        AABB dock_space = { .min = v2f(0.0f,
                                       top_bar_height + top_bar_menu_height) };
        dock_space.size = v2_sub(app.dimensions, dock_space.min);
        ui_context_begin(app.dimensions, &dock_space, app.delta_time,
                         check_collision);
        {

            UiWindow* top_bar = ui_window_get(app.top_bar_window);
            top_bar->position = v2d();
            top_bar->size =
                v2f(app.dimensions.width, top_bar_height + top_bar_menu_height);
            top_bar->top_color = v4ic(0.2f);
            top_bar->bottom_color = v4ic(0.15f);

            ui_window_begin(app.top_bar_window, false);
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

                disable = !directory_can_go_up(
                    directory_current(&tab->directory_history)
                        ->directory.parent);
                if (ui_window_add_icon_button(button_aabb.min, button_aabb.size,
                                              arrow_up_icon_co, 3.0f, disable))
                {
                    directory_go_up(&tab->directory_history);
                }

                button_aabb.min.x += ui_window_row_end() + 10.0f;
                button_aabb.size.x =
                    (app.dimensions.width - (search_bar_width + 20.0f)) -
                    button_aabb.min.x;

                b8 active_before = app.parent_directory_input.active;
                if (ui_window_add_input_field(button_aabb.min, button_aabb.size,
                                              &app.parent_directory_input))
                {
                    DirectoryPage* current =
                        directory_current(&tab->directory_history);
                    get_suggestions(&app.font, button_aabb.min, current,
                                    &app.parent_directory_input,
                                    &app.suggestions, &app.suggestion_data);
                }

                if (app.parent_directory_input.active)
                {
                    if (!active_before)
                    {
                        const char* path =
                            directory_current(&tab->directory_history)
                                ->directory.parent;
                        set_input_buffer_to_current_directory(
                            path, &app.parent_directory_input);
                    }
                    app.suggestion_data.change_directory = false;
                    drop_down_menu_add(&app.suggestions, &app, &app.suggestion_data);

                    if (app.suggestion_data.change_directory ||
                        is_key_clicked(FTIC_KEY_ENTER))
                    {
                        char* last_char =
                            array_back(&app.parent_directory_input.buffer);
                        char saved = *last_char;
                        b8 should_restore =
                            *last_char == '\\' || *last_char == '/';
                        if (should_restore)
                        {
                            *last_char = '\0';
                            app.parent_directory_input.buffer.size--;
                        }
                        if (!directory_go_to(app.parent_directory_input.buffer.data,
                                             app.parent_directory_input.buffer.size,
                                             &tab->directory_history))
                        {
                        }
                        if (should_restore)
                        {
                            *last_char = saved;
                            app.parent_directory_input.buffer.size++;
                        }
                        app.parent_directory_input.input_index =
                            app.parent_directory_input.buffer.size;
                        app.suggestions.tab_index = -1;
                        app.suggestions.options.size = 0;
                    }
                }
                else
                {
                    const char* path =
                        directory_current(&tab->directory_history)
                            ->directory.parent;
                    set_input_buffer_to_current_directory(
                        path, &app.parent_directory_input);
                }
                if (is_mouse_button_clicked(FTIC_MOUSE_BUTTON_LEFT))
                {
                    if (!collision_point_in_aabb(app.mouse_position,
                                                 &app.suggestions.aabb))
                    {
                        app.suggestions.aabb = (AABB){ 0 };
                        app.suggestions.x = 0;
                    }
                }

                button_aabb.min.x += button_aabb.size.x + 10.0f;
                button_aabb.size.x = search_bar_width;
                if (ui_window_add_input_field(button_aabb.min, button_aabb.size,
                                              &app.search_page.input))
                {
                    search_page_search(&app.search_page, &tab->directory_history,
                                       &app.thread_queue.task_queue);
                }
            }
            ui_window_end(NULL, false);

            const f32 list_item_height = app.font.pixel_height + 10.0f;

            ui_window_begin(app.quick_access_window, true);
            {
                V2 list_position = v2i(10.0f);
                i32 selected_item = -1;
                if (ui_window_add_folder_list(list_position, list_item_height,
                                              &app.quick_access_folders, NULL,
                                              &selected_item))
                {
                    directory_open_folder(
                        app.quick_access_folders.data[selected_item].path,
                        &tab->directory_history);
                }
            }
            ui_window_end("Quick access", false);

            show_search_result_window(&app.search_page, app.search_result_window,
                                      list_item_height,
                                      &tab->directory_history);

            if (show_preview)
            {
                if (preview_textures.size > 0)
                {
                    UiWindow* preview = ui_window_get(app.preview_window);

                    V2 image_dimensions = load_and_scale_preview_image(
                        &app, &preview_image_dimensions, &current_path,
                        &directory_current(&tab->directory_history)
                             ->directory.files,
                        &preview_textures);

                    preview->size = image_dimensions;
                    preview->position = v2f(
                        middle(app.dimensions.width, preview->size.width),
                        middle(app.dimensions.height, preview->size.height));

                    ui_window_begin(app.preview_window, false);
                    {
                        show_preview = !ui_window_set_overlay();
                        ui_window_add_image(v2d(), image_dimensions,
                                            preview_textures.data[0]);
                    }
                    ui_window_end(NULL, false);
                }
                else
                {
                    UiWindow* preview = ui_window_get(app.preview_window);
                    preview->size = v2f(500.0f, app.dimensions.height * 0.9f);
                    preview->position = v2f(
                        middle(app.dimensions.width, preview->size.width),
                        middle(app.dimensions.height, preview->size.height));

                    ui_window_begin(app.preview_window, false);
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

            for (u32 i = 0; i < app.tabs.size; ++i)
            {
                const u32 window_id = app.tabs.data[i].window_id;
                if (ui_window_in_focus() == window_id)
                {
                    if (i != app.tab_index)
                    {
                        directory_reset_selected_items(&tab->selected_item_values);
                    }
                    app.tab_index = i;
                }
                if (show_directory_window(window_id, list_item_height,
                                          app.tabs.data + i))
                {
                    DirectoryTab tab_to_remove = app.tabs.data[i];
                    for (u32 j = i; j < app.tabs.size - 1; ++j)
                    {
                        app.tabs.data[j] = app.tabs.data[j + 1];
                    }
                    directory_tab_clear(&tab_to_remove);
                    app.tabs.size--;
                    array_push(&app.free_window_ids, window_id);

                    // NOTE: this is probably always true
                    if (i == app.tab_index)
                    {
                        app.tab_index = (app.tab_index + 1) % app.tabs.size;
                    }
                    --i;
                }
            }

            b8 search_result_open = search_page_has_result(&app.search_page) &&
                                    app.search_page.input.buffer.size;
            if (!search_result_open)
            {
                search_page_clear_result(&app.search_page);
            }
        }
        ui_context_end();

        rendering_properties_check_and_grow_buffers(&app.main_render,
                                                    app.main_index_count);

        buffer_set_sub_data(app.main_render.vertex_buffer_id, GL_ARRAY_BUFFER, 0,
                            sizeof(Vertex) * app.main_render.vertices.size,
                            app.main_render.vertices.data);

        AABB whole_screen_scissor = { .size = app.dimensions };

        rendering_properties_begin_draw(&app.main_render, &app.mvp);
        rendering_properties_draw(0, app.main_index_count, &whole_screen_scissor);
        rendering_properties_end_draw(&app.main_render);
#endif

        application_end_frame(&app);
    }
    if (current_path) free(current_path);

    // TODO: Cleanup of all
    application_uninitialize(&app);
}
