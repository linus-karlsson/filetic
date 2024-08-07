#include "application.h"
#include "hash.h"
#include "logging.h"
#include "texture.h"
#include "opengl_util.h"
#include "object_load.h"
#include "random.h"
#include "globals.h"
#include "theme.h"
#include <ctype.h>
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

#define icon_1_co(width, height)                                                                   \
    {                                                                                              \
        .x = 0.0f / width,                                                                         \
        .y = 0.0f / height,                                                                        \
        .z = height / width,                                                                       \
        .w = height / height,                                                                      \
    };

global const V4 file_icon_co = icon_1_co(512.0f, 128.0f);
global const V4 arrow_back_icon_co = icon_1_co(72.0f, 24.0f);

#define icon_2_co(width, height)                                                                   \
    {                                                                                              \
        .x = height / width,                                                                       \
        .y = 0.0f / height,                                                                        \
        .z = (height * 2.0f) / width,                                                              \
        .w = height / height,                                                                      \
    };
global const V4 folder_icon_co = icon_2_co(512.0f, 128.0f);
global const V4 arrow_up_icon_co = icon_2_co(72.0f, 24.0f);

#define icon_3_co(width, height)                                                                   \
    {                                                                                              \
        .x = (height * 2.0f) / width,                                                              \
        .y = 0.0f / height,                                                                        \
        .z = (height * 3.0f) / width,                                                              \
        .w = height / height,                                                                      \
    };
global const V4 pdf_icon_co = icon_3_co(512.0f, 128.0f);
global const V4 arrow_right_icon_co = icon_3_co(72.0f, 24.0f);

#define icon_4_co(width, height)                                                                   \
    {                                                                                              \
        .x = (height * 3.0f) / width,                                                              \
        .y = 0.0f / height,                                                                        \
        .z = (height * 4.0f) / width,                                                              \
        .w = height / height,                                                                      \
    };
global const V4 png_icon_co = icon_4_co(512.0f, 128.0f);

typedef struct DropDownLayout
{
    UiLayout ui_layout;
    f32 width;
    f32 switch_x;
    f32 text_y_offset;
} DropDownLayout;

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

internal DropDownLayout drop_down_layout_create(const f32 width, const V2 at)
{
    DropDownLayout layout = {
        .width = width,
    };
    layout.ui_layout = ui_layout_create(at);
    const V2 switch_size = ui_window_get_switch_size();
    layout.switch_x = layout.width - switch_size.width - layout.ui_layout.at.x;
    return layout;
}

internal V2 drop_down_layout_text_position(DropDownLayout* layout)
{
    return v2f(layout->ui_layout.at.x, layout->ui_layout.at.y + layout->text_y_offset);
}

internal V2 drop_down_layout_switch_right_align_position(DropDownLayout* layout)
{
    return v2f(layout->switch_x, layout->ui_layout.at.y);
}

internal V2 drop_down_layout_right_align_position(DropDownLayout* layout, const f32 item_width)
{
    const f32 x_position = layout->width - item_width - layout->ui_layout.at.x;
    return v2f(x_position, layout->ui_layout.at.y);
}

internal void drop_down_layout_add_switch_with_text(DropDownLayout* layout, const char* text,
                                                    b8* selected, f32* x)
{
    ui_window_add_text(drop_down_layout_text_position(layout), text, false, &layout->ui_layout);
    ui_window_add_switch(drop_down_layout_switch_right_align_position(layout), selected, x,
                         &layout->ui_layout);
    ui_layout_row(&layout->ui_layout);
}

internal void drop_down_layout_add_radio_button_with_text(DropDownLayout* layout, const char* text,
                                                          b8* selected)
{
    ui_window_add_text(drop_down_layout_text_position(layout), text, false, &layout->ui_layout);
    ui_window_add_radio_button(drop_down_layout_right_align_position(layout, 16.0f), v2i(16.0f),
                               selected, &layout->ui_layout);
    ui_layout_row(&layout->ui_layout);
}

internal void drop_down_layout_add_line(DropDownLayout* layout)
{
    ui_window_add_rectangle(layout->ui_layout.at,
                            v2f(layout->width - (2.0f * layout->ui_layout.start_x), 1.0f),
                            global_get_border_color(), &layout->ui_layout);
    ui_layout_row(&layout->ui_layout);
}

internal b8 drop_down_layout_add_reset_button(DropDownLayout* layout, const V4 button_color)
{
    drop_down_layout_add_line(layout);
    const char* text = "Reset to default";
    const V2 button_dim = ui_window_get_button_dimensions(v2d(), text, NULL);
    const f32 x_position = middle(layout->width, button_dim.width);
    b8 result = ui_window_add_button(v2f(x_position, layout->ui_layout.at.y), NULL, &button_color,
                                     text, &layout->ui_layout);
    ui_layout_row(&layout->ui_layout);
    return result;
}

internal u32 drop_down_layout_add_increment_width_text(DropDownLayout* layout, const char* text,
                                                       const V4 button_color, const u32 value)
{
    u32 result = 0;

    const V2 button1_dim = ui_window_get_button_dimensions(v2d(), "-", NULL);
    const V2 button2_dim = ui_window_get_button_dimensions(v2d(), "+", NULL);

    char buffer[20] = { 0 };
    value_to_string(buffer, "%u", value);

    const FontTTF* ui_font = ui_context_get_font();
    const f32 x_advance = text_x_advance(ui_font->chars, buffer, (u32)strlen(buffer), 1.0f);
    const f32 row_padding = 10.0f;
    const f32 total_width =
        button1_dim.width + button2_dim.width + x_advance + (row_padding * 2.0f);

    V2 text_position = drop_down_layout_text_position(layout);
    ui_window_add_text(text_position, text, false, &layout->ui_layout);

    layout->ui_layout.at = drop_down_layout_right_align_position(layout, total_width);
    if (ui_window_add_button(layout->ui_layout.at, NULL, &button_color, "-", &layout->ui_layout))
    {
        result = 1;
    }
    ui_layout_column(&layout->ui_layout);
    text_position.x = layout->ui_layout.at.x;
    ui_window_add_text(text_position, buffer, false, &layout->ui_layout);
    ui_layout_column(&layout->ui_layout);
    if (ui_window_add_button(layout->ui_layout.at, NULL, &button_color, "+", &layout->ui_layout))
    {
        result = 2;
    }

    ui_layout_row(&layout->ui_layout);
    ui_layout_reset_column(&layout->ui_layout);
    return result;
}

internal void set_gldebug_log_level(DebugLogLevel level)
{
    g_debug_log_level = level;
}

internal void opengl_log_message(GLenum source, GLenum type, GLuint id, GLenum severity,
                                 GLsizei length, const GLchar* message, const void* userParam)
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

internal void add_move_in_history_button(UiLayout* layout, const V4 icon_co, const b8 disable,
                                         const int mouse_button, const i32 history_add,
                                         SelectedItemValues* selected_item_values,
                                         DirectoryHistory* directory_history)
{
    if (ui_window_add_icon_button(layout->at, v2i(layout->column_width),
                                  global_get_highlight_color(), icon_co, UI_ARROW_ICON_TEXTURE,
                                  disable, layout))
    {
        directory_move_in_history(history_add, selected_item_values, directory_history);
    }
    if (!disable && event_is_mouse_button_clicked(mouse_button))
    {
        directory_move_in_history(history_add, selected_item_values, directory_history);
    }
}

internal void recent_panel_add_item(RecentPanel* recent, const DirectoryItem* item)
{
    if (platform_directory_exists(item->path))
    {
        DirectoryItemArray* items = &recent->panel.items;
        for (u32 i = 0; i < items->size; ++i)
        {
            if (guid_compare(items->data[i].id, item->id) == 0)
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
        DirectoryItem item_to_add = *item;
        item_to_add.id = item->id;
        item_to_add.path = string_copy_d(item_to_add.path);
        item_to_add.name_offset = item->name_offset;
        if (items->size == recent->total)
        {
            free(array_back(items)->path);
            items->size--;
        }
        array_push(items, item_to_add);
        for (i32 i = (i32)items->size - 1; i >= 1; --i)
        {
            items->data[i] = items->data[i - 1];
        }
        items->data[0] = item_to_add;
    }
}

internal char* get_parent_directory_name(DirectoryPage* current)
{
    return current->directory.parent +
           get_path_length(current->directory.parent, (u32)strlen(current->directory.parent));
}

internal void tab_clear_selected(DirectoryTab* tab)
{
    if (tab->directory_list.selected_item_values.last_selected)
    {
        free(tab->directory_list.selected_item_values.last_selected);
        tab->directory_list.selected_item_values.last_selected = NULL;
    }
    directory_clear_selected_items(&tab->directory_list.selected_item_values);
    tab->directory_list.last_selected_index = 0;
    for (u32 i = 0; i < tab->directory_list.inputs.size; ++i)
    {
        tab->directory_list.inputs.data[i].active = false;
    }
}

internal void render_3d_initialize(Render* render, const u32 shader, int* light_dir_location)
{
    VertexBufferLayout vertex_buffer_layout = default_vertex_3d_buffer_layout();
    u32 default_texture = create_default_texture();

    *light_dir_location = glGetUniformLocation(shader, "light_dir");
    ftic_assert((*light_dir_location) != -1);

    const u32 texture_count = 2;
    U32Array textures = { 0 };
    array_create(&textures, texture_count);
    array_push(&textures, default_texture);

    *render = render_create(shader, textures, &vertex_buffer_layout, vertex_buffer_create(),
                            index_buffer_create());

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
        for (u32 j = 0; j < current->directory.items.size; ++j)
        {
            DirectoryItem* current_item = current->directory.items.data + j;
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
            item->texture_id =
                texture_create(&texture->texture_properties, GL_RGBA8, GL_RGBA, GL_LINEAR);
            item->texture_width = (u16)texture->texture_properties.width;
            item->texture_height = (u16)texture->texture_properties.height;
            item->reload_thumbnail = false;
        }
        free(texture->texture_properties.bytes);
    }
    tab->textures.array.size = 0;
    platform_mutex_unlock(&tab->textures.mutex);
}

internal u32 mesh_3d_render_to_2d_texture(const Mesh3D* mesh, const AABB3D* mesh_aabb,
                                          const i32 width, const i32 height,
                                          const AABB* view_port_before, const u32 shader)
{
    u32 resulting_texture = 0;

    u32 fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    u32 multisampled_texture;
    glGenTextures(1, &multisampled_texture);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, multisampled_texture);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, (GLsizei)width, (GLsizei)height,
                            GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                           multisampled_texture, 0);

    u32 rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, (GLsizei)width,
                                     (GLsizei)height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
    {
        u32 resolve_fbo;
        glGenFramebuffers(1, &resolve_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, resolve_fbo);

        glGenTextures(1, &resulting_texture);
        glBindTexture(GL_TEXTURE_2D, resulting_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               resulting_texture, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
        {
            int light_dir_location = -1;
            Render temp_render = { 0 };
            render_3d_initialize(&temp_render, shader, &light_dir_location);

            vertex_buffer_orphan(temp_render.vertex_buffer_id,
                                 mesh->vertices.size * sizeof(Vertex3D), GL_STATIC_DRAW,
                                 mesh->vertices.data);
            index_buffer_orphan(temp_render.index_buffer_id, mesh->indices.size * sizeof(u32),
                                GL_STATIC_DRAW, mesh->indices.data);

            Camera camera = camera_create_default();
            camera_set_based_on_mesh_aabb(&camera, mesh_aabb);

            const AABB view_port = { .size = v2i((f32)width) };
            camera.view_port = view_port;
            camera.view_projection.projection =
                perspective(PI * 0.5f, view_port.size.width / view_port.size.height, 0.1f, 100.0f);
            camera.view_projection.view =
                view(camera.position, v3_add(camera.position, camera.orientation), camera.up);

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

            render_begin_draw(&temp_render, temp_render.shader_properties.shader, &mvp);

            glUniform3f(light_dir_location, -camera.orientation.x, -camera.orientation.y,
                        -camera.orientation.z);

            render_draw(0, mesh->indices.size, &camera.view_port);
            render_end_draw(&temp_render);

            glDisable(GL_DEPTH_TEST);

            glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolve_fbo);
            glBlitFramebuffer(0, 0, (GLsizei)width, (GLsizei)height, 0, 0, (GLsizei)width,
                              (GLsizei)height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

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

internal void look_for_and_load_object_thumbnails(const V2 dimensions, const u32 shader,
                                                  DirectoryTab* tab)
{
    DirectoryPage* current = directory_current(&tab->directory_history);
    platform_mutex_lock(&tab->objects.mutex);
    for (u32 i = 0; i < tab->objects.array.size; ++i)
    {
        ObjectThumbnail* object = tab->objects.array.data + i;
        DirectoryItem* item = NULL;
        for (u32 j = 0; j < current->directory.items.size; ++j)
        {
            DirectoryItem* current_item = current->directory.items.data + j;
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
            item->texture_id =
                mesh_3d_render_to_2d_texture(&object->mesh, &object->mesh_aabb, item->texture_width,
                                             item->texture_height, &view_port_before, shader);

            item->reload_thumbnail = false;
        }
        array_free(&object->mesh.vertices);
        array_free(&object->mesh.indices);
    }
    tab->objects.array.size = 0;
    platform_mutex_unlock(&tab->objects.mutex);
}

internal void add_arrow_icon(const V2 at, const V2 button_size, const u32 sort_count)
{
    f32 sort_icon = 0.0f;
    if (sort_count == 1)
    {
        sort_icon = UI_ARROW_DOWN_ICON_TEXTURE;
    }
    else if (sort_count == 2)
    {
        sort_icon = UI_ARROW_UP_ICON_TEXTURE;
    }
    V2 arrow_position =
        v2f(at.x + button_size.width - 24.0f, at.y + middle(button_size.height, 12.0f));
    ui_window_add_icon(arrow_position, v2i(12.0f), full_icon_co, sort_icon, NULL);
}

internal void sort_directory(const SortBy sort_by, DirectoryTab* tab)
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

internal b8 application_show_directory_window(ApplicationContext* app, const u32 window,
                                              const f32 list_item_height, const b8 check_collision,
                                              char** item_hit, DirectoryTab* tab)
{
    DirectoryPage* current = directory_current(&tab->directory_history);

    look_for_and_load_image_thumbnails(tab);
    look_for_and_load_object_thumbnails(app->dimensions, app->render_3d.shader_properties.shader,
                                        tab);

    if (ui_window_begin(window, get_parent_directory_name(current),
                        UI_WINDOW_TOP_BAR | UI_WINDOW_RESIZEABLE))
    {

        V2 position = v2f(0.0f, 0.0f);
        V2 button_size = ui_window_get_button_dimensions(v2d(), "D", NULL); // get height only
        const UiWindow* ui_window = ui_window_get(window);
        button_size.width = ui_window->size.width / 3.0f;

        tab->directory_list.item_selected = false;

        i32 hit_index = -1;
        i32 selected_item = -1;

        UiLayout layout = ui_layout_create(v2f(10.0f, position.y + button_size.height + 5.0f));
        if (current->grid_view)
        {
            selected_item = ui_window_add_directory_item_grid(
                v2f(0.0f, layout.at.y), &current->directory.items, &app->thread_queue.task_queue,
                &tab->textures, &tab->objects, &hit_index, &tab->directory_list);

            if (selected_item != -1)
            {
                DirectoryItem* item = current->directory.items.data + selected_item;

                if (item->type == FOLDER_DEFAULT)
                {
                    recent_panel_add_item(&app->recent, item);
                    directory_open_folder(item->id, &tab->directory_history);
                    tab_clear_selected(tab);
                }
                else
                {
                    platform_open_file(item->path);
                }
            }
        }
        else
        {
            selected_item = ui_window_add_directory_item_list(
                layout.at, list_item_height, &current->directory.items, &tab->directory_list,
                &hit_index, &layout);
            if (selected_item != -1)
            {
                DirectoryItem* item = current->directory.items.data + selected_item;

                if (item->type == FOLDER_DEFAULT)
                {
                    recent_panel_add_item(&app->recent, item);
                    directory_open_folder(item->id, &tab->directory_history);
                    tab_clear_selected(tab);
                }
                else
                {
                    platform_open_file(item->path);
                }
            }
        }
        if (hit_index != -1 && selected_item == -1 &&
            current->directory.items.data[hit_index].type == FOLDER_DEFAULT)
        {
            *item_hit = current->directory.items.data[hit_index].path;
        }
        if (!tab->directory_list.item_selected && !tab->directory_list.inputs.data[0].active)
        {
            if ((!app->open_context_menu_window &&
                 event_is_mouse_button_pressed_once(FTIC_MOUSE_BUTTON_LEFT)) ||
                event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_RIGHT))
            {
                tab_clear_selected(tab);
            }
        }
        V4 color = global_get_clear_color();
        color.a = 0.95f;
        layout.at = position;
        layout.row_height = 0.0f;

        if (ui_window_add_button(layout.at, &button_size, &color, "Name", &layout))
        {
            sort_directory(SORT_NAME, tab);
        }
        if (current->sort_by == SORT_NAME)
        {
            add_arrow_icon(layout.at, button_size, current->sort_count);
        }

        ui_layout_column(&layout);
        if (ui_window_add_button(layout.at, &button_size, &color, "Date", &layout))
        {
            sort_directory(SORT_DATE, tab);
        }
        if (current->sort_by == SORT_DATE)
        {
            add_arrow_icon(layout.at, button_size, current->sort_count);
        }

        ui_layout_column(&layout);
        if (ui_window_add_button(layout.at, &button_size, &color, "Size", &layout))
        {
            sort_directory(SORT_SIZE, tab);
        }
        if (current->sort_by == SORT_SIZE)
        {
            add_arrow_icon(layout.at, button_size, current->sort_count);
        }

        return ui_window_end();
    }
    return false;
}

internal b8 drop_down_menu_add(DropDownMenu2* drop_down_menu, const ApplicationContext* application,
                               void* option_data)
{
    const u32 drop_down_item_count = drop_down_menu->options.size;
    if (drop_down_item_count == 0) return true;

    drop_down_menu->x += (f32)(application->delta_time * 5.0);
    drop_down_menu->x = ftic_clamp_high(drop_down_menu->x, 1.0f);

    const f32 drop_down_item_height = 40.0f;
    const f32 end_height = drop_down_item_count * drop_down_item_height;
    const f32 current_y = ease_out_cubic(drop_down_menu->x) * end_height;
    const f32 precent = current_y / end_height;
    const f32 drop_down_width = 200.0f;
    const f32 drop_down_border_width = 1.0f;
    const f32 border_extra_padding = drop_down_border_width * 2.0f;
    const f32 drop_down_outer_padding = 10.0f + border_extra_padding;
    const V2 end_position =
        v2f(drop_down_menu->position.x + drop_down_width + drop_down_outer_padding,
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
             v2f(drop_down_width + border_extra_padding, current_y + border_extra_padding),
             v4a(global_get_clear_color(), 0.92f), 0.0f);
    *drop_down_menu->index_count += 6;

    quad_border(&drop_down_menu->render->vertices, drop_down_menu->index_count,
                drop_down_menu->aabb.min, drop_down_menu->aabb.size, global_get_secondary_color(),
                drop_down_border_width, 0.0f);

    b8 item_clicked = false;

    b8 mouse_button_pressed = event_is_mouse_button_pressed_once(FTIC_MOUSE_BUTTON_1);

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
        if (collision_point_in_aabb(application->mouse_position, &drop_down_item_aabb))
        {
            if (drop_down_menu->tab_index == -1 || event_get_mouse_move_event()->activated)
            {
                drop_down_item_hit = true;
                drop_down_menu->tab_index = -1;
                any_drop_down_item_hit = true;
                item_clicked = mouse_button_pressed;
            }
        }

        if (drop_down_item_hit)
        {
            quad(&drop_down_menu->render->vertices, promt_item_position, drop_down_item_aabb.size,
                 global_get_border_color(), 0.0f);
            *drop_down_menu->index_count += 6;
        }

        V2 promt_item_text_position = promt_item_position;
        promt_item_text_position.y +=
            application->font.pixel_height + promt_item_text_padding + 3.0f;
        promt_item_text_position.x += promt_item_text_padding;

        V4 text_color = v4i(1.0f);
        should_close = drop_down_menu->menu_options_selection(
            i, drop_down_item_hit, should_close, item_clicked, &text_color, option_data);

        *drop_down_menu->index_count += text_generation_color(
            application->font.chars, drop_down_menu->options.data[i], 1.0f,
            promt_item_text_position, 1.0f, application->font.pixel_height * precent, text_color,
            NULL, NULL, NULL, &drop_down_menu->render->vertices);

        promt_item_position.y += drop_down_item_aabb.size.y;
    }
    return should_close || ((!any_drop_down_item_hit) && mouse_button_pressed) ||
           event_is_key_clicked(FTIC_KEY_ESCAPE);
}

internal void get_suggestions(const V2 position, DirectoryPage* current,
                              InputBuffer* parent_directory_input, DropDownMenu2* suggestions,
                              SuggestionSelectionData* suggestion_data)
{
    char* path = parent_directory_input->buffer.data;
    u32 current_directory_len = get_path_length(path, parent_directory_input->buffer.size);
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
        directory = platform_get_directory(path, current_directory_len, false);
        path[current_directory_len--] = saved_chars[2];
        path[current_directory_len--] = saved_chars[1];

        suggestions->tab_index = -1;
    }
    path[current_directory_len] = saved_chars[0];

    for (u32 i = 0; i < directory.items.size; ++i)
    {
        DirectoryItem* dir_item = directory.items.data + i;
        dir_item->size =
            string_span_case_insensitive(item_name(dir_item), path + current_directory_len + 1);
    }
    directory_sort_by_size(&directory.items);
    directory_flip_array(&directory.items);

    for (u32 i = 0; i < suggestion_data->items.size; ++i)
    {
        free(suggestion_data->items.data[i].path);
    }
    suggestions->options.size = 0;
    suggestion_data->items.size = 0;
    const u32 item_count = min(directory.items.size, 6);
    for (u32 i = 0; i < item_count; ++i)
    {
        array_push(&suggestions->options, item_name(directory.items.data + i));
        array_push(&suggestion_data->items, directory.items.data[i]);
    }

    for (u32 i = item_count; i < directory.items.size; ++i)
    {
        free(directory.items.data[i].path);
    }
    array_free(&directory.items);
    free(directory.parent);

    const FontTTF* ui_font = ui_context_get_font();
    const f32 x_advance = text_x_advance(ui_font->chars, parent_directory_input->buffer.data,
                                         parent_directory_input->buffer.size, 1.0f);

    suggestions->position = v2f(position.x + x_advance, position.y + ui_font->pixel_height + 20.0f);
}

internal void set_input_buffer_to_current_directory(const char* path, InputBuffer* input)
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
    "int", "char", "u64", "u32", "u16", "u8", "b8", "i32", "f32", "f64", "V2", "V3", "V4", "void",
};

global const char* define_key_words[] = {
    "#include", "#define", "global", "internal", "typedef",
};

internal b8 search_keywords(const char** keywords, const u32 keyword_count, const CharArray* word,
                            UU32* start_end)
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
                if (word->data[start_end->second] != ')' && word->data[start_end->second] != '*')
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
    b8 found = search_keywords(primitive_key_words, static_array_size(primitive_key_words), word,
                               start_end);

    if (found)
    {
        *color = v4f(1.0f, 0.34117f, 0.2f, 1.0f);
    }
    else
    {
        found = search_keywords(user_key_words, static_array_size(user_key_words), word, start_end);
        if (found)
        {
            *color = v4f(1.0f, 0.527f, 0.0f, 1.0f);
        }
        else
        {
            found = search_keywords(define_key_words, static_array_size(define_key_words), word,
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

u32 load_object(Render* render, const char* path, AABB3D* preview_mesh_aabb)
{
    Mesh3D mesh = { 0 };
    *preview_mesh_aabb = mesh_3d_load(&mesh, path, 0.0f);
    vertex_buffer_orphan(render->vertex_buffer_id, mesh.vertices.size * sizeof(Vertex3D),
                         GL_STATIC_DRAW, mesh.vertices.data);
    index_buffer_orphan(render->index_buffer_id, mesh.indices.size * sizeof(u32), GL_STATIC_DRAW,
                        mesh.indices.data);
    u32 index_count = mesh.indices.size;

    array_free(&mesh.indices);
    array_free(&mesh.vertices);
    return index_count;
}

internal b8 main_drop_down_selection(u32 index, b8 hit, b8 should_close, b8 item_clicked,
                                     V4* text_color, void* data)
{
    MainDropDownSelectionData* arguments = (MainDropDownSelectionData*)data;
    switch (index)
    {
        case COPY_OPTION_INDEX:
        {
            if (arguments->selected_paths->size == 0 && arguments->panel == NULL)
            {
                *text_color = global_get_tab_color();
            }
            else if (item_clicked && hit)
            {
                if (arguments->panel)
                {
                    char* paths[1] = {
                        arguments->panel->items.data[arguments->panel->list.selected_item].path
                    };
                    CharPtrArray array = { .size = 1, .capacity = 1, .data = paths };
                    platform_copy_to_clipboard(&array);
                }
                else
                {
                    platform_copy_to_clipboard(arguments->selected_paths);
                }
                should_close = true;
            }
            break;
        }
        case PASTE_OPTION_INDEX:
        {
            if (platform_clipboard_is_empty())
            {
                *text_color = global_get_tab_color();
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
                *text_color = global_get_tab_color();
            }
            else if (item_clicked && hit)
            {
                platform_delete_files(arguments->selected_paths);
                directory_reload(arguments->directory);

                should_close = true;
            }
            break;
        }
        case ADD_TO_QUICK_OPTION_INDEX:
        {
            if (arguments->selected_paths->size == 0 && arguments->panel == NULL)
            {
                *text_color = global_get_tab_color();
            }
            else if (item_clicked && hit)
            {
                if (arguments->panel)
                {
                    DirectoryItemArray* items = &arguments->panel->items;
                    const i32 selected_item = arguments->panel->list.selected_item;
                    DirectoryItem item_to_remove = items->data[selected_item];
                    for (u32 i = selected_item; i < items->size - 1; ++i)
                    {
                        items->data[i] = items->data[i + 1];
                    }
                    items->size--;
                    free(item_to_remove.path);
                }
                else
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
                            if (!strcmp(path_temp, arguments->quick_access->data[j].path))
                            {
                                exist = true;
                                break;
                            }
                        }
                        if (!exist)
                        {
                            const u32 length = (u32)strlen(path_temp);
                            char* path = (char*)calloc(length + 1, sizeof(char));
                            memcpy(path, path_temp, length);
                            DirectoryItem item = {
                                .path = path,
                                .name_offset = (u16)get_path_length(path, length),
                            };
                            array_push(arguments->quick_access, item);
                        }
                    }
                }
                should_close = true;
            }
            break;
        }
        case PROPERTIES_OPTION_INDEX:
        {
            if (arguments->selected_paths->size == 0)
            {
                *text_color = global_get_tab_color();
            }
            else if (item_clicked && hit)
            {
                platform_show_properties(10, 10, arguments->selected_paths->data[0]);
                should_close = true;
            }
            break;
        }
        case OPEN_IN_TERMINAL_OPTION_INDEX:
        {
            if (item_clicked && hit)
            {
                should_close = true;
                if (arguments->panel)
                {
                    char* path =
                        arguments->panel->items.data[arguments->panel->list.selected_item].path;
                    if (platform_directory_exists(path))
                    {
                        platform_open_terminal(path);
                    }
                }
                else
                {
                    if (arguments->selected_paths->size != 0)
                    {
                        if (platform_directory_exists(arguments->selected_paths->data[0]))
                        {
                            platform_open_terminal(arguments->selected_paths->data[0]);
                            break;
                        }
                    }
                    platform_open_terminal(arguments->directory->directory.parent);
                }
            }
            break;
        }
        case MORE_OPTION_INDEX:
        {
            if (arguments->selected_paths->size == 0)
            {
                *text_color = global_get_tab_color();
            }
            else if (item_clicked && hit)
            {
                platform_open_context(arguments->window, arguments->selected_paths->data[0]);
                should_close = true;
            }
            break;
        }
        default: break;
    }
    return should_close;
}

internal void suggestions_add_extra_space(SuggestionSelectionData* arguments)
{
    array_push(arguments->parent_directory, '\0');
    array_push(arguments->parent_directory, '\0');
    array_push(arguments->parent_directory, '\0');
    arguments->parent_directory->size -= 3;
    *arguments->cursor_index = arguments->parent_directory->size;
}

internal b8 suggestion_selection(u32 index, b8 hit, b8 should_close, b8 item_clicked,
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
            suggestions_add_extra_space(arguments);
            arguments->change_directory = item_clicked;
            return item_clicked;
        }
    }
    return should_close;
}

internal void open_window(const V2 dimensions, const u32 window_id)
{
    V2 end_size = v2i(200.0f);
    V2 end_position =
        v2f(middle(dimensions.width, end_size.width), middle(dimensions.height, end_size.height));

    ui_window_set_size(window_id, v2i(0.0f));
    ui_window_set_position(window_id, v2_s_multi(dimensions, 0.5f));
    ui_window_start_size_animation(window_id, end_size);
    ui_window_start_position_animation(window_id, end_position);
}

internal void save_application_state(ApplicationContext* app)
{
    char full_path_buffer[FTIC_MAX_PATH] = { 0 };
    append_full_path("saved/application.txt", full_path_buffer);
    FILE* file = fopen(full_path_buffer, "wb");
    if (file == NULL)
    {
        log_file_error(full_path_buffer);
        return;
    }

    fwrite(&app->tabs.size, sizeof(app->tabs.size), 1, file);
    for (u32 i = 0; i < app->tabs.size; ++i)
    {
        FticGUID id = directory_current(&app->tabs.data[i].directory_history)->directory.parent_id;
        fwrite(&id, sizeof(FticGUID), 1, file);
        fwrite(&app->tabs.data[i].window_id, sizeof(app->tabs.data[i].window_id), 1, file);
    }
    fclose(file);
}

internal void load_application_state(ApplicationContext* app)
{
    char full_path_buffer[FTIC_MAX_PATH] = { 0 };
    append_full_path("saved/application.txt", full_path_buffer);
    FILE* file = fopen(full_path_buffer, "rb");
    if (file == NULL)
    {
        log_file_error(full_path_buffer);
        return;
    }

    u32 size = 0;
    if (!fread(&size, sizeof(size), 1, file))
    {
        return;
    }
    for (u32 i = 0; i < size; ++i)
    {
        FticGUID id = { 0 };
        fread(&id, sizeof(FticGUID), 1, file);
        u32 window_id = 0;
        fread(&window_id, sizeof(window_id), 1, file);

        char* path = platform_get_path_from_id(id);
        u32 path_length = (u32)strlen(path);
        if (path && platform_directory_exists(path))
        {
            path[path_length++] = '\\';
            path[path_length++] = '*';

            DirectoryTab tab = { 0 };
            array_push(&app->tabs, tab);
            directory_tab_add(path, &app->thread_queue.task_queue, array_back(&app->tabs));

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

    u32 buffer_offset = 0;
    u32 buffer_size = panel->items.size * sizeof(FticGUID);
    char* buffer = (char*)calloc(panel->items.size, sizeof(FticGUID));
    for (u32 i = 0; i < panel->items.size; ++i)
    {
        DirectoryItem* item = panel->items.data + i;
        for (u32 j = 0; j < static_array_size(item->id.bytes); ++j)
        {
            buffer[buffer_offset++] = item->id.bytes[j];
        }
    }
    file_write(file_path, buffer, buffer_offset);
    free(buffer);
}

internal void access_panel_load(AccessPanel* panel, const char* file_path)
{
    FileAttrib file = file_read(file_path);
    if (!file.buffer)
    {
        return;
    }
    while (!file_end_of_file(&file))
    {
        FticGUID guid = { 0 };
        for (u32 i = 0; i < static_array_size(guid.bytes); ++i)
        {
            guid.bytes[i] = file.buffer[file.current_pos++];
        }
        DirectoryItem item = {
            .id = guid,
            .path = platform_get_path_from_id(guid),
        };
        if (item.path)
        {
            item.name_offset = (u16)get_path_length(item.path, (u32)strlen(item.path));
            array_push(&panel->items, item);
        }
    }
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
    panel->list.selected_item = -1;
}

internal b8 access_panel_open(AccessPanel* panel, const char* title, const f32 list_item_height,
                              char** item_hit, RecentPanel* recent,
                              DirectoryHistory* directory_history)
{
    if (panel->menu_item.show)
    {
        if (ui_window_begin(panel->menu_item.window, title,
                            UI_WINDOW_TOP_BAR | UI_WINDOW_RESIZEABLE))
        {
            i32 hit_index = -1;
            V2 list_position = v2i(10.0f);
            if (ui_window_add_movable_list(list_position, &panel->items, &hit_index, &panel->list))
            {
                directory_open_folder(panel->items.data[panel->list.selected_item].id,
                                      directory_history);

                recent_panel_add_item(recent, &panel->items.data[panel->list.selected_item]);
            }
            else if (hit_index != -1)
            {
                *item_hit = panel->items.data[hit_index].path;
            }
            panel->menu_item.show = !ui_window_end();
        }
        if (!panel->menu_item.show)
        {
            panel->menu_item.switch_on = panel->menu_item.show;
            return false;
        }
        return panel->list.right_click;
    }
    return false;
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
    u32 font_texture = texture_create(texture_properties, GL_RGBA8, GL_RGBA, GL_NEAREST);
    free(texture_properties->bytes);

    VertexBufferLayout vertex_buffer_layout = default_vertex_buffer_layout();

    u32 default_texture = create_default_texture();
    u32 copy_texture = load_icon("res/icons/copy.png");
    u32 paste_texture = load_icon("res/icons/paste.png");
    u32 delete_texture = load_icon("res/icons/delete.png");

    u32 shader = shader_create("res/shaders/vertex.glsl", "res/shaders/fragment.glsl");

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
    vertex_buffer_orphan(vertex_buffer_id, main_render->vertices.capacity * sizeof(Vertex),
                         GL_STREAM_DRAW, NULL);
    main_render->vertex_buffer_capacity = main_render->vertices.capacity;

    array_create(&main_render->indices, 100 * 6);
    generate_indicies(&main_render->indices, 0, 100);
    u32 index_buffer_id = index_buffer_create();
    index_buffer_orphan(index_buffer_id, main_render->indices.size * sizeof(u32), GL_STATIC_DRAW,
                        main_render->indices.data);
    free(main_render->indices.data);

    main_render->render =
        render_create(shader, textures, &vertex_buffer_layout, vertex_buffer_id, index_buffer_id);

    array_free(&vertex_buffer_layout.items);
}

internal void application_set_colors(ApplicationContext* app)
{
    app->secondary_color = global_get_secondary_color();
    app->clear_color = global_get_clear_color();
    app->text_color = global_get_text_color();
    app->tab_color = global_get_tab_color();
    app->bar_top_color = global_get_bar_top_color();
    app->bar_bottom_color = global_get_bar_bottom_color();
    app->border_color = global_get_border_color();
    app->scroll_bar_color = global_get_scroll_bar_color();
}

void application_initialize(ApplicationContext* app)
{
    app->window = window_create("FileTic", 1250, 800);
    event_initialize(app->window);
    platform_init_drag_drop();
    thread_initialize(100000, platform_get_core_count() - 1, &app->thread_queue);
    platform_set_executable_directory();
    platform_initialize_filter();

    app->font = (FontTTF){ 0 };
    const i32 width_atlas = 512;
    const i32 height_atlas = 512;
    const f32 pixel_height = 16;
    const u32 bitmap_size = width_atlas * height_atlas;
    u8* font_bitmap_temp = (u8*)calloc(bitmap_size, sizeof(u8));
    init_ttf_atlas(width_atlas, height_atlas, pixel_height, 96, 32, "C:/Windows/Fonts/arial.ttf",
                   font_bitmap_temp, &app->font);

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

    u32 shader = shader_create("res/shaders/vertex3d.glsl", "res/shaders/fragment.glsl");
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
        directory_tab_add("C:\\*", &app->thread_queue.task_queue, array_back(&app->tabs));
        array_back(&app->tabs)->window_id = app->tab_windows.data[app->current_tab_window_index++];
        saved = false;
    }

    if (saved)
    {
        u32 count = 0;
        for (u32 i = 0; i < app->tab_windows.size && count < app->tabs.size; ++i)
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
    array_create(&app->context_menu_options, 10);

    access_panel_initialize(&app->quick_access, app->windows.data[window_index++],
                            "saved/quick_access.txt");
    access_panel_initialize(&app->recent.panel, app->windows.data[window_index++],
                            "saved/recent.txt");
    app->recent.total = 10;

    app->filter.buffer = ui_input_buffer_create();
    array_create(&app->filter.options, 20);
    const char* filter_texts[7] = { "png", "jpg", "pdf", "cpp", "c", "java", "obj" };
    for (u32 i = 0; i < static_array_size(filter_texts); ++i)
    {
        FilterOption option = {
            .value = string_copy_d(filter_texts[i]),
            .selected = true,
        };
        array_push(&app->filter.options, option);
        platform_insert_filter_value(option.value, option.selected);
    }

    app->context_menu_window = app->windows.data[window_index++];
    app->style_menu_window = app->windows.data[window_index++];
    app->color_picker_window = app->windows.data[window_index++];
    app->filter_menu_window = app->windows.data[window_index++];
    app->menu_bar_window = app->windows.data[window_index++];

    theme_set_dark(&app->picker);
    application_set_colors(app);

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

    app->preview_index = -1;
    array_create(&app->preview_image.textures, 10);
    array_create(&app->preview_text.file_colored, 1000);

    char* menu_options[] = { "Menu", "Windows", "Style", "Filter" };
    array_create(&app->menu_values, 10);
    array_push(&app->menu_values, menu_options[0]);
    array_push(&app->menu_values, menu_options[1]);
    array_push(&app->menu_values, menu_options[2]);
    array_push(&app->menu_values, menu_options[3]);

    enable_gldebugging();
    glEnable(GL_BLEND);
    glEnable(GL_MULTISAMPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void application_uninitialize(ApplicationContext* app)
{
    memset(app->search_page.running_callbacks, 0, sizeof(app->search_page.running_callbacks));

    if (app->preview_image.current_viewed_path)
    {
        free(app->preview_image.current_viewed_path);
    }

    for (u32 i = 0; i < app->filter.options.size; ++i)
    {
        free(app->filter.options.data[i].value);
    }
    array_free(&app->filter.options);

    window_destroy(app->window);

    save_application_state(app);
    for (u32 i = 0; i < app->tabs.size; ++i)
    {
        directory_tab_clear(app->tabs.data + i);
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
    glClearColor(global_get_clear_color().r, global_get_clear_color().g, global_get_clear_color().b,
                 global_get_clear_color().a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    app->mvp.projection =
        ortho(0.0f, app->dimensions.width, app->dimensions.height, 0.0f, -1.0f, 1.0f);

    app->main_index_count = 0;
    rendering_properties_clear(&app->main_render);

    if (event_is_ctrl_and_key_pressed(FTIC_KEY_T))
    {
        DirectoryTab tab = { 0 };
        array_push(&app->tabs, tab);
        directory_tab_add("C:\\*", &app->thread_queue.task_queue, array_back(&app->tabs));
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
    event_poll(app->mouse_position);

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
    memset(files->array.data, 0, files->array.size * sizeof(files->array.data[0]));
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

internal void safe_add_directory_item(const DirectoryItem* item,
                                      FindingCallbackAttribute* arguments,
                                      SafeFileArray* safe_array)
{
    const char* name = item_namec(item);
    char* path = item->path;
    if (string_contains_case_insensitive(name, arguments->string_to_match))
    {
        const u32 name_length = (u32)strlen(name);
        const u32 path_length = (u32)strlen(path);

        DirectoryItem copy = *item;
        copy.path = string_copy(path, path_length, 2);
        copy.name_offset = (u16)(path_length - name_length);
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
                                           arguments->start_directory_length, true);
        should_free_directory = true;
    }

    for (u32 i = 0; i < directory.items.size && running; ++i)
    {
        DirectoryItem* item = directory.items.data + i;
        if (item->type == FOLDER_DEFAULT)
        {
            safe_add_directory_item(item, arguments, arguments->folder_array);

            FindingCallbackAttribute* next_arguments =
                (FindingCallbackAttribute*)calloc(1, sizeof(FindingCallbackAttribute));

            char* path = item->path;
            size_t directory_name_length = strlen(path);

            next_arguments->start_directory = string_copy(path, (u32)directory_name_length, 2);
            next_arguments->start_directory[directory_name_length++] = '\\';
            next_arguments->start_directory[directory_name_length++] = '*';
            next_arguments->file_array = arguments->file_array;
            next_arguments->folder_array = arguments->folder_array;
            next_arguments->thread_queue = arguments->thread_queue;
            next_arguments->start_directory_length = (u32)directory_name_length;
            next_arguments->string_to_match = arguments->string_to_match;
            next_arguments->string_to_match_length = arguments->string_to_match_length;
            next_arguments->running_id = arguments->running_id;
            next_arguments->running_callbacks = arguments->running_callbacks;

            ThreadTask task = thread_task(finding_callback, next_arguments);
            thread_tasks_push(next_arguments->thread_queue, &task, 1, NULL);
        }
        else
        {
            safe_add_directory_item(item, arguments, arguments->file_array);
        }
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

        const char* parent = directory_current(directory_history)->directory.parent;
        size_t parent_length = strlen(parent);
        char* dir2 = (char*)calloc(parent_length + 3, sizeof(char));
        memcpy(dir2, parent, parent_length);
        dir2[parent_length++] = '\\';
        dir2[parent_length++] = '*';

        const char* string_to_match = page->input.buffer.data;

        FindingCallbackAttribute* arguments =
            (FindingCallbackAttribute*)calloc(1, sizeof(FindingCallbackAttribute));
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

internal void application_open_menu_window(ApplicationContext* app, DropDownLayout layout,
                                           const V4 button_color)
{
    if (!ui_window_begin(app->menu_window, NULL, UI_WINDOW_OVERLAY | UI_WINDOW_FROSTED_GLASS))
    {
        return;
    }

    presist b8 animation_on_selected = true;
    presist b8 dark_mode_selected = true;
    presist b8 highlight_fucused_window_selected = true;
    presist b8 ui_frosted_glass_selected = true;

    presist f32 dark_mode_x = 0.0f;
    presist f32 focused_window_x = 0.0f;
    presist f32 animation_x = 0.0f;
    presist f32 ui_frosted_x = 0.0f;

    {
        V2 text_position = drop_down_layout_text_position(&layout);
        ui_window_add_text(text_position, "Font:", false, &layout.ui_layout);

        const char* font_path = ui_context_get_font_path();
        const u32 path_length = get_path_length(font_path, (u32)strlen(font_path));

        const V2 button_dim = ui_window_get_button_dimensions(v2d(), font_path + path_length, NULL);

        const f32 x_position = layout.width - button_dim.width - layout.ui_layout.at.x;
        if (ui_window_add_button(v2f(x_position, layout.ui_layout.at.y), NULL, &button_color,
                                 font_path + path_length, &layout.ui_layout))
        {
            if (app->font_change_directory.parent)
            {
                platform_reset_directory(&app->font_change_directory, true);
            }
            const char* font_directory_path = "C:\\Windows\\Fonts\\*";
            app->font_change_directory =
                platform_get_directory(font_directory_path, (u32)strlen(font_directory_path), true);
            DirectoryItemArray* items = &app->font_change_directory.items;
            for (i32 i = 0; i < (i32)items->size; ++i)
            {
                const char* extension =
                    items->data[i].type == FOLDER_DEFAULT
                        ? NULL
                        : file_get_extension(item_name(items->data + i),
                                             (u32)strlen(item_name(items->data + i)));
                if (!extension || strcmp(extension, "ttf"))
                {
                    char* path_to_remove = items->data[i].path;
                    for (u32 j = i; j < items->size - 1; ++j)
                    {
                        items->data[j] = items->data[j + 1];
                    }
                    free(path_to_remove);
                    items->size--;
                    --i;
                }
            }
            app->open_font_change_window = true;
        }
        ui_layout_row(&layout.ui_layout);
    }

    const f32 ui_font_pixel_height = ui_context_get_font_pixel_height();
    const i32 add_table[] = { 0, -1, 1 };
    {
        u32 result = drop_down_layout_add_increment_width_text(&layout, "Font size:", button_color,
                                                               (u32)ui_font_pixel_height);
        ui_context_change_font_pixel_height(ui_font_pixel_height + add_table[result]);
    }
    {
        u32 result = drop_down_layout_add_increment_width_text(
            &layout, "Recent total:", button_color, app->recent.total);
        app->recent.total +=
            (add_table[result] * closed_interval(2, app->recent.total + add_table[result], 40));
    }
    const V2 ui_icon_min_max = ui_get_big_icon_min_max();
    {
        u32 result = drop_down_layout_add_increment_width_text(
            &layout, "Ui icon min:", button_color, (u32)ui_icon_min_max.min);
        ui_set_big_icon_min_size(
            ui_icon_min_max.min +
            ((f32)add_table[result] *
             closed_interval(32.0f, ui_icon_min_max.min + (f32)add_table[result], 64.0f)));
    }
    {
        u32 result = drop_down_layout_add_increment_width_text(
            &layout, "Ui icon max:", button_color, (u32)ui_icon_min_max.max);
        ui_set_big_icon_max_size(
            ui_icon_min_max.max +
            ((f32)add_table[result] *
             closed_interval(128.0f, ui_icon_min_max.max + (f32)add_table[result], 256.0f)));
    }
    {
        f32 ui_list_padding = ui_get_list_padding();
        u32 result = drop_down_layout_add_increment_width_text(
            &layout, "List padding:", button_color, (u32)ui_list_padding);
        ui_set_list_padding(
            ui_list_padding +
            ((f32)add_table[result] *
             closed_interval(0.0f, ui_list_padding + (f32)add_table[result], 15.0f)));
    }

    drop_down_layout_add_line(&layout);

    {
        drop_down_layout_add_switch_with_text(&layout, "Animation on:", &animation_on_selected,
                                              &animation_x);
        ui_context_set_animation(animation_on_selected);
    }
    {
        drop_down_layout_add_switch_with_text(
            &layout, "Highlight focused window:", &highlight_fucused_window_selected,
            &focused_window_x);
        ui_context_set_highlight_focused_window(highlight_fucused_window_selected);
    }
    {
        drop_down_layout_add_switch_with_text(&layout, "Frosted glass:", &ui_frosted_glass_selected,
                                              &ui_frosted_x);
        ui_set_frosted_glass(ui_frosted_glass_selected);
    }
    if (ui_frosted_glass_selected)
    {
        {
            ui_window_add_text(layout.ui_layout.at, "Blur amount:", false, &layout.ui_layout);
            const V2 slider_size =
                v2f(78.0f + (ui_font_pixel_height * 2), -3.0f + (ui_font_pixel_height * 0.5f));
            const f32 slider_x = layout.width - slider_size.width - layout.ui_layout.at.x;
            presist b8 pressed = false;
            ui_set_frosted_blur_amount(ui_window_add_slider(
                v2f(slider_x, layout.ui_layout.at.y + 8.0f), slider_size, 0.0002f, 0.003f,
                ui_get_frosted_blur_amount(), &pressed, &layout.ui_layout));
        }
        ui_layout_row(&layout.ui_layout);
    }

    if (drop_down_layout_add_reset_button(&layout, button_color))
    {
        ui_context_set_font_path("C:/Windows/Fonts/arial.ttf");
        ui_context_change_font_pixel_height(16);

        animation_x = 1.0f * animation_on_selected;
        dark_mode_x = 1.0f * dark_mode_selected;
        focused_window_x = 1.0f * highlight_fucused_window_selected;

        animation_on_selected = true;
        dark_mode_selected = true;
        highlight_fucused_window_selected = true;

        app->recent.total = 10;

        ui_set_big_icon_min_size(64.0f);
        ui_set_big_icon_max_size(256.0f);
        ui_set_frosted_blur_amount(0.00132f);
        ui_set_list_padding(0.0f);
    }

    ui_window_dock_space_size(app->menu_window, v2f(layout.width, layout.ui_layout.at.y));
    if (ui_window_end() && !app->open_font_change_window && app->open_menu_window)
    {
        dark_mode_x = 0.0f;
        focused_window_x = 0.0f;
        animation_x = 0.0f;
        ui_frosted_x = 0.0f;
        app->open_menu_window = false;
    }
}

internal void window_open_menu_item_add(WindowOpenMenuItem* item, DropDownLayout* layout,
                                        const char* text, const V2 dimensions)
{
    const b8 selected_before = item->switch_on;
    drop_down_layout_add_switch_with_text(layout, text, &item->switch_on, &item->switch_x);
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

internal void application_open_windows_window(ApplicationContext* app, DropDownLayout layout,
                                              const V4 button_color)
{
    if (ui_window_begin(app->windows_window, NULL, UI_WINDOW_OVERLAY | UI_WINDOW_FROSTED_GLASS))
    {
        const f32 ui_font_pixel_height = ui_context_get_font_pixel_height();

        window_open_menu_item_add(&app->quick_access.menu_item, &layout,
                                  "Quick access:", app->dimensions);
        window_open_menu_item_add(&app->recent.panel.menu_item, &layout,
                                  "Recent:", app->dimensions);
        window_open_menu_item_add(&app->search_result_window_item, &layout,
                                  "Search result:", app->dimensions);

        ui_window_dock_space_size(app->windows_window, v2f(layout.width, layout.ui_layout.at.y));
        if (ui_window_end() && app->open_windows_window)
        {
            app->quick_access.menu_item.switch_x = 0.0f;
            app->recent.panel.menu_item.switch_x = 0.0f;
            app->search_result_window_item.switch_x = 0.0f;
            app->open_windows_window = false;
        }
    }
}

internal void drop_down_layout_add_color_picker_button(DropDownLayout* layout, const char* text,
                                                       V2 button_size, const V4 button_color,
                                                       const V2 window_position,
                                                       ApplicationContext* app,
                                                       ColorPicker* color_picker_to_use,
                                                       V4* color_to_change)
{
    ui_window_add_text(drop_down_layout_text_position(layout), text, false, &layout->ui_layout);
    const V2 position = v2f(layout->switch_x, layout->ui_layout.at.y);
    if (ui_window_add_button(position, &button_size, &button_color, NULL, &layout->ui_layout))
    {

        app->color_picker_to_use = color_picker_to_use;
        app->color_to_change = color_to_change;
        app->open_color_picker_window = true;
        app->color_picker_position = v2f(window_position.x + layout->ui_layout.at.x + layout->width,
                                         window_position.y + layout->ui_layout.at.y);
    }
    ui_window_add_border(position, button_size, v4ic(1.0f), 1.0f);
    ui_layout_row(&layout->ui_layout);
}

internal void application_open_style_menu_window(ApplicationContext* app, DropDownLayout layout,
                                                 V4 button_color)
{
    if (ui_window_begin(app->style_menu_window, NULL, UI_WINDOW_OVERLAY | UI_WINDOW_FROSTED_GLASS))
    {
        const UiWindow* top_bar_style = ui_window_get(app->style_menu_window);

        const f32 ui_font_pixel_height = ui_context_get_font_pixel_height();
        V2 button_size = v2i(8.0f + ui_font_pixel_height);
        layout.switch_x = layout.width - button_size.width - 10.0f;

        {
            drop_down_layout_add_color_picker_button(
                &layout, "Secondary color:", button_size, app->secondary_color,
                top_bar_style->position, app, &app->picker.secondary_color, &app->secondary_color);
            global_set_secondary_color(app->secondary_color);
        }
        {
            drop_down_layout_add_color_picker_button(&layout, "Clear color:", button_size,
                                                     app->clear_color, top_bar_style->position, app,
                                                     &app->picker.clear_color, &app->clear_color);
            global_set_clear_color(app->clear_color);
        }
        {
            drop_down_layout_add_color_picker_button(&layout, "Text color:", button_size,
                                                     app->text_color, top_bar_style->position, app,
                                                     &app->picker.text_color, &app->text_color);
            global_set_text_color(app->text_color);
        }
        {
            drop_down_layout_add_color_picker_button(&layout, "Tab color:", button_size,
                                                     app->tab_color, top_bar_style->position, app,
                                                     &app->picker.tab_color, &app->tab_color);
            global_set_tab_color(app->tab_color);
        }
        {
            drop_down_layout_add_color_picker_button(
                &layout, "Bar top color:", button_size, app->bar_top_color, top_bar_style->position,
                app, &app->picker.bar_top_color, &app->bar_top_color);
            global_set_bar_top_color(app->bar_top_color);
        }
        {
            drop_down_layout_add_color_picker_button(&layout, "Bar bottom color:", button_size,
                                                     app->bar_bottom_color, top_bar_style->position,
                                                     app, &app->picker.bar_bottom_color,
                                                     &app->bar_bottom_color);
            global_set_bar_bottom_color(app->bar_bottom_color);
        }
        {
            drop_down_layout_add_color_picker_button(
                &layout, "Border color:", button_size, app->border_color, top_bar_style->position,
                app, &app->picker.border_color, &app->border_color);
            global_set_border_color(app->border_color);
        }
        {
            drop_down_layout_add_color_picker_button(&layout, "Scroll bar color:", button_size,
                                                     app->scroll_bar_color, top_bar_style->position,
                                                     app, &app->picker.scroll_bar_color,
                                                     &app->scroll_bar_color);
            global_set_scroll_bar_color(app->scroll_bar_color);
        }

        drop_down_layout_add_line(&layout);

        {
            if (ui_window_add_button(layout.ui_layout.at, NULL, &button_color, "Dark Theme",
                                     &layout.ui_layout))
            {
                theme_set_dark(&app->picker);
                application_set_colors(app);
            }
            ui_layout_column(&layout.ui_layout);
            if (ui_window_add_button(layout.ui_layout.at, NULL, &button_color, "Light Theme",
                                     &layout.ui_layout))
            {
                theme_set_light(&app->picker);
                application_set_colors(app);
            }
            ui_layout_reset_column(&layout.ui_layout);
            ui_layout_row(&layout.ui_layout);
            if (ui_window_add_button(layout.ui_layout.at, NULL, &button_color, "Tron Theme",
                                     &layout.ui_layout))
            {
                theme_set_tron(&app->picker);
                application_set_colors(app);
            }
            ui_layout_column(&layout.ui_layout);
            if (ui_window_add_button(layout.ui_layout.at, NULL, &button_color, "Slime Theme",
                                     &layout.ui_layout))
            {
                theme_set_slime(&app->picker);
                application_set_colors(app);
            }
            ui_layout_reset_column(&layout.ui_layout);
            ui_layout_row(&layout.ui_layout);
        }

        if (drop_down_layout_add_reset_button(&layout, button_color))
        {
            theme_set_dark(&app->picker);
            application_set_colors(app);
        }

        ui_window_dock_space_size(app->style_menu_window, v2f(layout.width, layout.ui_layout.at.y));
        if (ui_window_end() && !app->open_color_picker_window && app->open_style_menu_window)
        {
            app->open_style_menu_window = false;
        }
    }
}

internal void application_open_filter_menu_window(ApplicationContext* app, DropDownLayout layout,
                                                  V4 button_color)
{
    if (ui_window_begin(app->filter_menu_window, NULL, UI_WINDOW_OVERLAY | UI_WINDOW_FROSTED_GLASS))
    {
        {
            V2 button_dim = ui_window_get_button_dimensions(v2d(), "Add", NULL);
            V2 input_dim = v2f(layout.width - (layout.ui_layout.padding * 3.0f) - button_dim.width,
                               button_dim.height);

            ui_window_add_input_field(layout.ui_layout.at, input_dim, &app->filter.buffer,
                                      &layout.ui_layout);
            ui_layout_column(&layout.ui_layout);
            if (ui_window_add_button(layout.ui_layout.at, &button_dim, &button_color, "Add",
                                     &layout.ui_layout))
            {
                for (u32 i = 0; i < app->filter.buffer.buffer.size; ++i)
                {
                    app->filter.buffer.buffer.data[i] =
                        (char)tolower(app->filter.buffer.buffer.data[i]);
                }
                b8 exist = false;
                for (u32 i = 0; i < app->filter.options.size; ++i)
                {
                    if (strcmp(app->filter.buffer.buffer.data, app->filter.options.data[i].value) ==
                        0)
                    {
                        exist = true;
                        break;
                    }
                }
                if (!exist)
                {
                    FilterOption option = {
                        .value = string_copy(app->filter.buffer.buffer.data,
                                             app->filter.buffer.buffer.size, 2),
                        .selected = true,
                    };
                    array_push(&app->filter.options, option);
                    platform_insert_filter_value(option.value, option.selected);
                }
            }
            ui_layout_reset_column(&layout.ui_layout);
            ui_layout_row(&layout.ui_layout);
        }
        b8 changed = false;

        {
            presist b8 selected = true;
            b8 before = selected;
            drop_down_layout_add_radio_button_with_text(&layout, "Folders", &selected);
            platform_set_folder_filter(selected);
            changed |= before != selected;
        }

        for (u32 i = 0; i < app->filter.options.size; ++i)
        {
            FilterOption* option = app->filter.options.data + i;
            b8 before = option->selected;
            drop_down_layout_add_radio_button_with_text(&layout, option->value, &option->selected);
            if (before != option->selected)
            {
                platform_set_filter_on(option->value, option->selected);
                changed = true;
            }
        }

        drop_down_layout_add_line(&layout);

        presist f32 hidden_files_x = 0.0f;
        presist f32 filter_on_x = 0.0f;
        {
            b8 before = app->filter.on;
            drop_down_layout_add_switch_with_text(&layout, "Filter on:", &app->filter.on,
                                                  &filter_on_x);
            platform_set_filter(app->filter.on);
            changed |= before != app->filter.on;
        }
        {
            b8 before = app->show_hidden_files;
            drop_down_layout_add_switch_with_text(
                &layout, "Show hidden files:", &app->show_hidden_files, &hidden_files_x);
            platform_show_hidden_files(app->show_hidden_files);
            changed |= before != app->show_hidden_files;
        }

        if (drop_down_layout_add_reset_button(&layout, button_color))
        {
            for (u32 i = 0; i < app->filter.options.size; ++i)
            {
                FilterOption* option = app->filter.options.data + i;
                option->selected = true;
                platform_set_filter_on(option->value, option->selected);
            }
            changed = true;

            app->filter.on = false;
            platform_set_filter(app->filter.on);

            app->show_hidden_files = true;
        }
        if (changed)
        {
            for (u32 i = 0; i < app->tabs.size; ++i)
            {
                directory_reload(directory_current(&app->tabs.data[i].directory_history));
            }
        }

        ui_window_dock_space_size(app->filter_menu_window,
                                  v2f(layout.width, layout.ui_layout.at.y));
        if (ui_window_end() && app->open_filter_menu_window)
        {
            app->open_filter_menu_window = false;
            hidden_files_x = 0.0f;
            filter_on_x = 0.0f;
        }
    }
}

internal void display_context_menu_items(ContextMenu* context_menu, MenuItemArray* menu_items,
                                         void* window, V2 item_size, const f32 item_text_padding,
                                         const CharPtrArray* selected_paths,
                                         DirectoryItemArray* quick_access, UiLayout* layout)
{

    const f32 ui_font_pixel_height = ui_context_get_font_pixel_height();
    for (u32 i = 0; i < menu_items->size; ++i)
    {
        b8 clicked = ui_window_add_button(layout->at, &item_size, NULL, NULL, layout);
        V4 text_color = global_get_text_color();

        MenuItem* item = menu_items->data + i;
        if (item->texture_id)
        {
            V2 icon_position = layout->at;
            icon_position.x += item_text_padding;
            icon_position.y += middle(item_size.height, 16.0f);
            ui_window_add_image(icon_position, v2i(16.0f), item->texture_id, layout);
        }
        V2 item_text_position = layout->at;
        item_text_position.x += item_text_padding + 24.0f;
        item_text_position.y += middle(item_size.height, ui_font_pixel_height) - 2.0f;
        ui_window_add_text_c(item_text_position, text_color, item->text, false, layout);

        if (item->submenu_items.size)
        {
            V2 symbol_size;
            V2 drop_down_symbol_position;
            if (item->submenu_open)
            {
                symbol_size = v2f(2.0f, item_size.height * 0.6f);
                drop_down_symbol_position.y =
                    layout->at.y + middle(item_size.height, symbol_size.height);
            }
            else
            {
                symbol_size = v2f(item_size.height * 0.6f, 2.0f);
                drop_down_symbol_position.y =
                    layout->at.y + middle(item_size.height, symbol_size.height);
            }
            drop_down_symbol_position.x = layout->at.x + item_size.width;
            drop_down_symbol_position.x -= 20.0f + (symbol_size.width * 0.5f);
            ui_window_add_rectangle(drop_down_symbol_position, symbol_size, v4ic(1.0f), NULL);
        }
        else if (clicked)
        {
            if (item->id == 160 || item->id == 162)
            {
                for (u32 j = 0; j < selected_paths->size; ++j)
                {
                    char* path_temp = selected_paths->data[j];
                    if (!platform_directory_exists(path_temp))
                    {
                        continue;
                    }
                    b8 exist = false;
                    for (u32 k = 0; k < quick_access->size; ++k)
                    {
                        if (!strcmp(path_temp, quick_access->data[k].path))
                        {
                            exist = true;
                            break;
                        }
                    }
                    if (!exist)
                    {
                        DirectoryItem directory_item = { 0 };
                        if (platform_get_id_from_path(path_temp, &directory_item.id))
                        {
                            const u32 length = (u32)strlen(path_temp);
                            char* path = (char*)calloc(length + 1, sizeof(char));
                            memcpy(path, path_temp, length);
                            directory_item.path = path;
                            directory_item.name_offset = (u16)get_path_length(path, length);
                            array_push(quick_access, directory_item);
                        }
                    }
                }
            }
            else
            {
                platform_context_menu_invoke_command(context_menu, window, item->id);
            }
            ui_window_close_current();
        }
        ui_layout_row(layout);
        if (clicked)
        {
            item->submenu_open ^= true;
        }
        if (item->submenu_open && item->submenu_items.size)
        {
            display_context_menu_items(context_menu, &item->submenu_items, window, item_size,
                                       item_text_padding + 24.0f, selected_paths, quick_access,
                                       layout);
        }
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
            [ADD_TO_QUICK_OPTION_INDEX] =
                app->panel_right_clicked ? "Remove from quick" : "Add to quick",
            [OPEN_IN_TERMINAL_OPTION_INDEX] = "Open in terminal",
            [PROPERTIES_OPTION_INDEX] = "Properties",
            [MORE_OPTION_INDEX] = "More",
        };
        if (event_is_key_pressed_repeat(FTIC_KEY_TAB))
        {
            if (event_get_key_event()->shift_pressed)
            {
                if (--app->drop_down_tab_index < 0)
                {
                    app->drop_down_tab_index = CONTEXT_ITEM_COUNT - 1;
                }
            }
            else
            {
                app->drop_down_tab_index++;
                app->drop_down_tab_index %= CONTEXT_ITEM_COUNT;
            }
        }

        V2 item_size = v2f(300.0f, 8.0f + ui_font_pixel_height);
        UiLayout layout = ui_layout_create(v2d());
        const f32 item_text_padding = 5.0f;
        if (app->context_menu.pcm == NULL)
        {
            for (u32 i = 0; i < CONTEXT_ITEM_COUNT - 1; ++i)
            {
                b8 clicked = ui_window_add_button(layout.at, &item_size, NULL, NULL, &layout);

                MainDropDownSelectionData data = {
                    .window = app->window,
                    .quick_access = &app->quick_access.items,
                    .directory = directory_current(&current_tab->directory_history),
                    .selected_paths = &current_tab->directory_list.selected_item_values.paths,
                    .panel = app->panel_right_clicked,
                    .task_queue = &app->thread_queue.task_queue,
                };
                V4 text_color = global_get_text_color();
                if (main_drop_down_selection(i, clicked, false, clicked, &text_color, &data))
                {
                    tab_clear_selected(current_tab);
                    app->open_context_menu_window = false;
                }

                if (i < 3)
                {
                    V2 icon_position = layout.at;
                    icon_position.x += item_text_padding;
                    icon_position.y += middle(item_size.height, 16.0f);
                    ui_window_add_image(icon_position, v2i(16.0f),
                                        app->main_render.render.textures.data[2 + i], &layout);
                }
                V2 item_text_position = layout.at;
                item_text_position.x += item_text_padding + 24.0f;
                item_text_position.y += middle(item_size.height, ui_font_pixel_height) - 2.0f;
                ui_window_add_text_c(item_text_position, text_color, options[i], false, &layout);
                ui_layout_row(&layout);
            }
        }
        display_context_menu_items(&app->context_menu, &app->context_menu.items, app->window,
                                   item_size, item_text_padding,
                                   &current_tab->directory_list.selected_item_values.paths,
                                   &app->quick_access.items, &layout);

        const UiWindow* window = ui_window_get(app->context_menu_window);
        const f32 height = ease_out_cubic(app->context_menu_x) * layout.at.y;
        ui_window_set_size(app->context_menu_window, v2f(item_size.width, height));

        V2 window_position = window->position;
        f32 diff = (window_position.y + window->size.height + 20.0f) - app->dimensions.height;
        window_position.y -= diff * (diff > 0.0f) * (f32)app->delta_time * 10.0f;
        if (window_position.y < app->context_menu_open_position_y)
        {
            const f32 height_form_position = (window->position.y + window->size.height + 20.0f);
            if (height_form_position < app->dimensions.height)
            {
                const f32 to_bottom = app->dimensions.height - height_form_position;
                const f32 offset = app->context_menu_open_position_y - window->position.y;
                diff = ftic_min(to_bottom, offset);
                window_position.y += diff * (f32)app->delta_time * 10.0f;
            }
        }
        ui_window_set_position(app->context_menu_window, window_position);

        app->context_menu_x += (f32)(app->delta_time * 8.0);
        app->context_menu_x = ftic_clamp_high(app->context_menu_x, 1.0f);

        if (ui_window_end())
        {
            app->context_menu_x = 0.0f;
            app->open_context_menu_window = false;
            if (app->context_menu.pcm != NULL)
            {
                platform_context_menu_destroy(&app->context_menu);
            }
        }
    }
}

internal b8 load_preview_image(const char* path, V2* image_dimensions, U32Array* textures)
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
        texture_load_full_path(path, &texture_properties);
        *image_dimensions = v2f((f32)texture_properties.width, (f32)texture_properties.height);

        ftic_assert(texture_properties.bytes);
        u32 texture = texture_create(&texture_properties, GL_RGBA8, GL_RGBA, GL_LINEAR);
        array_push(textures, texture);
        free(texture_properties.bytes);
        result = true;
    }
    return result;
}

internal b8 check_and_load_preview_image(const i32 index, V2* image_dimensions, char** current_path,
                                         const DirectoryItemArray* files, U32Array* textures)
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
                                         V2* image_dimensions, char** current_path,
                                         const DirectoryItemArray* items, U32Array* textures)
{
    b8 right_key = event_is_key_pressed_repeat(FTIC_KEY_RIGHT);
    b8 left_key = event_is_key_pressed_repeat(FTIC_KEY_LEFT);
    if (right_key || left_key)
    {
        for (i32 i = 0; i < (i32)items->size; ++i)
        {
            if (items->data[i].type == FOLDER_DEFAULT)
            {
                continue;
            }
            else if (!string_compare_case_insensitive(*current_path, items->data[i].path))
            {
                if (right_key)
                {
                    for (i32 count = 1; count <= (i32)items->size; ++count)
                    {
                        const i32 index = (i + count) % items->size;
                        if (check_and_load_preview_image(index, image_dimensions, current_path,
                                                         items, textures))
                        {
                            break;
                        }
                    }
                }
                else
                {
                    for (i32 count = 1, index = i - count; count <= (i32)items->size;
                         ++count, --index)
                    {
                        if (index < 0) index = items->size - 1;
                        if (check_and_load_preview_image(index, image_dimensions, current_path,
                                                         items, textures))
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

    const V2 total_preview_dimensions = v2_s_sub(application->dimensions, 40.0f);
    const V2 ratios = v2_div(total_preview_dimensions, image_dimensions_internal);
    f32 scale_factor = ratios.width < ratios.height ? ratios.width : ratios.height;
    if (scale_factor < 1.0f)
    {
        v2_s_multi_equal(&image_dimensions_internal, scale_factor);
    }
    return image_dimensions_internal;
}

void application_open_preview(ApplicationContext* app)
{
    UiLayout layout = { 0 };
    if (app->preview_index == 0)
    {
        DirectoryPage* current =
            directory_current(&app->tabs.data[app->tab_index].directory_history);

        V2 image_dimensions = load_and_scale_preview_image(
            app, &app->preview_image.dimensions, &app->preview_image.current_viewed_path,
            &current->directory.items, &app->preview_image.textures);

        ui_window_set_size(app->preview_window, image_dimensions);
        ui_window_set_position(app->preview_window,
                               v2f(middle(app->dimensions.width, image_dimensions.width),
                                   middle(app->dimensions.height, image_dimensions.height)));

        if (ui_window_begin(app->preview_window, NULL, UI_WINDOW_OVERLAY))
        {
            ui_window_add_image(v2d(), image_dimensions, app->preview_image.textures.data[0],
                                &layout);

            if (ui_window_end()) app->preview_index = -1;
        }
    }
    else if (app->preview_index == 1)
    {
        AABB view_port = {
            .min = v2i(0.0f),
            .size = v2_s_sub(app->dimensions, 0.0f),
        };
        app->preview_camera.view_port = view_port;
        app->preview_camera.view_projection.projection =
            perspective(PI * 0.5f, view_port.size.width / view_port.size.height, 0.01f, 100.0f);
        app->preview_camera.view_projection.view =
            view(app->preview_camera.position,
                 v3_add(app->preview_camera.position, app->preview_camera.orientation),
                 app->preview_camera.up);
        camera_update(&app->preview_camera, app->delta_time);
    }
    else
    {
        const V2 size = v2f(app->dimensions.width * 0.9f, app->dimensions.height * 0.9f);
        ui_window_set_size(app->preview_window, size);
        ui_window_set_position(app->preview_window,
                               v2f(middle(app->dimensions.width, size.width),
                                   middle(app->dimensions.height, size.height)));

        if (ui_window_begin(app->preview_window, NULL, UI_WINDOW_OVERLAY | UI_WINDOW_FROSTED_GLASS))
        {
            ui_window_add_text_colored(v2f(10.0f, 10.0f), &app->preview_text.file_colored, true,
                                       &layout);
            if (ui_window_end()) app->preview_index = -1;
        }
    }
}

void pre_open_context_menu(void* data)
{
    ContextMenu* menu = (ContextMenu*)data;
    platform_context_menu_create(menu, "C:");
    platform_context_menu_destroy(menu);
}

void open_menu_window(const u32 window, const V2 position, const f32 starting_width)
{
    ui_window_set_position(window, position);
    ui_window_set_size(window, v2f(starting_width * 0.8f, 0.0f));
    ui_window_start_size_animation(window, v2f(starting_width * 0.8f, 0.0f));
    ui_window_set_animation_x(window, 0.2f);
}

internal void application_handle_file_drag(ApplicationContext* app)
{
    const MouseMoveEvent* mouse_move_event = event_get_mouse_move_event();
    const MouseButtonEvent* mouse_button_event = event_get_mouse_button_event();
    presist b8 activated = false;
    if (mouse_button_event->action == FTIC_RELEASE)
    {
        activated = false;
    }
    if (app->current_tab->directory_list.inputs.data[0].active)
    {
        return;
    }

    if (event_is_mouse_button_pressed_once(FTIC_MOUSE_BUTTON_LEFT) &&
        !event_get_key_event()->alt_pressed)
    {
        app->last_mouse_position = app->mouse_position;
        app->mouse_drag_distance = 0.0f;
        activated = true;
    }

    if (activated && mouse_button_event->action == FTIC_PRESS &&
        app->current_tab->directory_list.selected_item_values.paths.size > 0)
    {
        app->mouse_drag_distance = v2_distance(app->last_mouse_position, app->mouse_position);
        if (app->mouse_drag_distance >= 10.0f)
        {
            platform_start_drag_drop(&app->current_tab->directory_list.selected_item_values.paths);
            activated = false;

            double x, y;
            window_get_mouse_position(app->window, &x, &y);
            app->mouse_position = v2f((f32)x, (f32)y);
            event_update_position(app->mouse_position);
            app->last_time = window_get_time();
        }
    }
}

internal void application_handle_keyboard_shortcuts(ApplicationContext* app)
{
    if (!app->use_shortcuts_for_tabs || app->preview_index == 1)
    {
        return;
    }
    if (app->current_tab->directory_list.selected_item_values.paths.size)
    {
        if (event_is_ctrl_and_key_pressed(FTIC_KEY_C))
        {
            platform_copy_to_clipboard(
                &app->current_tab->directory_list.selected_item_values.paths);
        }
        else if (app->preview_index == -1 &&
                 app->current_tab->directory_list.selected_item_values.paths.size == 1 &&
                 event_is_key_pressed_once(FTIC_KEY_E) && event_get_key_event()->shift_pressed)
        {
            const char* path = app->current_tab->directory_list.selected_item_values.paths.data[0];

            if (load_preview_image(path, &app->preview_image.dimensions,
                                   &app->preview_image.textures))
            {
                app->preview_image.current_viewed_path = string_copy(path, (u32)strlen(path), 0);
                directory_clear_selected_items(
                    &app->current_tab->directory_list.selected_item_values);
                app->preview_index = 0;
            }
            else
            {
                const char* extension = file_get_extension(path, (u32)strlen(path));
                if (strcmp(extension, "obj") == 0)
                {
                    app->preview_index = 1;
                    app->index_count_3d =
                        load_object(&app->render_3d, path, &app->preview_mesh_aabb);

                    app->preview_camera = camera_create_default();
                    camera_set_based_on_mesh_aabb(&app->preview_camera, &app->preview_mesh_aabb);
                }
                else
                {
                    app->preview_text.file = file_read_full_path(path);
                    parse_file(&app->preview_text.file, &app->preview_text.file_colored);
                    app->preview_index = 2;
                }
            }
        }
    }

    if (event_is_ctrl_and_key_pressed(FTIC_KEY_V))
    {
        directory_paste_in_directory(directory_current(&app->current_tab->directory_history));
    }
}

internal void application_check_if_any_tab_is_in_focus(ApplicationContext* app)
{
    app->use_shortcuts_for_tabs = false;
    app->tab_in_fucus = -1;
    for (u32 i = 0; i < app->tabs.size; ++i)
    {
        const u32 window_id = app->tabs.data[i].window_id;
        app->tab_in_fucus =
            app->tab_in_fucus + ((i - app->tab_in_fucus) * (ui_window_in_focus() == window_id));
    }

    if (app->tab_in_fucus != -1)
    {
        if (event_is_key_pressed_repeat(FTIC_KEY_TAB))
        {
            if (event_get_key_event()->shift_pressed)
            {
                --app->tab_in_fucus;
                app->tab_in_fucus = app->tab_in_fucus < 0 ? app->tabs.size - 1 : app->tab_in_fucus;
            }
            else
            {
                ++app->tab_in_fucus;
                app->tab_in_fucus %= app->tabs.size;
            }
            ui_context_set_window_in_focus(app->tabs.data[app->tab_in_fucus].window_id);
            app->tab_index = app->tab_in_fucus;
            app->current_tab = app->tabs.data + app->tab_index;
        }
        // Rename of files should stop ctrl-c, ctrl-v and so on of files.
        app->use_shortcuts_for_tabs =
            !app->tabs.data[app->tab_in_fucus].directory_list.inputs.data[0].active;
    }
}

internal b8 application_show_search_result_window(ApplicationContext* app,
                                                  const f32 list_item_height)
{
    if (ui_window_begin(app->search_result_window_item.window, "Search result",
                        UI_WINDOW_TOP_BAR | UI_WINDOW_RESIZEABLE))
    {
        SearchPage* page = &app->search_page;
        platform_mutex_lock(&page->search_result_file_array.mutex);
        platform_mutex_lock(&page->search_result_folder_array.mutex);

        i32 hit_index = -1;

        UiLayout layout = { .at = v2f(10.0f, 10.0f) };
        i32 selected_item = -1;
        selected_item = ui_window_add_directory_item_list(layout.at, list_item_height,
                                                          &page->search_result_folder_array.array,
                                                          NULL, &hit_index, &layout);
        if (selected_item != -1)
        {
            directory_open_folder(page->search_result_folder_array.array.data[selected_item].id,
                                  &app->current_tab->directory_history);
        }
        else if (hit_index != -1)
        {
            app->item_hit = page->search_result_folder_array.array.data[hit_index].path;
        }
        ui_layout_row(&layout);
        selected_item = ui_window_add_directory_item_list(layout.at, list_item_height,
                                                          &page->search_result_file_array.array,
                                                          NULL, &hit_index, &layout);
        if (selected_item != -1)
        {
            platform_open_file(page->search_result_file_array.array.data[selected_item].path);
        }

        platform_mutex_unlock(&page->search_result_file_array.mutex);
        platform_mutex_unlock(&page->search_result_folder_array.mutex);

        return ui_window_end();
    }
    return false;
}

void application_check_and_open_context_menu(ApplicationContext* app)
{
    if (app->preview_index != 1 && !event_get_key_event()->ctrl_pressed &&
        event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_RIGHT))
    {
        const f32 ui_font_pixel_height = ui_context_get_font_pixel_height();
        app->open_context_menu_window = true;
        V2 item_size = v2f(300.0f, 8.0f + ui_font_pixel_height);
        const f32 end_height = CONTEXT_ITEM_COUNT * item_size.height;

        V2 position = app->mouse_position;
        position.x = min(app->dimensions.width - item_size.width - 10.0f, position.x);
        ui_window_set_position(app->context_menu_window, position);
        app->context_menu_open_position_y = position.y;
        app->menu_open_this_frame = true;
    }
}

void application_update_ui(ApplicationContext* app)
{
    const f32 ui_font_pixel_height = ui_context_get_font_pixel_height();
    const f32 search_bar_width = 250.0f;
    const f32 top_bar_height = 20.0f + ui_font_pixel_height;
    const f32 top_bar_menu_height = 10.0f + ui_font_pixel_height;
    const f32 bottom_bar_height = 15.0f + ui_font_pixel_height;
    const f32 list_item_height = 16.0f + ui_font_pixel_height;

    AABB dock_space = { .min = v2f(0.0f, top_bar_height + top_bar_menu_height) };
    dock_space.size = v2_sub(app->dimensions, dock_space.min);
    dock_space.size.height -= bottom_bar_height;
    ui_context_begin(app->dimensions, &dock_space, app->delta_time, app->check_collision_in_ui);
    {
        const DirectoryTab* current_tab = app->current_tab;
        const DirectoryPage* current_directory =
            directory_current(&app->current_tab->directory_history);

        const V4 button_color = v4a(v4_s_multi(global_get_clear_color(), 3.0f), 1.0f);

        DropDownLayout layout = drop_down_layout_create(350.0f, v2i(10.0f));
        layout.ui_layout.padding = 10.0f;
        if (app->open_menu_window)
        {
            application_open_menu_window(app, layout, button_color);
        }
        if (app->open_windows_window)
        {
            application_open_windows_window(app, layout, button_color);
        }
        if (app->open_style_menu_window)
        {
            application_open_style_menu_window(app, layout, button_color);
        }
        if (app->open_filter_menu_window)
        {
            application_open_filter_menu_window(app, layout, button_color);
        }

        if (app->open_font_change_window)
        {
            const V2 size = v2f(app->dimensions.width * 0.5f, app->dimensions.height * 0.9f);
            ui_window_set_size(app->font_change_window, size);
            ui_window_set_position(app->font_change_window,
                                   v2f(middle(app->dimensions.width, size.width),
                                       middle(app->dimensions.height, size.height)));
            if (ui_window_begin(app->font_change_window, NULL,
                                UI_WINDOW_OVERLAY | UI_WINDOW_FROSTED_GLASS))
            {
                i32 hit_index = -1;
                V2 list_position = v2f(10.0f, 10.0f);
                i32 selected_item = ui_window_add_directory_item_list(
                    list_position, list_item_height, &app->font_change_directory.items, NULL,
                    &hit_index, NULL);
                if (selected_item != -1)
                {
                    const char* path = app->font_change_directory.items.data[selected_item].path;
                    ui_context_set_font_path(path);
                    ui_context_change_font_pixel_height(ui_font_pixel_height);
                    app->open_font_change_window = false;
                    app->open_menu_window = true;
                }
                app->open_font_change_window = !ui_window_end();
            }
        }

        if (app->open_color_picker_window)
        {
            if (ui_window_begin(app->color_picker_window, NULL,
                                UI_WINDOW_OVERLAY | UI_WINDOW_FROSTED_GLASS))
            {
                UiLayout ui_layout = ui_layout_create(v2i(10.0f));
                *app->color_to_change = ui_window_add_color_picker(
                    v2i(10.0f), v2f(200.0f, 200.0f), app->color_picker_to_use, &ui_layout);
#if 1
                char buffer[64] = { 0 };
                value_to_string(buffer, V2_FMT(app->color_picker_to_use->at));
                log_message(buffer, strlen(buffer));
                memset(buffer, 0, sizeof(buffer));
                value_to_string(buffer, "%f", app->color_picker_to_use->spectrum_at);
                log_message(buffer, strlen(buffer));
                memset(buffer, 0, sizeof(buffer));
                value_to_string(buffer, V3_FMT(*app->color_to_change));
                log_message(buffer, strlen(buffer));
#endif
                ui_window_set_position(app->color_picker_window, app->color_picker_position);
                ui_window_set_size(app->color_picker_window, v2f(ui_layout.column_width + 20.0f,
                                                                 ui_layout.row_height + 20.0f));

                app->open_color_picker_window = !ui_window_end();
            }
        }

        ui_window_set_position(app->menu_bar_window, v2d());
        ui_window_set_size(app->menu_bar_window, v2f(app->dimensions.width, top_bar_menu_height));
        if (ui_window_begin(app->menu_bar_window, NULL, UI_WINDOW_NONE))
        {
            V2 drop_down_position = v2d();
            i32 index_clicked = ui_window_add_menu_bar(&app->menu_values, &drop_down_position);
            if (index_clicked == 0)
            {
                open_menu_window(app->menu_window, drop_down_position, layout.width);
                app->open_windows_window = false;
                app->open_style_menu_window = false;
                app->open_filter_menu_window = false;
                app->open_menu_window = true;
            }
            else if (index_clicked == 1)
            {
                open_menu_window(app->windows_window, drop_down_position, layout.width);
                app->open_menu_window = false;
                app->open_style_menu_window = false;
                app->open_filter_menu_window = false;
                app->open_windows_window = true;
            }
            else if (index_clicked == 2)
            {
                open_menu_window(app->style_menu_window, drop_down_position, layout.width);
                app->open_menu_window = false;
                app->open_windows_window = false;
                app->open_filter_menu_window = false;
                app->open_style_menu_window = true;
            }
            else if (index_clicked == 3)
            {
                open_menu_window(app->filter_menu_window, drop_down_position, layout.width);
                app->open_menu_window = false;
                app->open_windows_window = false;
                app->open_style_menu_window = false;
                app->open_filter_menu_window = true;
            }
            ui_window_end();
        }

        ui_window_set_position(app->top_bar_window, v2f(0.0f, top_bar_menu_height));
        ui_window_set_size(app->top_bar_window, v2f(app->dimensions.width, top_bar_height));

        ui_context_set_window_top_color(global_get_bar_top_color());
        ui_context_set_window_bottom_color(global_get_bar_bottom_color());

        if (ui_window_begin(app->top_bar_window, NULL, UI_WINDOW_NONE))
        {
            const f32 button_size = top_bar_height - 10.0f;
            UiLayout ui_layout = ui_layout_create(v2f(10.0f, middle(top_bar_height, button_size)));
            ui_layout.column_width = button_size;
            b8 disable = app->current_tab->directory_history.current_index <= 0;
            add_move_in_history_button(&ui_layout, arrow_back_icon_co, disable, FTIC_MOUSE_BUTTON_4,
                                       -1, &app->current_tab->directory_list.selected_item_values,
                                       &app->current_tab->directory_history);

            ui_layout_column(&ui_layout);
            disable = app->current_tab->directory_history.history.size <=
                      app->current_tab->directory_history.current_index + 1;
            add_move_in_history_button(&ui_layout, arrow_right_icon_co, disable,
                                       FTIC_MOUSE_BUTTON_4, 1,
                                       &app->current_tab->directory_list.selected_item_values,
                                       &app->current_tab->directory_history);

            ui_layout_column(&ui_layout);
            disable = !directory_can_go_up(
                directory_current(&app->current_tab->directory_history)->directory.parent);
            if (ui_window_add_icon_button(ui_layout.at, v2i(ui_layout.column_width),
                                          global_get_highlight_color(), arrow_up_icon_co,
                                          UI_ARROW_ICON_TEXTURE, disable, &ui_layout))
            {
                directory_go_up(&app->current_tab->directory_history);
            }

            ui_layout.padding = 10.0f;
            ui_layout_column(&ui_layout);

            const V2 bar_size =
                v2f((app->dimensions.width - (search_bar_width + 20.0f)) - ui_layout.at.x,
                    top_bar_height - 10.0f);

            ui_layout.at.y = middle(top_bar_height, bar_size.height);

            b8 active_before = app->parent_directory_input.active;
            if (ui_window_add_input_field(ui_layout.at, bar_size, &app->parent_directory_input,
                                          &ui_layout))
            {
                DirectoryPage* current = directory_current(&app->current_tab->directory_history);
                get_suggestions(v2f(ui_layout.at.x, ui_layout.at.y + top_bar_menu_height), current,
                                &app->parent_directory_input, &app->suggestions,
                                &app->suggestion_data);
            }

            if (collision_point_in_aabb(app->mouse_position, &app->suggestions.aabb))
            {
                app->parent_directory_input.active = true;
            }
            if (app->parent_directory_input.active)
            {
                if (!active_before)
                {
                    const char* path =
                        directory_current(&app->current_tab->directory_history)->directory.parent;
                    set_input_buffer_to_current_directory(path, &app->parent_directory_input);
                }
                app->suggestion_data.change_directory = false;
                drop_down_menu_add(&app->suggestions, app, &app->suggestion_data);

                if (app->suggestion_data.change_directory ||
                    event_is_key_pressed_once(FTIC_KEY_ENTER))
                {
                    if (event_is_mouse_button_pressed_once(FTIC_MOUSE_BUTTON_LEFT))
                    {
                        array_push(app->suggestion_data.parent_directory, '\\');
                        suggestions_add_extra_space(&app->suggestion_data);
                    }
                    char* last_char = array_back(&app->parent_directory_input.buffer);
                    char saved = *last_char;
                    b8 should_restore = *last_char == '\\' || *last_char == '/';
                    if (should_restore)
                    {
                        *last_char = '\0';
                        app->parent_directory_input.buffer.size--;
                    }
                    if (!directory_go_to(app->parent_directory_input.buffer.data,
                                         app->parent_directory_input.buffer.size,
                                         &app->current_tab->directory_history))
                    {
                    }
                    if (should_restore)
                    {
                        *last_char = saved;
                        app->parent_directory_input.buffer.size++;
                    }

                    app->parent_directory_input.input_index = -1;
                    app->suggestions.tab_index = -1;
                    DirectoryPage* current =
                        directory_current(&app->current_tab->directory_history);
                    get_suggestions(v2f(ui_layout.at.x, ui_layout.at.y + top_bar_menu_height),
                                    current, &app->parent_directory_input, &app->suggestions,
                                    &app->suggestion_data);
                }
            }
            else
            {
                const char* path =
                    directory_current(&app->current_tab->directory_history)->directory.parent;
                set_input_buffer_to_current_directory(path, &app->parent_directory_input);
                app->suggestions.aabb = (AABB){ 0 };
                app->suggestions.x = 0;
            }

            ui_layout_column(&ui_layout);
            if (ui_window_add_input_field(ui_layout.at, v2f(search_bar_width, bar_size.height),
                                          &app->search_page.input, &ui_layout))
            {
                search_page_clear_result(&app->search_page);
                app->search_page.running_callbacks[app->search_page.last_running_id] = false;
                if (!app->search_result_window_item.show)
                {
                    open_window(app->dimensions, app->search_result_window_item.window);
                    app->search_result_window_item.show = true;
                    app->search_result_window_item.switch_on = true;
                }
            }
            if (app->search_page.running_callbacks[app->search_page.last_running_id])
            {
                ui_layout_column(&ui_layout);
                V2 dim = v2f(ui_font_pixel_height + 12.0f, ui_font_pixel_height + 10.0f);
                if (ui_window_add_button(v2f(ui_layout.at.x - dim.width - 5.0f,
                                             ui_layout.at.y + middle(button_size, dim.height)),
                                         &dim, &button_color, "X", &ui_layout))
                {
                    app->search_page.running_callbacks[app->search_page.last_running_id] = false;
                }
            }
            if (app->search_page.input.active)
            {
                if (event_is_key_pressed_once(FTIC_KEY_ENTER))
                {
                    search_page_search(&app->search_page, &app->current_tab->directory_history,
                                       &app->thread_queue.task_queue);
                }
            }
            else
            {
                app->search_page.running_callbacks[app->search_page.last_running_id] = false;
            }

            ui_window_end();
        }

        const V2 size = v2f(app->dimensions.width, bottom_bar_height);
        ui_window_set_position(app->bottom_bar_window,
                               v2f(0.0f, app->dimensions.height - bottom_bar_height));
        ui_window_set_size(app->bottom_bar_window, size);
        ui_context_set_window_top_color(global_get_bar_top_color());
        ui_context_set_window_bottom_color(global_get_bar_bottom_color());
        if (ui_window_begin(app->bottom_bar_window, NULL, UI_WINDOW_NONE))
        {
            UiLayout ui_layout = ui_layout_create(v2f(10.0f, 4.0f));
            DirectoryPage* current = directory_current(&app->current_tab->directory_history);
            char buffer[64] = { 0 };
            sprintf_s(buffer, sizeof(buffer), "Items: %u", current->directory.items.size);
            ui_window_add_text(ui_layout.at, buffer, false, &ui_layout);

            const f32 list_grid_icon_position = size.width - (2.0f * bottom_bar_height + 5.0f);
            if (current->grid_view)
            {
                ui_layout.at =
                    v2f(list_grid_icon_position - 120.0f, middle(bottom_bar_height, 5.0f));

                const V2 ui_icon_min_max = ui_get_big_icon_min_max();
                static b8 pressed = false;
                ui_set_big_icon_size(ui_window_add_slider(
                    ui_layout.at, v2f(110.0f, 5.0f), ui_icon_min_max.min, ui_icon_min_max.max,
                    ui_get_big_icon_size(), &pressed, &ui_layout));
            }
            ui_layout.at.x = list_grid_icon_position;
            ui_layout.at.y = 0.0f;

            if (ui_window_add_icon_button(ui_layout.at, v2i(bottom_bar_height),
                                          global_get_border_color(), full_icon_co,
                                          UI_LIST_ICON_TEXTURE, false, &ui_layout))
            {
                current->grid_view = false;
            }
            ui_layout_column(&ui_layout);
            if (ui_window_add_icon_button(ui_layout.at, v2i(bottom_bar_height),
                                          global_get_border_color(), full_icon_co,
                                          UI_GRID_ICON_TEXTURE, false, &ui_layout))
            {
                current->grid_view = true;
            }
            ui_window_end();
        }

        if (!app->open_context_menu_window)
        {
            app->panel_right_clicked = NULL;
        }

        if (access_panel_open(&app->quick_access, "Quick access", list_item_height, &app->item_hit,
                              &app->recent, &app->current_tab->directory_history))
        {
            app->panel_right_clicked = &app->quick_access;
        }

        access_panel_open(&app->recent.panel, "Recent", list_item_height, &app->item_hit,
                          &app->recent, &app->current_tab->directory_history);

        if (app->search_result_window_item.show)
        {
            app->search_result_window_item.show =
                !application_show_search_result_window(app, list_item_height);
            if (!app->search_result_window_item.show)
            {
                app->search_result_window_item.switch_on = false;
            }
        }

        if (app->preview_index != -1)
        {
            application_open_preview(app);
        }
        else
        {
            if (app->preview_image.textures.size > 0)
            {
                texture_delete(
                    app->preview_image.textures.data[--app->preview_image.textures.size]);
            }
            if (app->preview_text.file.buffer)
            {
                free(app->preview_text.file.buffer);
                app->preview_text.file = (FileAttrib){ 0 };
                app->preview_text.file_colored.size = 0;
            }
        }

        for (u32 i = 0; i < app->tabs.size; ++i)
        {
            const u32 window_id = app->tabs.data[i].window_id;
            if (ui_window_in_focus() == window_id)
            {
                app->tab_index = i;
                app->current_tab = app->tabs.data + app->tab_index;
            }

            DirectoryTab* tab_to_update = app->tabs.data + i;
            if (app->item_hit == NULL && ui_window_is_hit(window_id))
            {
                app->item_hit =
                    directory_current(&tab_to_update->directory_history)->directory.parent;
            }

            ui_window_set_alpha(window_id, i == app->tab_index ? 1.0f : 0.7f);

            if (application_show_directory_window(app, window_id, list_item_height,
                                                  app->check_collision_in_ui, &app->item_hit,
                                                  tab_to_update))
            {
                DirectoryTab tab_to_remove = app->tabs.data[i];
                for (u32 j = i; j < app->tabs.size - 1; ++j)
                {
                    app->tabs.data[j] = app->tabs.data[j + 1];
                }
                directory_tab_clear(&tab_to_remove);
                app->tabs.size--;
                array_push(&app->free_window_ids, window_id);

                // NOTE: this is probably always true
                if (i == app->tab_index)
                {
                    app->tab_index = (app->tab_index + 1) % app->tabs.size;
                }
                --i;
            }
        }
        if (current_tab == app->current_tab)
        {
            const DirectoryPage* next_directory =
                directory_current(&app->current_tab->directory_history);
            if (current_directory != next_directory)
            {
                ui_window_set_end_scroll_offset(app->current_tab->window_id,
                                                next_directory->offset);
                ui_window_set_current_scroll_offset(app->current_tab->window_id, 0.0f);
            }
        }
        for (u32 i = 0; i < app->tabs.size; ++i)
        {
            directory_current(&app->tabs.data[i].directory_history)->offset =
                ui_window_get(app->tabs.data[i].window_id)->end_scroll_offset;
        }

        if (app->open_context_menu_window)
        {
            if (app->menu_open_this_frame &&
                app->current_tab->directory_list.selected_item_values.paths.size)
            {
                if (app->context_menu.pcm != NULL)
                {
                    platform_context_menu_destroy(&app->context_menu);
                }
                platform_context_menu_create(
                    &app->context_menu,
                    app->current_tab->directory_list.selected_item_values.paths.data[0]);

                app->menu_open_this_frame = false;
            }
            application_open_context_menu_window(app, app->current_tab);
        }
    }
    ui_context_end();
}

internal void preview_render_3d(ApplicationContext* app)
{
    MVP mvp = {
        .model = m4i(1.0f),
        .view = app->preview_camera.view_projection.view,
        .projection = app->preview_camera.view_projection.projection,
    };

    glEnable(GL_DEPTH_TEST);
    render_begin_draw(&app->render_3d, app->render_3d.shader_properties.shader, &mvp);

    glUniform3f(app->light_dir_location, -app->preview_camera.orientation.x,
                -app->preview_camera.orientation.y, -app->preview_camera.orientation.z);

    render_draw(0, app->index_count_3d, &app->preview_camera.view_port);
    render_end_draw(&app->render_3d);
    glDisable(GL_DEPTH_TEST);
    if (event_is_mouse_button_clicked(FTIC_MOUSE_BUTTON_LEFT))
    {
        app->preview_index = -1;
    }

    if (event_is_key_pressed_once(FTIC_KEY_R))
    {
        camera_set_based_on_mesh_aabb(&app->preview_camera, &app->preview_mesh_aabb);
    }
}

void application_look_for_dropped_files(ApplicationContext* app)
{
    const CharPtrArray* dropped_paths = event_get_drop_buffer();
    if (dropped_paths->size)
    {
        Directory* current = &directory_current(&app->current_tab->directory_history)->directory;
        char* path_to_dropp_in = app->item_hit ? app->item_hit : current->parent;
        b8 same_directory = false;
        for (u32 i = 0; i < dropped_paths->size; ++i)
        {
            char* path = dropped_paths->data[i];
            const u32 path_length = get_path_length(path, (u32)strlen(path)) - 1;
            char saved = path[path_length];
            path[path_length] = '\0';
            if (string_compare_case_insensitive(path, path_to_dropp_in) == 0)
            {
                same_directory = true;
                break;
            }
            path[path_length] = saved;
        }
        if (!same_directory)
        {
            platform_move_to_directory(dropped_paths, path_to_dropp_in);
        }
    }
}

void application_run()
{
    ApplicationContext app = { 0 };
    application_initialize(&app);

    while (!window_should_close(app.window))
    {
        application_begin_frame(&app);
        app.current_tab = app.tabs.data + app.tab_index;
        application_handle_file_drag(&app);
        application_check_if_any_tab_is_in_focus(&app);
        application_handle_keyboard_shortcuts(&app);

        app.check_collision_in_ui = app.preview_index != 1;
        if (app.check_collision_in_ui &&
            collision_point_in_aabb(app.mouse_position, &app.suggestions.aabb))
        {
            app.check_collision_in_ui = false;
        }
        application_check_and_open_context_menu(&app);

        app.item_hit = NULL;
        application_update_ui(&app);

        rendering_properties_check_and_grow_vertex_buffer(&app.main_render);
        rendering_properties_check_and_grow_index_buffer(&app.main_render, app.main_index_count);

        buffer_set_sub_data(app.main_render.render.vertex_buffer_id, GL_ARRAY_BUFFER, 0,
                            sizeof(Vertex) * app.main_render.vertices.size,
                            app.main_render.vertices.data);

        AABB whole_screen_scissor = { .size = app.dimensions };

        render_begin_draw(&app.main_render.render, app.main_render.render.shader_properties.shader,
                          &app.mvp);
        render_draw(0, app.main_index_count, &whole_screen_scissor);
        render_end_draw(&app.main_render.render);

        if (app.preview_index == 1)
        {
            preview_render_3d(&app);
        }

        application_look_for_dropped_files(&app);

        for (u32 i = 0; i < app.tabs.size; ++i)
        {
            DirectoryTab* current = app.tabs.data + i;
            if (directory_look_for_directory_change(current->directory_history.change_handle))
            {
                log_message("reload", 6);
                directory_reload(directory_current(&current->directory_history));
                directory_history_update_directory_change_handle(&current->directory_history);
            }
        }

        application_end_frame(&app);
    }

    // TODO: Cleanup of all
    application_uninitialize(&app);
}
