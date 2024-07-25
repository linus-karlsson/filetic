#include "application.h"
#include "hash.h"
#include "logging.h"
#include "texture.h"
#include "opengl_util.h"
#include "camera.h"
#include "object_load.h"
#include "random.h"
#include "globals.h"
#include <stdio.h>
#include <string.h>
#include <glad/glad.h>
#include <math.h>

global const V4 full_icon_co = {
    .x = 0.0f,
    .y = 0.0f,
    .z = 1.0f,
    .w = 1.0f,
};

#define icon_1_co(width, height)                                               \
    {                                                                          \
        .x = 0.0f / width,                                                     \
        .y = 0.0f / height,                                                    \
        .z = height / width,                                                   \
        .w = height / height,                                                  \
    };

global const V4 file_icon_co = icon_1_co(512.0f, 128.0f);
global const V4 arrow_back_icon_co = icon_1_co(72.0f, 24.0f);

#define icon_2_co(width, height)                                               \
    {                                                                          \
        .x = height / width,                                                   \
        .y = 0.0f / height,                                                    \
        .z = (height * 2.0f) / width,                                          \
        .w = height / height,                                                  \
    };
global const V4 folder_icon_co = icon_2_co(512.0f, 128.0f);
global const V4 arrow_up_icon_co = icon_2_co(72.0f, 24.0f);

#define icon_3_co(width, height)                                               \
    {                                                                          \
        .x = (height * 2.0f) / width,                                          \
        .y = 0.0f / height,                                                    \
        .z = (height * 3.0f) / width,                                          \
        .w = height / height,                                                  \
    };
global const V4 pdf_icon_co = icon_3_co(512.0f, 128.0f);
global const V4 arrow_right_icon_co = icon_3_co(72.0f, 24.0f);

#define icon_4_co(width, height)                                               \
    {                                                                          \
        .x = (height * 3.0f) / width,                                          \
        .y = 0.0f / height,                                                    \
        .z = (height * 4.0f) / width,                                          \
        .w = height / height,                                                  \
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

internal void set_gldebug_log_level(DebugLogLevel level)
{
    g_debug_log_level = level;
}

internal void opengl_log_message(GLenum source, GLenum type, GLuint id,
                                 GLenum severity, GLsizei length,
                                 const GLchar* message, const void* userParam)
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

internal void enable_gldebugging()
{
    glDebugMessageCallback(opengl_log_message, NULL);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
}

internal b8 load_preview_image(const char* path, V2* image_dimensions,
                               U32Array* textures)
{
    b8 result = false;
    const char* extension = file_get_extension(path, (u32)strlen(path));
    if (extension && (!strcmp(extension, "png") || !strcmp(extension, "jpg")))
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
        u32 texture =
            texture_create(&texture_properties, GL_RGBA8, GL_RGBA, GL_LINEAR);
        array_push(textures, texture);
        free(texture_properties.bytes);
        result = true;
    }
    return result;
}

internal b8 check_and_load_image(const i32 index, V2* image_dimensions,
                                 char** current_path,
                                 const DirectoryItemArray* files,
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

internal V2 load_and_scale_preview_image(const ApplicationContext* application,
                                         V2* image_dimensions,
                                         char** current_path,
                                         const DirectoryItemArray* files,
                                         U32Array* textures)
{
    b8 right_key = event_is_key_pressed_repeat(FTIC_KEY_RIGHT);
    b8 left_key = event_is_key_pressed_repeat(FTIC_KEY_LEFT);
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

internal void look_for_dropped_files(DirectoryPage* current,
                                     const char* directory_path)
{
    const CharPtrArray* dropped_paths = event_get_drop_buffer();
    if (dropped_paths->size)
    {
        platform_paste_to_directory(dropped_paths,
                                    directory_path ? directory_path
                                                   : current->directory.parent);
    }
}

internal void drag_drop_callback(void* data)
{
    CharPtrArray* selected_paths = (CharPtrArray*)data;
    platform_start_drag_drop(selected_paths);
}

internal void set_sorting_buttons(const SortBy sort_by, DirectoryTab* tab)
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

internal void
add_move_in_history_button(const AABB* aabb, const V4 icon_co, const b8 disable,
                           const int mouse_button, const i32 history_add,
                           SelectedItemValues* selected_item_values,
                           DirectoryHistory* directory_history)
{
    if (ui_window_add_icon_button(aabb->min, aabb->size,
                                  global_get_highlight_color(), icon_co,
                                  UI_ARROW_ICON_TEXTURE, disable))
    {
        directory_move_in_history(history_add, selected_item_values,
                                  directory_history);
    }
    if (!disable && event_is_mouse_button_clicked(mouse_button))
    {
        directory_move_in_history(history_add, selected_item_values,
                                  directory_history);
    }
}

internal b8 show_search_result_window(SearchPage* page, const u32 window,
                                      const f32 list_item_height,
                                      DirectoryHistory* directory_history)
{
    if (ui_window_begin(window, "Search result",
                        UI_WINDOW_TOP_BAR | UI_WINDOW_RESIZEABLE))
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

        return ui_window_end();
    }
    return false;
}

internal void recent_panel_add_item(RecentPanel* recent, const char* path_temp)
{
    if (platform_directory_exists(path_temp))
    {
        DirectoryItemArray* items = &recent->panel.items;
        for (u32 i = 0; i < items->size; ++i)
        {
            if (!strcmp(path_temp, items->data[i].path))
            {
                DirectoryItem temp = items->data[i];
                for (i32 j = i; j >= 1; --j)
                {
                    items->data[j] = items->data[j - 1];
                }
                items->data[0] = temp;
                return;
            }
        }
        const u32 length = (u32)strlen(path_temp);
        char* path = (char*)calloc(length + 1, sizeof(char));
        memcpy(path, path_temp, length);
        DirectoryItem item = {
            .path = path,
            .name = path + get_path_length(path, length),
        };
        if (items->size == recent->total)
        {
            free(array_back(items)->path);
            items->size--;
        }
        array_push(items, item);
        for (i32 i = (i32)items->size - 1; i >= 1; --i)
        {
            items->data[i] = items->data[i - 1];
        }
        items->data[0] = item;
    }
}

internal char* get_parent_directory_name(DirectoryPage* current)
{
    return current->directory.parent +
           get_path_length(current->directory.parent,
                           (u32)strlen(current->directory.parent));
}

internal void tab_clear_selected(DirectoryTab* tab)
{
    if (tab->directory_list.selected_item_values.last_selected)
    {
        free(tab->directory_list.selected_item_values.last_selected);
        tab->directory_list.selected_item_values.last_selected = NULL;
    }
    directory_clear_selected_items(&tab->directory_list.selected_item_values);
}

internal void render_3d_initialize(Render* render, const u32 shader,
                                   int* light_dir_location)
{
    VertexBufferLayout vertex_buffer_layout = default_vertex_3d_buffer_layout();
    u32 default_texture = create_default_texture();

    *light_dir_location = glGetUniformLocation(shader, "light_dir");
    ftic_assert((*light_dir_location) != -1);

    const u32 texture_count = 2;
    U32Array textures = { 0 };
    array_create(&textures, texture_count);
    array_push(&textures, default_texture);

    *render = render_create(shader, textures, &vertex_buffer_layout,
                            vertex_buffer_create(), index_buffer_create());

    array_free(&vertex_buffer_layout.items);
}

internal void look_for_and_load_image_thumbnails(DirectoryTab* tab)
{
    DirectoryPage* current = directory_current(&tab->directory_history);
    platform_mutex_lock(&tab->textures.mutex);
    for (u32 i = 0; i < tab->textures.array.size; ++i)
    {
        IdTextureProperties* texture = tab->textures.array.data + i;
        DirectoryItem* item = NULL;
        for (u32 j = 0; j < current->directory.files.size; ++j)
        {
            DirectoryItem* current_item = current->directory.files.data + j;
            if (!guid_compare(texture->id, current_item->id))
            {
                item = current_item;
                break;
            }
        }
        if (item)
        {
            if (item->texture_id)
            {
                texture_delete(item->texture_id);
                item->texture_id = 0;
            }
            item->texture_id = texture_create(&texture->texture_properties,
                                              GL_RGBA8, GL_RGBA, GL_LINEAR);
            item->texture_width = texture->texture_properties.width;
            item->texture_height = texture->texture_properties.height;
            item->reload_thumbnail = false;
        }
        free(texture->texture_properties.bytes);
    }
    tab->textures.array.size = 0;
    platform_mutex_unlock(&tab->textures.mutex);
}

internal u32 mesh_3d_render_to_2d_texture(const Mesh3D* mesh,
                                          const AABB3D* mesh_aabb,
                                          const i32 width, const i32 height,
                                          const AABB* view_port_before,
                                          const u32 shader)
{
    u32 resulting_texture = 0;

    u32 fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    u32 multisampled_texture;
    glGenTextures(1, &multisampled_texture);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, multisampled_texture);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8,
                            (GLsizei)width, (GLsizei)height, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D_MULTISAMPLE, multisampled_texture, 0);

    u32 rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT,
                                     (GLsizei)width, (GLsizei)height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
    {
        u32 resolve_fbo;
        glGenFramebuffers(1, &resolve_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, resolve_fbo);

        glGenTextures(1, &resulting_texture);
        glBindTexture(GL_TEXTURE_2D, resulting_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width,
                     (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, resulting_texture, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
        {
            int light_dir_location = -1;
            Render temp_render = { 0 };
            render_3d_initialize(&temp_render, shader, &light_dir_location);

            vertex_buffer_orphan(temp_render.vertex_buffer_id,
                                 mesh->vertices.size * sizeof(Vertex3D),
                                 GL_STATIC_DRAW, mesh->vertices.data);
            index_buffer_orphan(temp_render.index_buffer_id,
                                mesh->indices.size * sizeof(u32),
                                GL_STATIC_DRAW, mesh->indices.data);

            const V3 aabb_center =
                v3_add(mesh_aabb->min, v3_s_multi(mesh_aabb->size, 0.5f));

            const V3 top_right_corner =
                v3_add(mesh_aabb->min, v3f(mesh_aabb->size.x, mesh_aabb->size.y,
                                           mesh_aabb->size.z));

            Camera camera = camera_create_default();
            camera.position = top_right_corner;
            camera.orientation =
                v3_normalize(v3_sub(aabb_center, camera.position));

            const f32 diagonal_length =
                v3_len(v3_sub(top_right_corner, mesh_aabb->min));
            const f32 scale_factor = 0.082f * diagonal_length;

            camera.position = v3_sub(
                camera.position, v3_s_multi(camera.orientation, scale_factor));

            const AABB view_port = { .size = v2i((f32)width) };
            camera.view_port = view_port;
            camera.view_projection.projection = perspective(
                PI * 0.5f, view_port.size.width / view_port.size.height, 0.1f,
                100.0f);
            camera.view_projection.view =
                view(camera.position,
                     v3_add(camera.position, camera.orientation), camera.up);

            const MVP mvp = {
                .model = m4d(),
                .view = camera.view_projection.view,
                .projection = camera.view_projection.projection,
            };

            glEnable(GL_DEPTH_TEST);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glViewport(0, 0, (int)round_f32(camera.view_port.size.width),
                       (int)round_f32(camera.view_port.size.height));

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            render_begin_draw(&temp_render,
                              temp_render.shader_properties.shader, &mvp);

            glUniform3f(light_dir_location, -camera.orientation.x,
                        -camera.orientation.y, -camera.orientation.z);

            render_draw(0, mesh->indices.size, &camera.view_port);
            render_end_draw(&temp_render);

            glDisable(GL_DEPTH_TEST);

            glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolve_fbo);
            glBlitFramebuffer(0, 0, (GLsizei)width, (GLsizei)height, 0, 0,
                              (GLsizei)width, (GLsizei)height,
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport((int)round_f32(view_port_before->min.x),
                       (int)round_f32(view_port_before->min.y),
                       (int)round_f32(view_port_before->size.width),
                       (int)round_f32(view_port_before->size.height));

            render_destroy(&temp_render);
        }
        else
        {
            texture_delete(resulting_texture);
            resulting_texture = 0;
        }
        glDeleteFramebuffers(1, &resolve_fbo);
    }
    glDeleteTextures(1, &multisampled_texture);
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &rbo);

    return resulting_texture;
}

internal void look_for_and_load_object_thumbnails(const V2 dimensions,
                                                  const u32 shader,
                                                  DirectoryTab* tab)
{
    DirectoryPage* current = directory_current(&tab->directory_history);
    platform_mutex_lock(&tab->objects.mutex);
    for (u32 i = 0; i < tab->objects.array.size; ++i)
    {
        ObjectThumbnail* object = tab->objects.array.data + i;
        DirectoryItem* item = NULL;
        for (u32 j = 0; j < current->directory.files.size; ++j)
        {
            DirectoryItem* current_item = current->directory.files.data + j;
            if (!guid_compare(object->id, current_item->id))
            {
                item = current_item;
                break;
            }
        }
        if (item)
        {
            if (item->texture_id)
            {
                texture_delete(item->texture_id);
                item->texture_id = 0;
            }
            AABB view_port_before = { .size = dimensions };
            item->texture_id = mesh_3d_render_to_2d_texture(
                &object->mesh, &object->mesh_aabb, item->texture_width,
                item->texture_height, &view_port_before, shader);

            item->reload_thumbnail = false;
        }
        array_free(&object->mesh.vertices);
        array_free(&object->mesh.indices);
    }
    tab->objects.array.size = 0;
    platform_mutex_unlock(&tab->objects.mutex);
}

internal b8 show_directory_window(
    const u32 window, const f32 list_item_height, const b8 check_collision,
    const b8 context_menu_open, const V2 dimensions, const u32 shader,
    RecentPanel* recent, ThreadTaskQueue* task_queue, DirectoryTab* tab)
{
    DirectoryPage* current = directory_current(&tab->directory_history);

    look_for_and_load_image_thumbnails(tab);
    look_for_and_load_object_thumbnails(dimensions, shader, tab);

    if (ui_window_begin(window, get_parent_directory_name(current),
                        UI_WINDOW_TOP_BAR | UI_WINDOW_RESIZEABLE))
    {

        V2 position = v2f(0.0f, 0.0f);
        V2 button_size = ui_window_get_button_dimensions(v2d(), "Date", NULL);
        UiWindow* ui_window = ui_window_get(window);
        button_size.width = ui_window->size.width / 3.0f;

        tab->directory_list.reload = false;
        tab->directory_list.item_to_change = NULL;
        tab->directory_list.item_selected = false;

        V2 list_position = v2f(10.0f, position.y + button_size.height + 5.0f);
        if (current->grid_view)
        {
            II32 result = ui_window_add_directory_item_grid(
                v2f(0.0f, list_position.y), &current->directory.sub_directories,
                &current->directory.files, task_queue, &tab->textures,
                &tab->objects, &tab->directory_list);

            if (result.first > -1)
            {
                if (result.first == 0)
                {
                    directory_open_folder(
                        current->directory.sub_directories.data[result.second]
                            .path,
                        &tab->directory_history);
                    directory_clear_selected_items(
                        &tab->directory_list.selected_item_values);

                    recent_panel_add_item(
                        recent,
                        current->directory.sub_directories.data[result.second]
                            .path);
                }
                else
                {
                    platform_open_file(
                        current->directory.files.data[result.second].path);
                }
            }
        }
        else
        {
            i32 selected_item = -1;
            if (ui_window_add_folder_list(list_position, list_item_height,
                                          &current->directory.sub_directories,
                                          &tab->directory_list, &selected_item))
            {
                directory_open_folder(
                    current->directory.sub_directories.data[selected_item].path,
                    &tab->directory_history);

                directory_clear_selected_items(
                    &tab->directory_list.selected_item_values);

                recent_panel_add_item(recent, current->directory.sub_directories
                                                  .data[selected_item]
                                                  .path);
            }
            list_position.y +=
                list_item_height * current->directory.sub_directories.size;
            if (ui_window_add_file_list(list_position, list_item_height,
                                        &current->directory.files,
                                        &tab->directory_list, &selected_item))
            {
                platform_open_file(
                    current->directory.files.data[selected_item].path);
            }
        }
        if (tab->directory_list.reload)
        {
            platform_rename_file(tab->directory_list.item_to_change->path,
                                 tab->directory_list.input.buffer.data,
                                 tab->directory_list.input.buffer.size);
        }
        if (!tab->directory_list.item_selected &&
            !tab->directory_list.input.active)
        {
            if ((!context_menu_open &&
                 event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_LEFT)) ||
                event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_RIGHT))
            {
                tab_clear_selected(tab);
            }
        }
        V4 color = global_get_clear_color();
        color.a = 0.95f;
        ui_window_row_begin(0.0f);
        if (ui_window_add_button(position, &button_size, &color, "Name"))
        {
            set_sorting_buttons(SORT_NAME, tab);
        }
        if (ui_window_add_button(position, &button_size, &color, "Date"))
        {
            set_sorting_buttons(SORT_DATE, tab);
        }
        if (ui_window_add_button(position, &button_size, &color, "Size"))
        {
            set_sorting_buttons(SORT_SIZE, tab);
        }
        ui_window_row_end();
        return ui_window_end();
    }
    return false;
}

internal b8 drop_down_menu_add(DropDownMenu2* drop_down_menu, const b8 icons,
                               const ApplicationContext* application,
                               void* option_data)
{
    const u32 drop_down_item_count = drop_down_menu->options.size;
    if (drop_down_item_count == 0) return true;

    drop_down_menu->x += (f32)(application->delta_time * 5.0);
    drop_down_menu->x = clampf32_high(drop_down_menu->x, 1.0f);

    const f32 drop_down_item_height = 40.0f;
    const f32 end_height = drop_down_item_count * drop_down_item_height;
    const f32 current_y = ease_out_cubic(drop_down_menu->x) * end_height;
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

    drop_down_menu->aabb =
        quad(&drop_down_menu->render->vertices,
             v2f(drop_down_menu->position.x - drop_down_border_width,
                 drop_down_menu->position.y - drop_down_border_width),
             v2f(drop_down_width + border_extra_padding,
                 current_y + border_extra_padding),
             v4a(v4ic(0.15f), 0.92f), 0.0f);
    *drop_down_menu->index_count += 6;

    quad_border_gradiant(&drop_down_menu->render->vertices,
                         drop_down_menu->index_count, drop_down_menu->aabb.min,
                         drop_down_menu->aabb.size, global_get_lighter_color(),
                         global_get_lighter_color(), drop_down_border_width,
                         0.0f);

    b8 item_clicked = false;

    b8 mouse_button_clicked =
        event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_1);

    if (event_is_key_pressed_repeat(FTIC_KEY_TAB))
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

    b8 enter_clicked = event_is_key_clicked(FTIC_KEY_ENTER);

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

        item_clicked = false;

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

        if (drop_down_item_hit)
        {
            quad(&drop_down_menu->render->vertices, promt_item_position,
                 drop_down_item_aabb.size, global_get_border_color(), 0.0f);
            *drop_down_menu->index_count += 6;
        }

        V2 promt_item_text_position = promt_item_position;
        promt_item_text_position.y +=
            application->font.pixel_height + promt_item_text_padding + 3.0f;
        promt_item_text_position.x += promt_item_text_padding;
        promt_item_text_position.x += 32.0f * icons;

        V4 text_color = v4i(1.0f);
        should_close = drop_down_menu->menu_options_selection(
            i, drop_down_item_hit, should_close, item_clicked, &text_color,
            option_data);

        if (icons && i < 3)
        {
            V2 icon_position = promt_item_position;
            icon_position.x += promt_item_text_padding;
            icon_position.y += middle(drop_down_item_aabb.size.height, 24.0f);
            quad(&drop_down_menu->render->vertices, icon_position, v2i(24.0f),
                 text_color, 2.0f + (f32)i);
            *drop_down_menu->index_count += 6;
        }

        *drop_down_menu->index_count += text_generation_color(
            application->font.chars, drop_down_menu->options.data[i], 1.0f,
            promt_item_text_position, 1.0f,
            application->font.pixel_height * precent, text_color, NULL, NULL,
            NULL, &drop_down_menu->render->vertices);

        promt_item_position.y += drop_down_item_aabb.size.y;
    }
    return should_close ||
           ((!any_drop_down_item_hit) && mouse_button_clicked) ||
           event_is_key_clicked(FTIC_KEY_ESCAPE);
}

internal void get_suggestions(const V2 position, DirectoryPage* current,
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

    const FontTTF* ui_font = ui_context_get_font();
    const f32 x_advance =
        text_x_advance(ui_font->chars, parent_directory_input->buffer.data,
                       parent_directory_input->buffer.size, 1.0f);

    suggestions->position =
        v2f(position.x + x_advance, position.y + ui_font->pixel_height + 20.0f);
}

internal void set_input_buffer_to_current_directory(const char* path,
                                                    InputBuffer* input)
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

internal b8 get_word(const FileAttrib* file, u32* index, CharArray* buffer,
                     ColoredCharacterArray* array)
{
    while (file->buffer[*index] == ' ' || file->buffer[*index] == '\t')
    {
        ColoredCharacter value = {
            .color = v4ic(1.0f),
            .character = file->buffer[*index],
        };
        array_push(array, value);
        if (++(*index) >= file->size) return false;
    }
    while (file->buffer[*index] != ' ' && file->buffer[*index] != '=' &&
           file->buffer[*index] != '\n' && file->buffer[*index] != '\r')
    {
        array_push(buffer, file->buffer[*index]);
        if (++(*index) >= file->size) break;
    }
    array_push(buffer, '\0');
    buffer->size--;
    return true;
}

typedef struct UserKeyWords
{
    V4 color;
    CharPtrArray words;
} UserKeyWords;

typedef struct UserKeyWordsArray
{
    u32 size;
    u32 capacity;
    UserKeyWords* data;
} UserKeyWordsArray;

global const char* primitive_key_words[] = {
    "const", "continue", "break", "for",  "if",     "static",
    "else",  "struct",   "enum",  "case", "switch", "while",
};

global const char* user_key_words[] = {
    "int", "char", "u64", "u32", "u16", "u8", "b8",
    "i32", "f32",  "f64", "V2",  "V3",  "V4", "void",
};

global const char* define_key_words[] = {
    "#include", "#define", "global", "internal", "typedef",
};

internal b8 search_keywords(const char** keywords, const u32 keyword_count,
                            const CharArray* word, UU32* start_end)
{
    for (u32 i = 0; i < keyword_count; ++i)
    {
        const char* keyword = keywords[i];
        const char* found_pos = strstr(word->data, keyword);
        if (found_pos != NULL)
        {
            start_end->first = (u32)(found_pos - word->data);
            if (start_end->first != 0)
            {
                if (word->data[start_end->first - 1] != '(')
                {
                    start_end->first = 0;
                    start_end->second = 0;
                    return false;
                }
            }
            start_end->second = start_end->first + (u32)strlen(keyword);
            if (start_end->second != word->size)
            {
                if (word->data[start_end->second] != ')' &&
                    word->data[start_end->second] != '*')
                {
                    start_end->first = 0;
                    start_end->second = 0;
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}

internal b8 contains_key_word(const CharArray* word, V4* color, UU32* start_end)
{
    b8 found = search_keywords(primitive_key_words,
                               static_array_size(primitive_key_words), word,
                               start_end);

    if (found)
    {
        *color = v4f(1.0f, 0.34117f, 0.2f, 1.0f);
    }
    else
    {
        found = search_keywords(
            user_key_words, static_array_size(user_key_words), word, start_end);
        if (found)
        {
            *color = v4f(1.0f, 0.527f, 0.0f, 1.0f);
        }
        else
        {
            found = search_keywords(define_key_words,
                                    static_array_size(define_key_words), word,
                                    start_end);
            if (found)
            {
                *color = v4f(0.527f, 0.68f, 0.527f, 1.0f);
            }
        }
    }

    return found;
}

internal void word_to_color(const CharArray* word, ColoredCharacterArray* array)
{
    V4 color = v4f(0.95f, 0.95f, 1.0f, 1.0f);
    V4 color2 = color;
    V4 function_color = v4f(0.686f, 0.686f, 0.0f, 1.0f);
    b8 found = false;
    UU32 start_end = { 0 };
    found = contains_key_word(word, &color, &start_end);
    if (found)
    {
        u32 index = 0;
        for (u32 i = 0; i < word->size; ++i)
        {
            if (word->data[i] == '(')
            {
                index = i;
                break;
            }
        }
        for (u32 i = 0; i < index; ++i)
        {
            ColoredCharacter value = {
                .color = function_color,
                .character = word->data[i],
            };
            array_push(array, value);
        }
        for (u32 i = index; i < start_end.first; ++i)
        {
            ColoredCharacter value = {
                .color = color2,
                .character = word->data[i],
            };
            array_push(array, value);
        }
        for (u32 i = start_end.first; i < start_end.second; ++i)
        {
            ColoredCharacter value = {
                .color = color,
                .character = word->data[i],
            };
            array_push(array, value);
        }
        for (u32 i = start_end.second; i < word->size; ++i)
        {
            ColoredCharacter value = {
                .color = color2,
                .character = word->data[i],
            };
            array_push(array, value);
        }
    }
    else
    {
        u32 index = 0;
        for (u32 i = 0; i < word->size; ++i)
        {
            if (word->data[i] == '(')
            {
                index = i;
                break;
            }
        }
        for (u32 i = 0; i < index; ++i)
        {
            ColoredCharacter value = {
                .color = function_color,
                .character = word->data[i],
            };
            array_push(array, value);
        }
        for (u32 i = index; i < word->size; ++i)
        {
            ColoredCharacter value = {
                .color = color,
                .character = word->data[i],
            };
            array_push(array, value);
        }
    }
}

void parse_file(FileAttrib* file, ColoredCharacterArray* array)
{
    CharArray word = { 0 };
    array_create(&word, 64);
    for (u32 i = 0; i < file->size; ++i)
    {
        word.size = 0;
        if (get_word(file, &i, &word, array))
        {
            word_to_color(&word, array);
            if (i < file->size)
            {
                ColoredCharacter value = {
                    .color = v4ic(1.0f),
                    .character = file->buffer[i],
                };
                array_push(array, value);
            }
        }
    }
    free(word.data);
}

u32 load_object(Render* render, const char* path)
{
    Mesh3D mesh = { 0 };
    mesh_3d_load(&mesh, path, 0.0f);
    vertex_buffer_orphan(render->vertex_buffer_id,
                         mesh.vertices.size * sizeof(Vertex3D), GL_STATIC_DRAW,
                         mesh.vertices.data);
    index_buffer_orphan(render->index_buffer_id,
                        mesh.indices.size * sizeof(u32), GL_STATIC_DRAW,
                        mesh.indices.data);
    u32 index_count = mesh.indices.size;

    array_free(&mesh.indices);
    array_free(&mesh.vertices);
    return index_count;
}

internal b8 main_drop_down_selection(u32 index, b8 hit, b8 should_close,
                                     b8 item_clicked, V4* text_color,
                                     void* data)
{
    MainDropDownSelectionData* arguments = (MainDropDownSelectionData*)data;
    switch (index)
    {
        case COPY_OPTION_INDEX:
        {
            if (arguments->selected_paths->size == 0)
            {
                *text_color = global_get_lighter_color();
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
                *text_color = global_get_lighter_color();
            }
            else
            {
                if (item_clicked && hit)
                {
                    directory_paste_in_directory(arguments->directory);
                    should_close = true;
                }
            }
            break;
        }
        case DELETE_OPTION_INDEX:
        {
            if (arguments->selected_paths->size == 0)
            {
                *text_color = global_get_lighter_color();
            }
            else
            {
                if (item_clicked && hit)
                {
                    platform_delete_files(arguments->selected_paths);
                    directory_reload(arguments->directory);
                    should_close = true;
                }
            }
            break;
        }
        case ADD_TO_QUICK_OPTION_INDEX:
        {
            if (arguments->selected_paths->size == 0)
            {
                *text_color = global_get_lighter_color();
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
                *text_color = global_get_lighter_color();
            }
            else
            {
                if (item_clicked && hit)
                {
                    platform_show_properties(
                        10, 10, arguments->selected_paths->data[0]);
                    should_close = true;
                }
            }
            break;
        }
        case OPEN_IN_TERMINAL_OPTION_INDEX:
        {
            if (item_clicked && hit)
            {
                should_close = true;
                if (arguments->selected_paths->size != 0)
                {
                    if (platform_directory_exists(
                            arguments->selected_paths->data[0]))
                    {
                        platform_open_terminal(
                            arguments->selected_paths->data[0]);
                        break;
                    }
                }
                platform_open_terminal(arguments->directory->directory.parent);
            }
            break;
        }
        case MORE_OPTION_INDEX:
        {
            if (arguments->selected_paths->size == 0)
            {
                *text_color = global_get_lighter_color();
            }
            else
            {
                if (item_clicked && hit)
                {
                    platform_open_context(arguments->window,
                                          arguments->selected_paths->data[0]);
                    should_close = true;
                }
            }
            break;
        }
        default: break;
    }
    return should_close;
}

internal b8 suggestion_selection(u32 index, b8 hit, b8 should_close,
                                 b8 item_clicked, V4* text_color, void* data)
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

internal void open_window(const V2 dimensions, const u32 window_id)
{
    UiWindow* window = ui_window_get(window_id);
    V2 end_size = v2i(200.0f);
    V2 end_position = v2f(middle(dimensions.width, end_size.width),
                          middle(dimensions.height, end_size.height));

    window->size = v2i(0.0f);
    window->position = v2f(middle(dimensions.width, window->size.width),
                           middle(dimensions.height, window->size.height));

    window->dock_node->aabb.min = end_position;
    window->dock_node->aabb.size = end_size;

    ui_window_start_position_animation(window, window->position, end_position);
    ui_window_start_size_animation(window, window->size, end_size);
}

internal void save_application_state(ApplicationContext* app)
{
    FILE* file = fopen("saved/application.txt", "wb");
    if (file == NULL)
    {
        log_file_error("saved/application.txt");
        return;
    }

    fwrite(&app->tabs.size, sizeof(app->tabs.size), 1, file);
    for (u32 i = 0; i < app->tabs.size; ++i)
    {
        char* path = directory_current(&app->tabs.data[i].directory_history)
                         ->directory.parent;
        u32 path_length = (u32)strlen(path);
        fwrite(&path_length, sizeof(u32), 1, file);
        fwrite(path, sizeof(char), path_length, file);
        fwrite(&app->tabs.data[i].window_id,
               sizeof(app->tabs.data[i].window_id), 1, file);
    }
    fclose(file);
}

internal void load_application_state(ApplicationContext* app)
{
    FILE* file = fopen("saved/application.txt", "rb");
    if (file == NULL)
    {
        log_file_error("saved/application.txt");
        return;
    }

    u32 size = 0;
    if (!fread(&size, sizeof(size), 1, file))
    {
        return;
    }
    for (u32 i = 0; i < size; ++i)
    {
        u32 path_length = 0;
        fread(&path_length, sizeof(u32), 1, file);

        char* path = (char*)calloc(path_length + 3, sizeof(char));
        fread(path, sizeof(char), path_length, file);

        u32 window_id = 0;
        fread(&window_id, sizeof(window_id), 1, file);

        if (platform_directory_exists(path))
        {
            path[path_length++] = '\\';
            path[path_length++] = '*';

            DirectoryTab tab = { 0 };
            array_push(&app->tabs, tab);
            directory_tab_add(path, &app->thread_queue.task_queue,
                              array_back(&app->tabs));

            path[path_length - 2] = '\0';
            path[path_length - 1] = '\0';

            array_back(&app->tabs)->window_id = window_id;
        }
        free(path);
    }
    fclose(file);
}

internal void access_panel_save(AccessPanel* panel, const char* file_path)
{
    if (!panel->items.size) return;

    u32 buffer_size = 0;
    u32* path_lengths = (u32*)calloc(panel->items.size, sizeof(u32));
    for (u32 i = 0; i < panel->items.size; ++i)
    {
        path_lengths[i] = (u32)strlen(panel->items.data[i].path);
        buffer_size += path_lengths[i];
        buffer_size++;
    }

    char* buffer = (char*)calloc(buffer_size, sizeof(char));
    u32 buffer_offset = 0;
    for (u32 i = 0; i < panel->items.size; ++i)
    {
        memcpy(buffer + buffer_offset, panel->items.data[i].path,
               path_lengths[i]);
        buffer_offset += path_lengths[i];
        buffer[buffer_offset++] = '\n';
    }
    buffer[--buffer_offset] = '\0';

    file_write(file_path, buffer, buffer_offset);

    free(path_lengths);
    free(buffer);
}

internal void access_panel_load(AccessPanel* panel, const char* file_path)
{
    FileAttrib file = file_read(file_path);
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
            array_push(&panel->items, item);
        }
        line.size = 0;
    }
    free(line.data);
    free(file.buffer);
}

internal void access_panel_initialize(AccessPanel* panel, const u32 window,
                                      const char* load_file_path)
{
    panel->menu_item.window = window;
    array_create(&panel->items, 10);
    access_panel_load(panel, load_file_path);
    panel->menu_item.show = true;
    panel->menu_item.switch_on = true;
    panel->menu_item.switch_x = 0.0f;
}

internal void access_panel_open(AccessPanel* panel, const char* title,
                                const f32 list_item_height, RecentPanel* recent,
                                DirectoryHistory* directory_history)
{
    if (panel->menu_item.show)
    {
        if (ui_window_begin(panel->menu_item.window, title,
                            UI_WINDOW_TOP_BAR | UI_WINDOW_RESIZEABLE))
        {
            V2 list_position = v2i(10.0f);
            i32 selected_item = -1;
            if (ui_window_add_folder_list(list_position, list_item_height,
                                          &panel->items, NULL, &selected_item))
            {
                directory_open_folder(panel->items.data[selected_item].path,
                                      directory_history);

                recent_panel_add_item(recent,
                                      panel->items.data[selected_item].path);
            }

            panel->menu_item.show = !ui_window_end();
        }
        if (!panel->menu_item.show)
        {
            panel->menu_item.switch_on = panel->menu_item.show;
        }
    }
}

internal void search_page_initialize(SearchPage* search_page)
{
    safe_array_create(&search_page->search_result_file_array, 10);
    safe_array_create(&search_page->search_result_folder_array, 10);
    search_page->input = ui_input_buffer_create();
    search_page->running_id = 0;
    search_page->last_running_id = 0;
}

internal void main_render_initialize(RenderingProperties* main_render,
                                     TextureProperties* texture_properties)
{
    u32 font_texture =
        texture_create(texture_properties, GL_RGBA8, GL_RGBA, GL_NEAREST);
    free(texture_properties->bytes);

    VertexBufferLayout vertex_buffer_layout = default_vertex_buffer_layout();

    u32 default_texture = create_default_texture();
    u32 copy_texture = load_icon("res/icons/copy.png");
    u32 paste_texture = load_icon("res/icons/paste.png");
    u32 delete_texture = load_icon("res/icons/delete.png");

    u32 shader = shader_create("./res/shaders/vertex.glsl",
                               "./res/shaders/fragment.glsl");

    ftic_assert(shader);

    const u32 texture_count = 4;
    U32Array textures = { 0 };
    array_create(&textures, texture_count);
    array_push(&textures, default_texture);
    array_push(&textures, font_texture);
    array_push(&textures, copy_texture);
    array_push(&textures, paste_texture);
    array_push(&textures, delete_texture);

    array_create(&main_render->vertices, 100 * 4);
    u32 vertex_buffer_id = vertex_buffer_create();
    vertex_buffer_orphan(vertex_buffer_id,
                         main_render->vertices.capacity * sizeof(Vertex),
                         GL_STREAM_DRAW, NULL);
    main_render->vertex_buffer_capacity = main_render->vertices.capacity;

    array_create(&main_render->indices, 100 * 6);
    generate_indicies(&main_render->indices, 0, 100);
    u32 index_buffer_id = index_buffer_create();
    index_buffer_orphan(index_buffer_id,
                        main_render->indices.size * sizeof(u32), GL_STATIC_DRAW,
                        main_render->indices.data);
    free(main_render->indices.data);

    main_render->render = render_create(shader, textures, &vertex_buffer_layout,
                                        vertex_buffer_id, index_buffer_id);

    array_free(&vertex_buffer_layout.items);
}

u8* application_initialize(ApplicationContext* app)
{
    app->window = window_create("FileTic", 1250, 800);
    event_initialize(app->window);
    platform_init_drag_drop();
    thread_initialize(100000, platform_get_core_count() - 1,
                      &app->thread_queue);

    app->font = (FontTTF){ 0 };
    const i32 width_atlas = 512;
    const i32 height_atlas = 512;
    const f32 pixel_height = 16;
    const u32 bitmap_size = width_atlas * height_atlas;
    u8* font_bitmap_temp = (u8*)calloc(bitmap_size, sizeof(u8));
    init_ttf_atlas(width_atlas, height_atlas, pixel_height, 96, 32,
                   "C:/Windows/Fonts/arial.ttf", font_bitmap_temp, &app->font);

    // Puts the red channel in the alpha.
    u8* font_bitmap = (u8*)malloc(bitmap_size * 4 * sizeof(u8));
    memset(font_bitmap, UINT8_MAX, bitmap_size * 4 * sizeof(u8));
    for (u32 i = 0, j = 3; i < bitmap_size; ++i, j += 4)
    {
        font_bitmap[j] = font_bitmap_temp[i];
    }
    free(font_bitmap_temp);

    TextureProperties font_texture_properties = {
        .width = width_atlas,
        .height = height_atlas,
        .bytes = font_bitmap,
    };

    main_render_initialize(&app->main_render, &font_texture_properties);

    u32 shader = shader_create("./res/shaders/vertex3d.glsl",
                               "./res/shaders/fragment.glsl");
    ftic_assert(shader);
    render_3d_initialize(&app->render_3d, shader, &app->light_dir_location);

    array_create(&app->tabs, 10);
    app->tab_index = 0;

    app->last_time = platform_get_time();
    app->delta_time = 0.0f;
    app->mvp = (MVP){ 0 };
    app->mvp.view = m4d();
    app->mvp.model = m4d();

    app->last_moved_time = window_get_time();

    search_page_initialize(&app->search_page);

    ui_context_create();

    array_create(&app->windows, 20);
    for (u32 i = 0; i < 20; ++i)
    {
        array_push(&app->windows, ui_window_create());
    }

    u32 window_index = 0;
    app->top_bar_window = app->windows.data[window_index++];
    app->bottom_bar_window = app->windows.data[window_index++];
    app->preview_window = app->windows.data[window_index++];
    app->menu_window = app->windows.data[window_index++];
    app->windows_window = app->windows.data[window_index++];
    app->font_change_window = app->windows.data[window_index++];

    app->search_result_window_item.window = app->windows.data[window_index++];
    app->search_result_window_item.show = true;
    app->search_result_window_item.switch_on = true;
    app->search_result_window_item.switch_x = 0.0f;

    array_create(&app->free_window_ids, 10);
    array_create(&app->tab_windows, 20);
    for (u32 i = 0; i < 20; ++i)
    {
        array_push(&app->tab_windows, ui_window_create());
    }

    b8 saved = true;
    load_application_state(app);
    if (app->tabs.size == 0)
    {
        DirectoryTab tab = { 0 };
        array_push(&app->tabs, tab);
        directory_tab_add("C:\\*", &app->thread_queue.task_queue,
                          array_back(&app->tabs));
        array_back(&app->tabs)->window_id =
            app->tab_windows.data[app->current_tab_window_index++];
        saved = false;
    }

    if (saved)
    {
        u32 count = 0;
        for (u32 i = 0; i < app->tab_windows.size && count < app->tabs.size;
             ++i)
        {
            b8 exist = false;
            for (u32 j = 0; j < app->tabs.size; ++j)
            {
                if (app->tabs.data[j].window_id == app->tab_windows.data[i])
                {
                    ++count;
                    exist = true;
                    app->current_tab_window_index = i + 1;
                    break;
                }
            }
            if (!exist)
            {
                array_push(&app->free_window_ids, app->tab_windows.data[i]);
            }
        }
    }

    array_create(&app->pasted_paths, 10);

    app->context_menu = (DropDownMenu2){
        .index_count = &app->main_index_count,
        .tab_index = -1,
        .menu_options_selection = main_drop_down_selection,
        .render = &app->main_render,
    };

    char* options[] = {
        [COPY_OPTION_INDEX] = "Copy",
        [PASTE_OPTION_INDEX] = "Paste",
        [DELETE_OPTION_INDEX] = "Delete",
        [ADD_TO_QUICK_OPTION_INDEX] = "Add to quick",
        [OPEN_IN_TERMINAL_OPTION_INDEX] = "Open in terminal",
        [PROPERTIES_OPTION_INDEX] = "Properties",
        [MORE_OPTION_INDEX] = "More",
    };
    array_create(&app->context_menu.options, static_array_size(options));
    array_push(&app->context_menu.options,
               string_copy_d(options[COPY_OPTION_INDEX]));
    array_push(&app->context_menu.options,
               string_copy_d(options[PASTE_OPTION_INDEX]));
    array_push(&app->context_menu.options,
               string_copy_d(options[DELETE_OPTION_INDEX]));
    array_push(&app->context_menu.options,
               string_copy_d(options[ADD_TO_QUICK_OPTION_INDEX]));
    array_push(&app->context_menu.options,
               string_copy_d(options[OPEN_IN_TERMINAL_OPTION_INDEX]));
    array_push(&app->context_menu.options,
               string_copy_d(options[PROPERTIES_OPTION_INDEX]));
    array_push(&app->context_menu.options,
               string_copy_d(options[MORE_OPTION_INDEX]));

    access_panel_initialize(&app->quick_access,
                            app->windows.data[window_index++],
                            "saved/quick_access.txt");
    access_panel_initialize(&app->recent.panel,
                            app->windows.data[window_index++],
                            "saved/recent.txt");
    app->recent.total = 10;

    app->context_menu_window = app->windows.data[window_index++];

    app->parent_directory_input = ui_input_buffer_create();

    app->suggestions = (DropDownMenu2){
        .index_count = &app->main_index_count,
        .tab_index = -1,
        .menu_options_selection = suggestion_selection,
        .render = &app->main_render,
    };
    array_create(&app->suggestions.options, 10);

    app->suggestion_data = (SuggestionSelectionData){
        .parent_directory = &app->parent_directory_input.buffer,
        .tab_index = &app->suggestions.tab_index,
        .cursor_index = &app->parent_directory_input.input_index,
    };
    array_create(&app->suggestion_data.items, 6);

    app->show_hidden_files = true;

    return font_bitmap;
}

void application_uninitialize(ApplicationContext* app)
{
    memset(app->search_page.running_callbacks, 0,
           sizeof(app->search_page.running_callbacks));

    window_destroy(app->window);

    save_application_state(app);
    for (u32 i = 0; i < app->tabs.size; ++i)
    {
        directory_tab_clear(app->tabs.data + i);
    }
    for (u32 i = 0; i < app->context_menu.options.size; ++i)
    {
        free(app->context_menu.options.data[i]);
    }
    free(app->windows.data);
    free(app->tab_windows.data);
    free(app->free_window_ids.data);

    ui_context_destroy();

    render_destroy(&app->main_render.render);
    free(app->main_render.vertices.data);

    access_panel_save(&app->quick_access, "saved/quick_access.txt");
    access_panel_save(&app->recent.panel, "saved/recent.txt");

    platform_uninit_drag_drop();
    threads_uninitialize(&app->thread_queue);
    event_uninitialize();
}

void application_begin_frame(ApplicationContext* app)
{
    int width, height;
    window_get_size(app->window, &width, &height);
    app->dimensions = v2f((f32)width, (f32)height);
    double x, y;
    window_get_mouse_position(app->window, &x, &y);
    app->mouse_position = v2f((f32)x, (f32)y);
    if (event_get_mouse_move_event()->activated)
    {
        app->last_moved_time = window_get_time();
    }

    glViewport(0, 0, width, height);
    glClearColor(global_get_clear_color().r, global_get_clear_color().g,
                 global_get_clear_color().b, global_get_clear_color().a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    app->mvp.projection = ortho(0.0f, app->dimensions.width,
                                app->dimensions.height, 0.0f, -1.0f, 1.0f);

    app->main_index_count = 0;
    rendering_properties_clear(&app->main_render);

    if (event_is_ctrl_and_key_pressed(FTIC_KEY_T))
    {
        DirectoryTab tab = { 0 };
        array_push(&app->tabs, tab);
        directory_tab_add("C:\\*", &app->thread_queue.task_queue,
                          array_back(&app->tabs));
        app->tab_index = app->tabs.size - 1;
        u32 window_id = 0;
        if (app->free_window_ids.size)
        {
            window_id = *array_back(&app->free_window_ids);
            app->free_window_ids.size--;
        }
        else
        {
            if (app->tab_windows.size <= app->current_tab_window_index)
            {
                ftic_assert(false);
            }
            window_id = app->tab_windows.data[app->current_tab_window_index++];
        }
        array_back(&app->tabs)->window_id = window_id;
        open_window(app->dimensions, window_id);
    }
}

void application_end_frame(ApplicationContext* app)
{
    window_swap(app->window);
    event_poll();

    f64 now = window_get_time();
    app->delta_time = now - app->last_time;
    app->last_time = now;
}

f64 application_get_last_mouse_move_time(const ApplicationContext* app)
{
    return window_get_time() - app->last_moved_time;
}

internal void clear_search_result(SafeFileArray* files)
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

void search_page_clear_result(SearchPage* page)
{
    clear_search_result(&page->search_result_file_array);
    clear_search_result(&page->search_result_folder_array);
}

b8 search_page_has_result(const SearchPage* search_page)
{
    return search_page->search_result_file_array.array.size > 0 ||
           search_page->search_result_folder_array.array.size > 0;
}

internal void parse_all_subdirectories(const char* start_directory,
                                       const u32 length)
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

internal void safe_add_directory_item(const DirectoryItem* item,
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

internal void finding_callback(void* data)
{
    FindingCallbackAttribute* arguments = (FindingCallbackAttribute*)data;

    b8 should_free_directory = false;
    b8 running = arguments->running_callbacks[arguments->running_id];
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
        next_arguments->running_callbacks = arguments->running_callbacks;

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
        platform_reset_directory(&directory, true);
    }
    free(arguments->start_directory);
    free(data);
}

void search_page_search(SearchPage* page, DirectoryHistory* directory_history,
                        ThreadTaskQueue* thread_task_queue)
{
    search_page_clear_result(page);

    page->running_callbacks[page->last_running_id] = false;

    if (page->input.buffer.size)
    {
        page->last_running_id = page->running_id;
        page->running_callbacks[page->running_id++] = true;
        page->running_id %= 100;

        const char* parent =
            directory_current(directory_history)->directory.parent;
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
        arguments->running_callbacks = page->running_callbacks;
        finding_callback(arguments);
    }
}

internal i32 add_increment(const V2 position, const char* text,
                           const V4 button_color, const f32 drop_down_width,
                           const u32 value)
{
    i32 result = -1;

    const f32 ui_font_pixel_height = ui_context_get_font_pixel_height();
    const V2 button1_dim = ui_window_get_button_dimensions(v2d(), "-", NULL);
    const V2 button2_dim = ui_window_get_button_dimensions(v2d(), "+", NULL);

    char buffer[20] = { 0 };
    value_to_string(buffer, "%u", value);

    const FontTTF* ui_font = ui_context_get_font();
    const f32 x_advance =
        text_x_advance(ui_font->chars, buffer, (u32)strlen(buffer), 1.0f);

    const f32 x_position = drop_down_width - button1_dim.width -
                           button2_dim.width - x_advance - (20.0f * 2.0f) -
                           position.x;

    V2 text_position = v2f(position.x, position.y + 2.0f);
    ui_window_add_text(text_position, text, false);

    ui_window_row_begin(20.0f);

    if (ui_window_add_button(v2f(x_position, position.y), NULL, &button_color,
                             "-"))
    {
        result = 0;
    }
    text_position.x = x_position;
    ui_window_add_text(text_position, buffer, false);
    if (ui_window_add_button(v2f(x_position, position.y), NULL, &button_color,
                             "+"))
    {
        result = 1;
    }

    ui_window_row_end();
    return result;
}

internal void application_open_menu_window(ApplicationContext* app,
                                           const f32 drop_down_width,
                                           const V4 button_color)
{
    if (ui_window_begin(app->menu_window, NULL,
                        UI_WINDOW_OVERLAY | UI_WINDOW_FROSTED_GLASS))
    {
        const f32 ui_font_pixel_height = ui_context_get_font_pixel_height();

        presist b8 animation_on_selected = true;
        presist b8 dark_mode_selected = true;
        presist b8 highlight_fucused_window_selected = true;
        presist b8 ui_frosted_glass_selected = true;

        presist f32 dark_mode_x = 0.0f;
        presist f32 hidden_files_x = 0.0f;
        presist f32 focused_window_x = 0.0f;
        presist f32 animation_x = 0.0f;
        presist f32 ui_frosted_x = 0.0f;

        AABB row = {
            .min = v2i(10.0f),
            .size = v2f(200.0f, 40.0f),
        };

        const V2 switch_size = ui_window_get_switch_size();

        const f32 switch_x = drop_down_width - switch_size.width - row.min.x;

        {
            V2 text_position = v2f(row.min.x, row.min.y + 2.0f);
            ui_window_add_text(text_position, "Font:", false);

            const char* font_path = ui_context_get_font_path();
            const u32 path_length =
                get_path_length(font_path, (u32)strlen(font_path));

            const V2 button_dim = ui_window_get_button_dimensions(
                v2d(), font_path + path_length, NULL);

            const f32 x_position =
                drop_down_width - button_dim.width - row.min.x;

            if (ui_window_add_button(v2f(x_position, row.min.y), NULL,
                                     &button_color, font_path + path_length))
            {
                if (app->font_change_directory.parent)
                {
                    platform_reset_directory(&app->font_change_directory, true);
                }
                const char* font_directory_path = "C:\\Windows\\Fonts\\*";
                app->font_change_directory = platform_get_directory(
                    font_directory_path, (u32)strlen(font_directory_path));
                DirectoryItemArray* files = &app->font_change_directory.files;
                for (i32 i = 0; i < (i32)files->size; ++i)
                {
                    const char* extension = file_get_extension(
                        files->data[i].name, (u32)strlen(files->data[i].name));
                    if (!extension || strcmp(extension, "ttf"))
                    {
                        char* path_to_remove = files->data[i].path;
                        for (u32 j = i; j < files->size - 1; ++j)
                        {
                            files->data[j] = files->data[j + 1];
                        }
                        free(path_to_remove);
                        files->size--;
                        --i;
                    }
                }
                app->open_font_change_window = true;
            }
        }

        row.min.y += ui_font_pixel_height + 20.0f;
        {
            i32 result =
                add_increment(row.min, "Font size:", button_color,
                              drop_down_width, (u32)ui_font_pixel_height);
            if (result == 0)
            {
                ui_context_change_font_pixel_height(ui_font_pixel_height - 1);
            }
            else if (result == 1)
            {
                ui_context_change_font_pixel_height(ui_font_pixel_height + 1);
            }
        }

        row.min.y += ui_font_pixel_height + 20.0f;
        {
            i32 result = add_increment(row.min, "Recent total:", button_color,
                                       drop_down_width, app->recent.total);
            if (result == 0)
            {
                app->recent.total = max(app->recent.total - 1, 2);
            }
            else if (result == 1)
            {
                app->recent.total = min(app->recent.total + 1, 40);
            }
        }

        const V2 ui_icon_min_max = ui_get_big_icon_min_max();
        row.min.y += ui_font_pixel_height + 20.0f;
        {
            i32 result =
                add_increment(row.min, "Ui icon min:", button_color,
                              drop_down_width, (u32)ui_icon_min_max.min);
            if (result == 0)
            {
                ui_set_big_icon_min_size(
                    max(ui_icon_min_max.min - 1.0f, 32.0f));
            }
            else if (result == 1)
            {
                ui_set_big_icon_min_size(
                    min(ui_icon_min_max.min + 1.0f, 64.0f));
            }
        }
        row.min.y += ui_font_pixel_height + 20.0f;
        {
            i32 result =
                add_increment(row.min, "Ui icon max:", button_color,
                              drop_down_width, (u32)ui_icon_min_max.max);
            if (result == 0)
            {
                ui_set_big_icon_max_size(
                    max(ui_icon_min_max.max - 1.0f, 128.0f));
            }
            else if (result == 1)
            {
                ui_set_big_icon_max_size(
                    min(ui_icon_min_max.max + 1.0f, 256.0f));
            }
        }

        row.min.y += ui_font_pixel_height + 20.0f;
        {
            ui_window_add_text(row.min, "Animation on:", false);
            ui_window_add_switch(v2f(switch_x, row.min.y),
                                 &animation_on_selected, &animation_x);
            ui_context_set_animation(animation_on_selected);
        }

        row.min.y += ui_font_pixel_height + 20.0f;
        {
            ui_window_add_text(row.min, "Highlight focused window:", false);
            ui_window_add_switch(v2f(switch_x, row.min.y),
                                 &highlight_fucused_window_selected,
                                 &focused_window_x);
            ui_context_set_highlight_focused_window(
                highlight_fucused_window_selected);
        }

        row.min.y += ui_font_pixel_height + 20.0f;
        {
            ui_window_add_text(row.min, "Show hidden files:", false);
            b8 before = app->show_hidden_files;
            ui_window_add_switch(v2f(switch_x, row.min.y),
                                 &app->show_hidden_files, &hidden_files_x);
            platform_show_hidden_files(app->show_hidden_files);
            if (before != app->show_hidden_files)
            {
                for (u32 i = 0; i < app->tabs.size; ++i)
                {
                    directory_reload(directory_current(
                        &app->tabs.data[i].directory_history));
                }
            }
        }

        row.min.y += ui_font_pixel_height + 20.0f;
        {
            ui_window_add_text(row.min, "Dark mode:", false);
            ui_window_add_switch(v2f(switch_x, row.min.y), &dark_mode_selected,
                                 &dark_mode_x);
            if (dark_mode_selected)
            {
                global_set_clear_color(v4ic(0.09f));
                global_set_text_color(v4ic(1.0f));
            }
            else
            {
                global_set_clear_color(v4ic(0.8f));
                global_set_text_color(v4ic(0.0f));
            }
        }

        row.min.y += ui_font_pixel_height + 20.0f;
        {
            ui_window_add_text(row.min, "Frosted glass:", false);
            ui_window_add_switch(v2f(switch_x, row.min.y),
                                 &ui_frosted_glass_selected, &ui_frosted_x);
            ui_set_frosted_glass(ui_frosted_glass_selected);
        }

        if (ui_frosted_glass_selected)
        {
            row.min.y += ui_font_pixel_height + 20.0f;
            {
                ui_window_add_text(row.min, "Blur amount:", false);
                const V2 slider_size = v2f(78.0f + (ui_font_pixel_height * 2),
                                           -3.0f + (ui_font_pixel_height * 0.5f));
                const f32 slider_x =
                    drop_down_width - slider_size.width - row.min.x;
                presist b8 pressed = false;
                ui_set_frosted_blur_amount(ui_window_add_slider(
                    v2f(slider_x, row.min.y + 8.0f), slider_size, 0.0002f,
                    0.003f, ui_get_frosted_blur_amount(), &pressed));
            }
        }

        row.min.y += ui_font_pixel_height + 20.0f;
        {
            const char* text = "Reset to default";
            const V2 button_dim =
                ui_window_get_button_dimensions(v2d(), text, NULL);
            const f32 x_position = middle(drop_down_width, button_dim.width);
            if (ui_window_add_button(v2f(x_position, row.min.y), NULL,
                                     &button_color, text))
            {
                ui_context_set_font_path("C:/Windows/Fonts/arial.ttf");
                ui_context_change_font_pixel_height(16);

                animation_x = 1.0f * animation_on_selected;
                dark_mode_x = 1.0f * dark_mode_selected;
                focused_window_x = 1.0f * highlight_fucused_window_selected;
                hidden_files_x = 1.0f * app->show_hidden_files;

                animation_on_selected = true;
                dark_mode_selected = true;
                highlight_fucused_window_selected = true;
                app->show_hidden_files = true;

                app->recent.total = 10;

                ui_set_big_icon_min_size(64.0f);
                ui_set_big_icon_max_size(256.0f);
                ui_set_frosted_blur_amount(0.00132f);
            }
        }

        if (ui_window_end() && !app->open_font_change_window &&
            app->open_menu_window)
        {
            dark_mode_x = 0.0f;
            hidden_files_x = 0.0f;
            focused_window_x = 0.0f;
            animation_x = 0.0f;
            ui_frosted_x = 0.0f;
            app->open_menu_window = false;
        }

        row.min.y += ui_font_pixel_height + 20.0f;
        UiWindow* top_bar_menu = ui_window_get(app->menu_window);
        top_bar_menu->size.width = drop_down_width;
        top_bar_menu->size.height = row.min.y;
    }
}

internal void window_open_menu_item_add(WindowOpenMenuItem* item,
                                        const V2 position,
                                        const f32 switch_position_x,
                                        const char* text, const V2 dimensions)
{
    ui_window_add_text(position, text, false);
    b8 selected_before = item->switch_on;
    ui_window_add_switch(v2f(switch_position_x, position.y), &item->switch_on,
                         &item->switch_x);

    if (item->switch_on)
    {
        if (!item->show)
        {
            open_window(dimensions, item->window);
            item->show = true;
        }
    }
    else if (selected_before != item->switch_on)
    {
        if (item->show)
        {
            ui_window_close(item->window);
        }
    }
}

internal void application_open_windows_window(ApplicationContext* app,
                                              const f32 drop_down_width,
                                              const V4 button_color)
{
    if (ui_window_begin(app->windows_window, NULL,
                        UI_WINDOW_OVERLAY | UI_WINDOW_FROSTED_GLASS))
    {
        const f32 ui_font_pixel_height = ui_context_get_font_pixel_height();

        AABB row = {
            .min = v2i(10.0f),
            .size = v2f(200.0f, 40.0f),
        };

        const V2 switch_size = ui_window_get_switch_size();
        const f32 switch_x = drop_down_width - switch_size.width - row.min.x;

        window_open_menu_item_add(&app->quick_access.menu_item, row.min,
                                  switch_x, "Quick access:", app->dimensions);

        row.min.y += ui_font_pixel_height + 20.0f;

        window_open_menu_item_add(&app->recent.panel.menu_item, row.min,
                                  switch_x, "Recent:", app->dimensions);

        row.min.y += ui_font_pixel_height + 20.0f;

        window_open_menu_item_add(&app->search_result_window_item, row.min,
                                  switch_x, "Search result:", app->dimensions);

        if (ui_window_end() && app->open_windows_window)
        {
            app->quick_access.menu_item.switch_x = 0.0f;
            app->recent.panel.menu_item.switch_x = 0.0f;
            app->search_result_window_item.switch_x = 0.0f;
            app->open_windows_window = false;
        }

        row.min.y += ui_font_pixel_height + 20.0f;
        UiWindow* top_bar_windows = ui_window_get(app->windows_window);
        top_bar_windows->size.width = drop_down_width;
        top_bar_windows->size.height = row.min.y;
    }
}

internal void application_open_context_menu_window(ApplicationContext* app,
                                                   DirectoryTab* current_tab)
{
    if (ui_window_begin(app->context_menu_window, NULL,
                        UI_WINDOW_OVERLAY | UI_WINDOW_FROSTED_GLASS))
    {
        const f32 ui_font_pixel_height = ui_context_get_font_pixel_height();

        const char* options[] = {
            [COPY_OPTION_INDEX] = "Copy",
            [PASTE_OPTION_INDEX] = "Paste",
            [DELETE_OPTION_INDEX] = "Delete",
            [ADD_TO_QUICK_OPTION_INDEX] = "Add to quick",
            [OPEN_IN_TERMINAL_OPTION_INDEX] = "Open in terminal",
            [PROPERTIES_OPTION_INDEX] = "Properties",
            [MORE_OPTION_INDEX] = "More",
        };
        V2 position = v2i(0.0f);
        V2 item_size = v2f(250.0f, 24.0f + ui_font_pixel_height);
        const f32 item_text_padding = 5.0f;
        for (u32 i = 0; i < CONTEXT_ITEM_COUNT; ++i)
        {
            V4 text_color = v4ic(1.0f);
            b8 clicked = ui_window_add_button(position, &item_size, NULL, NULL);

            MainDropDownSelectionData data = {
                .window = app->window,
                .quick_access = &app->quick_access.items,
                .directory = directory_current(&current_tab->directory_history),
                .selected_paths =
                    &current_tab->directory_list.selected_item_values.paths,
            };
            if (main_drop_down_selection(i, clicked, false, clicked,
                                         &text_color, &data))
            {
                tab_clear_selected(current_tab);
                app->open_context_menu_window = false;
            }

            if (i < 3)
            {
                V2 icon_position = position;
                icon_position.x += item_text_padding;
                icon_position.y += middle(item_size.height, 24.0f);
                ui_window_add_image(
                    icon_position, v2i(24.0f),
                    app->main_render.render.textures.data[2 + i]);
            }

            V2 item_text_position = position;
            item_text_position.x += item_text_padding;
            item_text_position.x += 32.0f;
            item_text_position.y +=
                middle(item_size.height, ui_font_pixel_height) - 2.0f;

            ui_window_add_text_c(item_text_position, text_color, options[i],
                                 false);

            position.y += item_size.height;
        }

        const f32 end_height = CONTEXT_ITEM_COUNT * item_size.height;
        UiWindow* window = ui_window_get(app->context_menu_window);
        window->size.height = ease_out_cubic(app->context_menu_x) * end_height;
        window->size.width = item_size.width;

        app->context_menu_x += (f32)(app->delta_time * 8.0);
        app->context_menu_x = clampf32_high(app->context_menu_x, 1.0f);

        if (ui_window_end())
        {
            app->context_menu_x = 0.0f;
            app->open_context_menu_window = false;
        }
    }
}

void application_run()
{
    ApplicationContext app = { 0 };
    u8* font_bitmap = application_initialize(&app);

    ContextMenu menu = { 0 };

    V2 preview_image_dimensions = v2d();
    char* current_path = NULL;
    FileAttrib preview_file = { 0 };
    ColoredCharacterArray preview_file_colored = { 0 };
    array_create(&preview_file_colored, 1000);
    U32Array preview_textures = { 0 };
    array_create(&preview_textures, 10);
    i32 preview_index = -1;
    Camera preview_camera = camera_create_default();

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
            tab->directory_list.selected_item_values.paths.size > 0)
        {
            distance = v2_distance(last_mouse_position, app.mouse_position);
            if (distance >= 10.0f)
            {
                // platform_start_drag_drop(&tab->selected_item_values.paths);
                activated = false;
            }
        }

        b8 check_collision = preview_index != 1;

        if (check_collision &&
            (collision_point_in_aabb(app.mouse_position,
                                     &app.context_menu.aabb) ||
             collision_point_in_aabb(app.mouse_position,
                                     &app.suggestions.aabb)))
        {
            check_collision = false;
        }

        if (preview_index != 1 &&
            tab->directory_list.selected_item_values.paths.size)
        {
            if (event_is_ctrl_and_key_pressed(FTIC_KEY_C))
            {
                // TODO: this is used in input fields
                // platform_copy_to_clipboard(&tab->selected_item_values.paths);
            }
            else if (preview_index == -1 &&
                     tab->directory_list.selected_item_values.paths.size == 1 &&
                     event_is_ctrl_and_key_pressed(FTIC_KEY_D))
            {
                const char* path =
                    tab->directory_list.selected_item_values.paths.data[0];

                if (load_preview_image(path, &preview_image_dimensions,
                                       &preview_textures))
                {
                    current_path = string_copy(path, (u32)strlen(path), 0);
                    directory_clear_selected_items(
                        &tab->directory_list.selected_item_values);
                    preview_index = 0;
                }
                else
                {
                    const char* extension =
                        file_get_extension(path, (u32)strlen(path));
                    if (strcmp(extension, "obj") == 0)
                    {
                        preview_index = 1;
                        app.index_count_3d = load_object(&app.render_3d, path);
                    }
                    else
                    {
                        preview_file = file_read(path);
                        parse_file(&preview_file, &preview_file_colored);
                        preview_index = 2;
                    }
                }
            }
            else if (key_event->activated && key_event->action == 1 &&
                     key_event->key == FTIC_KEY_ESCAPE)
            {
                // TODO: Not get called when you press escape in a drop
                // down for example
                // platform_delete_files(&tab->selected_item_values.paths);
                // directory_reload(
                //   directory_current(&application.directory_history));
            }
        }

        if (preview_index != 1 && event_is_ctrl_and_key_pressed(FTIC_KEY_V))
        {
            // TODO: this is used in input fields
            // directory_paste_in_directory(
            //   directory_current(&tab->directory_history));
        }
        if (app.context_menu_open)
        {
            MainDropDownSelectionData data = {
                .window = app.window,
                .quick_access = &app.quick_access.items,
                .directory = directory_current(&tab->directory_history),
                .selected_paths =
                    &tab->directory_list.selected_item_values.paths,
            };
            app.context_menu_open =
                !drop_down_menu_add(&app.context_menu, true, &app, &data);
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

        const f32 ui_font_pixel_height = ui_context_get_font_pixel_height();

        if (preview_index != 1 && !event_get_key_event()->ctrl_pressed &&
            event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_2))
        {
#if 0
            if (tab->directory_list.selected_item_values.paths.size)
            {
#if 0
                platform_context_menu_create(
                    &menu,
                    tab->directory_list.selected_item_values.paths.data[0]);
#else
                platform_open_context(
                    app.window,
                    tab->directory_list.selected_item_values.paths.data[0]);
#endif
            }
            else
#endif
            {
                // app.context_menu_open = true;
                app.open_context_menu_window = true;
                app.context_menu.position = app.mouse_position;
                app.context_menu.position.x += 18.0f;
                app.context_menu.x = 0.0f;
                UiWindow* window = ui_window_get(app.context_menu_window);
                V2 item_size = v2f(250.0f, 24.0f + ui_font_pixel_height);
                const f32 end_height = CONTEXT_ITEM_COUNT * item_size.height;

                window->position = app.mouse_position;
                window->position.y =
                    min(app.dimensions.height - end_height - 10.0f,
                        window->position.y);
                window->position.x =
                    min(app.dimensions.width - item_size.width - 10.0f,
                        window->position.x);
            }
        }
        look_for_dropped_files(directory_current(&tab->directory_history),
                               directory_to_drop_in);

        const f32 top_bar_height = 24.0f + ui_font_pixel_height;
        const f32 top_bar_menu_height = 10.0f + ui_font_pixel_height;
        const f32 bottom_bar_height = 15.0f + ui_font_pixel_height;
        const f32 list_item_height = 16.0f + ui_font_pixel_height;

        AABB dock_space = { .min = v2f(0.0f,
                                       top_bar_height + top_bar_menu_height) };
        dock_space.size = v2_sub(app.dimensions, dock_space.min);
        dock_space.size.height -= bottom_bar_height;
        ui_context_begin(app.dimensions, &dock_space, app.delta_time,
                         check_collision);
        {
            const f32 drop_down_width = 350.0f;

            const DirectoryTab* current_tab = tab;
            const DirectoryPage* current_directory =
                directory_current(&tab->directory_history);

            const V4 button_color =
                v4a(v4_s_multi(global_get_clear_color(), 3.0f), 1.0f);

            if (app.open_menu_window)
            {
                application_open_menu_window(&app, drop_down_width,
                                             button_color);
            }
            if (app.open_windows_window)
            {
                application_open_windows_window(&app, drop_down_width,
                                                button_color);
            }

            if (app.open_font_change_window)
            {
                UiWindow* font_change_window =
                    ui_window_get(app.font_change_window);
                font_change_window->size = v2f(app.dimensions.width * 0.5f,
                                               app.dimensions.height * 0.9f);
                font_change_window->position =
                    v2f(middle(app.dimensions.width,
                               font_change_window->size.width),
                        middle(app.dimensions.height,
                               font_change_window->size.height));
                if (ui_window_begin(app.font_change_window, NULL,
                                    UI_WINDOW_OVERLAY |
                                        UI_WINDOW_FROSTED_GLASS))
                {
                    V2 list_position = v2f(10.0f, 10.0f);
                    i32 selected_item = -1;
                    if (ui_window_add_file_list(
                            list_position, list_item_height,
                            &app.font_change_directory.files, NULL,
                            &selected_item))
                    {
                        const char* path =
                            app.font_change_directory.files.data[selected_item]
                                .path;
                        ui_context_set_font_path(path);
                        ui_context_change_font_pixel_height(
                            ui_font_pixel_height);
                        app.open_font_change_window = false;
                        app.open_menu_window = true;
                    }
                    app.open_font_change_window = !ui_window_end();
                }
            }

            UiWindow* top_bar = ui_window_get(app.top_bar_window);
            top_bar->position = v2d();
            top_bar->size =
                v2f(app.dimensions.width, top_bar_height + top_bar_menu_height);

            if (ui_window_begin(app.top_bar_window, NULL, UI_WINDOW_NONE))
            {
                V2 drop_down_position = v2d();
                i32 index_clicked =
                    ui_window_add_menu_bar(&menu_values, &drop_down_position);
                if (index_clicked == 0)
                {
                    UiWindow* top_bar_menu = ui_window_get(app.menu_window);
                    top_bar_menu->position = drop_down_position;

                    app.open_windows_window = false;
                    app.open_menu_window = true;
                }
                else if (index_clicked == 1)
                {
                    UiWindow* top_bar_windows =
                        ui_window_get(app.windows_window);
                    top_bar_windows->position = drop_down_position;

                    app.open_menu_window = false;
                    app.open_windows_window = true;
                }

                AABB button_aabb = { 0 };
                button_aabb.size = v2i(34.4f),
                button_aabb.min = v2f(
                    10.0f, top_bar_menu_height - 2.0f +
                               middle(top_bar_height, button_aabb.size.height));

                ui_window_row_begin(0.0f);

                b8 disable = tab->directory_history.current_index <= 0;
                add_move_in_history_button(
                    &button_aabb, arrow_back_icon_co, disable,
                    FTIC_MOUSE_BUTTON_4, -1,
                    &tab->directory_list.selected_item_values,
                    &tab->directory_history);

                disable = tab->directory_history.history.size <=
                          tab->directory_history.current_index + 1;
                add_move_in_history_button(
                    &button_aabb, arrow_right_icon_co, disable,
                    FTIC_MOUSE_BUTTON_4, 1,
                    &tab->directory_list.selected_item_values,
                    &tab->directory_history);

                disable = !directory_can_go_up(
                    directory_current(&tab->directory_history)
                        ->directory.parent);
                if (ui_window_add_icon_button(button_aabb.min, button_aabb.size,
                                              global_get_highlight_color(),
                                              arrow_up_icon_co,
                                              UI_ARROW_ICON_TEXTURE, disable))
                {
                    directory_go_up(&tab->directory_history);
                }

                button_aabb.min.x += ui_window_row_end() + 10.0f;
                button_aabb.size.height = top_bar_height - 10.0f,
                button_aabb.min.y =
                    top_bar_menu_height +
                    middle(top_bar_height, button_aabb.size.height);
                button_aabb.size.width =
                    (app.dimensions.width - (search_bar_width + 20.0f)) -
                    button_aabb.min.x;

                b8 active_before = app.parent_directory_input.active;
                if (ui_window_add_input_field(button_aabb.min, button_aabb.size,
                                              &app.parent_directory_input))
                {
                    DirectoryPage* current =
                        directory_current(&tab->directory_history);
                    get_suggestions(button_aabb.min, current,
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
                    drop_down_menu_add(&app.suggestions, false, &app,
                                       &app.suggestion_data);

                    if (app.suggestion_data.change_directory ||
                        event_is_key_pressed_once(FTIC_KEY_ENTER))
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
                        if (!directory_go_to(
                                app.parent_directory_input.buffer.data,
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
                        app.suggestions.aabb = (AABB){ 0 };
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
                if (event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_LEFT))
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
                    search_page_clear_result(&app.search_page);
                    app.search_page
                        .running_callbacks[app.search_page.last_running_id] =
                        false;
                    if (!app.search_result_window_item.show)
                    {
                        open_window(app.dimensions,
                                    app.search_result_window_item.window);
                        app.search_result_window_item.show = true;
                        app.search_result_window_item.switch_on = true;
                    }
                }
                if (app.search_page
                        .running_callbacks[app.search_page.last_running_id])
                {
                    V2 dim = v2f(ui_font_pixel_height + 12.0f,
                                 ui_font_pixel_height + 10.0f);
                    if (ui_window_add_button(
                            v2f(button_aabb.min.x +
                                    (button_aabb.size.width - dim.width) - 5.0f,
                                button_aabb.min.y +
                                    middle(button_aabb.size.height,
                                           dim.height)),
                            &dim, &button_color, "X"))
                    {
                        app.search_page.running_callbacks
                            [app.search_page.last_running_id] = false;
                    }
                }
                if (app.search_page.input.active)
                {
                    if (event_is_key_pressed_once(FTIC_KEY_ENTER))
                    {
                        search_page_search(&app.search_page,
                                           &tab->directory_history,
                                           &app.thread_queue.task_queue);
                    }
                }
                else
                {
                    app.search_page
                        .running_callbacks[app.search_page.last_running_id] =
                        false;
                }

                ui_window_end();
            }

            UiWindow* bottom_bar = ui_window_get(app.bottom_bar_window);
            bottom_bar->position =
                v2f(0.0f, app.dimensions.height - bottom_bar_height);
            bottom_bar->size = v2f(app.dimensions.width, bottom_bar_height);
            if (ui_window_begin(app.bottom_bar_window, NULL, UI_WINDOW_NONE))
            {
                DirectoryPage* current =
                    directory_current(&tab->directory_history);
                char buffer[64] = { 0 };
                sprintf_s(buffer, sizeof(buffer), "Folders: %u  |  Files: %u",
                          current->directory.sub_directories.size,
                          current->directory.files.size);
                ui_window_add_text(v2f(10.0f, 1.0f), buffer, false);

                const V2 list_grid_icon_position = v2f(
                    bottom_bar->size.width - (2.0f * bottom_bar_height + 5.0f),
                    0);

                if (current->grid_view)
                {
                    const V2 slider_position =
                        v2f(list_grid_icon_position.x - 120.0f,
                            list_grid_icon_position.y +
                                middle(bottom_bar_height, 5.0f));

                    const V2 ui_icon_min_max = ui_get_big_icon_min_max();
                    static b8 pressed = false;
                    ui_set_big_icon_size(ui_window_add_slider(
                        slider_position, v2f(110.0f, 5.0f), ui_icon_min_max.min,
                        ui_icon_min_max.max, ui_get_big_icon_size(), &pressed));
                }

                ui_window_row_begin(0.0f);

                if (ui_window_add_icon_button(
                        list_grid_icon_position, v2i(bottom_bar_height),
                        global_get_border_color(), full_icon_co,
                        UI_LIST_ICON_TEXTURE, false))
                {
                    current->grid_view = false;
                }
                if (ui_window_add_icon_button(
                        list_grid_icon_position, v2i(bottom_bar_height),
                        global_get_border_color(), full_icon_co,
                        UI_GRID_ICON_TEXTURE, false))
                {
                    current->grid_view = true;
                }

                ui_window_row_end();

                ui_window_end();
            }

            access_panel_open(&app.quick_access, "Quick access",
                              list_item_height, &app.recent,
                              &tab->directory_history);

            access_panel_open(&app.recent.panel, "Recent", list_item_height,
                              &app.recent, &tab->directory_history);

            if (app.search_result_window_item.show)
            {
                app.search_result_window_item.show = !show_search_result_window(
                    &app.search_page, app.search_result_window_item.window,
                    list_item_height, &tab->directory_history);
                if (!app.search_result_window_item.show)
                {
                    app.search_result_window_item.switch_on = false;
                }
            }

            if (preview_index != -1)
            {
                if (preview_index == 0)
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
                    preview->top_color = global_get_clear_color();
                    preview->bottom_color = global_get_clear_color();

                    if (ui_window_begin(app.preview_window, NULL,
                                        UI_WINDOW_OVERLAY))
                    {
                        ui_window_add_image(v2d(), image_dimensions,
                                            preview_textures.data[0]);

                        if (ui_window_end()) preview_index = -1;
                    }
                }
                else if (preview_index == 1)
                {
                    AABB view_port = {
                        .min = v2i(0.0f),
                        .size = v2_s_sub(app.dimensions, 0.0f),
                    };
                    preview_camera.view_port = view_port;
                    preview_camera.view_projection.projection = perspective(
                        PI * 0.5f, view_port.size.width / view_port.size.height,
                        0.1f, 100.0f);
                    preview_camera.view_projection.view =
                        view(preview_camera.position,
                             v3_add(preview_camera.position,
                                    preview_camera.orientation),
                             preview_camera.up);
                    camera_update(&preview_camera, app.delta_time);
                }
                else
                {
                    UiWindow* preview = ui_window_get(app.preview_window);
                    preview->size = v2f(app.dimensions.width * 0.9f,
                                        app.dimensions.height * 0.9f);
                    preview->position = v2f(
                        middle(app.dimensions.width, preview->size.width),
                        middle(app.dimensions.height, preview->size.height));

                    preview->top_color = v4ic(0.0f);
                    preview->bottom_color = v4ic(0.0f);

                    if (ui_window_begin(app.preview_window, NULL,
                                        UI_WINDOW_OVERLAY))
                    {
                        ui_window_add_text_colored(v2f(10.0f, 10.0f),
                                                   &preview_file_colored, true);
                        if (ui_window_end()) preview_index = -1;
                    }
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
                    preview_file_colored.size = 0;
                }
            }

            for (u32 i = 0; i < app.tabs.size; ++i)
            {
                const u32 window_id = app.tabs.data[i].window_id;
                if (ui_window_in_focus() == window_id)
                {
                    app.tab_index = i;
                }

                ui_window_get(window_id)->alpha =
                    i == app.tab_index ? 1.0f : 0.7f;

                if (show_directory_window(
                        window_id, list_item_height, check_collision,
                        app.open_context_menu_window, app.dimensions,
                        app.render_3d.shader_properties.shader, &app.recent,
                        &app.thread_queue.task_queue, app.tabs.data + i))
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
            if (current_tab == tab)
            {
                const DirectoryPage* next_directory =
                    directory_current(&tab->directory_history);
                if (current_directory != next_directory)
                {
                    ui_window_set_end_scroll_offset(tab->window_id,
                                                    next_directory->offset);
                    ui_window_set_current_scroll_offset(tab->window_id, 0.0f);
                }
            }
            for (u32 i = 0; i < app.tabs.size; ++i)
            {
                directory_current(&app.tabs.data[i].directory_history)->offset =
                    ui_window_get(app.tabs.data[i].window_id)
                        ->end_scroll_offset;
            }

            if (app.open_context_menu_window)
            {
                application_open_context_menu_window(&app, tab);
            }
        }
        ui_context_end();

        rendering_properties_check_and_grow_vertex_buffer(&app.main_render);
        rendering_properties_check_and_grow_index_buffer(&app.main_render,
                                                         app.main_index_count);

        buffer_set_sub_data(app.main_render.render.vertex_buffer_id,
                            GL_ARRAY_BUFFER, 0,
                            sizeof(Vertex) * app.main_render.vertices.size,
                            app.main_render.vertices.data);

        AABB whole_screen_scissor = { .size = app.dimensions };

        render_begin_draw(&app.main_render.render,
                          app.main_render.render.shader_properties.shader,
                          &app.mvp);
        render_draw(0, app.main_index_count, &whole_screen_scissor);
        render_end_draw(&app.main_render.render);

        if (preview_index == 1)
        {
#if 0
            glViewport((int)roundf(preview_camera.view_port.min.x),
                       (int)roundf(preview_camera.view_port.min.y),
                       (int)roundf(preview_camera.view_port.size.width),
                       (int)roundf(preview_camera.view_port.size.height));
#endif

            MVP mvp = {
                .model = m4i(1.0f),
                .view = preview_camera.view_projection.view,
                .projection = preview_camera.view_projection.projection,
            };

            glEnable(GL_DEPTH_TEST);
            render_begin_draw(&app.render_3d,
                              app.render_3d.shader_properties.shader, &mvp);

            glUniform3f(app.light_dir_location, -preview_camera.orientation.x,
                        -preview_camera.orientation.y,
                        -preview_camera.orientation.z);

            render_draw(0, app.index_count_3d, &preview_camera.view_port);
            render_end_draw(&app.render_3d);
            glDisable(GL_DEPTH_TEST);
            if (event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_LEFT))
            {
                preview_index = -1;
            }

            if (event_is_key_pressed_once(FTIC_KEY_R))
            {
                preview_camera = camera_create_default();
            }
        }

#endif
        for (u32 i = 0; i < app.tabs.size; ++i)
        {
            DirectoryTab* current = app.tabs.data + i;
            if (directory_look_for_directory_change(
                    current->directory_history.change_handle))
            {
                directory_reload(
                    directory_current(&current->directory_history));
                directory_history_update_directory_change_handle(
                    &current->directory_history);
            }
        }

        application_end_frame(&app);
    }
    if (current_path) free(current_path);

    // TODO: Cleanup of all
    application_uninitialize(&app);
}
