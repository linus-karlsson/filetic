#include "ui.h"
#include "event.h"
#include "application.h"
#include "opengl_util.h"
#include "texture.h"
#include "logging.h"
#include <string.h>
#include <stdio.h>
#include <glad/glad.h>

typedef struct UiWindow
{
    u32 id;
    i32 docking_space;
    i32 docking_id;
    V2 dimensions;
    V2 position;
    V2 first_item_position;
    V2 top_bar_offset;
    u32 index;

    u32 rendering_index_offset;
    u32 rendering_index_count;

    f32 total_height;
    f32 end_scroll_offset;
    f32 current_scroll_offset;

    b8 area_hit;

    b8 top_bar_pressed;
    b8 top_bar_hold;

    ScrollBar scroll_bar;
} UiWindow;

typedef struct UiWindowArray
{
    u32 size;
    u32 capacity;
    UiWindow* data;
} UiWindowArray;

typedef struct HoverClickedIndexArray
{
    u32 size;
    u32 capacity;
    HoverClickedIndex* data;
} HoverClickedIndexArray;

typedef struct AABBArrayArray
{
    u32 size;
    u32 capacity;
    AABBArray* data;
} AABBArrayArray;

typedef struct Dock
{
    u32 id;
    AABB aabb;
} Dock;

typedef struct DockArray
{

    u32 size;
    u32 capacity;
    Dock* data;
} DockArray;

typedef struct V2Array
{
    u32 size;
    u32 capacity;
    V2* data;
} V2Array;

typedef struct UiContext
{
    MVP mvp;

    u32 current_window_id;
    u32 current_index_offset;
    U32Array id_to_index;
    U32Array free_indices;
    UiWindowArray windows;
    AABBArrayArray window_aabbs;
    HoverClickedIndexArray window_hover_clicked_indices;

    RenderingProperties render;

    U32Array dock_free_indices[4];
    U32Array dock_id_to_index[4];
    DockArray docks[4];
    AABB dock_expanded_spaces[4];
    b8 docking_hit[4];

    FontTTF font;

    V2 dimensions;
    f64 delta_time;

    // TODO: Better solution?
    b8 any_window_hold;

} UiContext;

global UiContext ui_context = { 0 };

internal f32 set_scroll_offset(const f32 total_height, const f32 area_y,
                               f32 offset)
{
    const f32 y_offset = event_get_mouse_wheel_event()->y_offset * 100.0f;
    offset += y_offset;
    offset = clampf32_high(offset, 0.0f);

    f32 low = area_y - total_height;
    low = clampf32_high(low, 0.0f);
    return clampf32_low(offset, low);
}

internal f32 smooth_scroll(const f64 delta_time, const f32 offset,
                           f32 scroll_offset)
{
    scroll_offset += (f32)((offset - scroll_offset) * (delta_time * 15.0f));
    return scroll_offset;
}

internal b8 is_mouse_button_clicked(i32 button)
{
    const MouseButtonEvent* event = event_get_mouse_button_event();
    return event->activated && event->action == FTIC_RELEASE &&
           event->button == button;
}

#define move_to_back(type, array, index)                                       \
    do                                                                         \
    {                                                                          \
        type temp = (array)->data[(index)];                                    \
        for (u32 i = (index); i >= 1; --i)                                     \
        {                                                                      \
            (array)->data[i] = (array)->data[i - 1];                           \
        }                                                                      \
        (array)->data[0] = temp;                                               \
    } while (0)

internal void change_window_order(UiContext* context,
                                  const u32 window_in_focus_index)
{
    if (window_in_focus_index == 0)
    {
        return;
    }
    move_to_back(UiWindow, &context->windows, window_in_focus_index);
    for (u32 i = 0; i < context->windows.size; ++i)
    {
        context->id_to_index.data[context->windows.data[i].id] = i;
        context->windows.data[i].index = i;
    }
    move_to_back(AABBArray, &context->window_aabbs, window_in_focus_index);
    move_to_back(HoverClickedIndex, &context->window_hover_clicked_indices,
                 window_in_focus_index);
}

internal u32 get_id(const u32 size, U32Array* free_indices,
                    U32Array* id_to_index)
{
    u32 id = 0;
    if (free_indices->size)
    {
        id = *array_back(free_indices);
        free_indices->size--;
        id_to_index->data[id] = size;
    }
    else
    {
        id = id_to_index->size;
        array_push(id_to_index, size);
    }
    return id;
}

void ui_context_create()
{
    array_create(&ui_context.id_to_index, 100);
    array_create(&ui_context.free_indices, 100);
    array_create(&ui_context.windows, 10);
    array_create(&ui_context.window_aabbs, 10);
    array_create(&ui_context.window_hover_clicked_indices, 10);

    for (u32 i = 0; i < 4; ++i)
    {
        array_create(ui_context.docks + i, 10);
        array_create(ui_context.dock_id_to_index + i, 10);
        array_create(ui_context.dock_free_indices + i, 10);
    }

    VertexBufferLayout vertex_buffer_layout = default_vertex_buffer_layout();

    u32 shader = shader_create("./res/shaders/vertex.glsl",
                               "./res/shaders/fragment.glsl");

    ui_context.font = (FontTTF){ 0 };
    const i32 width_atlas = 512;
    const i32 height_atlas = 512;
    const f32 pixel_height = 16;
    const u32 bitmap_size = width_atlas * height_atlas;
    u8* font_bitmap_temp = (u8*)calloc(bitmap_size, sizeof(u8));
    init_ttf_atlas(width_atlas, height_atlas, pixel_height, 96, 32,
                   "res/fonts/arial.ttf", font_bitmap_temp, &ui_context.font);
    u8* font_bitmap = (u8*)malloc(bitmap_size * 4 * sizeof(u8));
    memset(font_bitmap, UINT8_MAX, bitmap_size * 4 * sizeof(u8));
    for (u32 i = 0, j = 3; i < bitmap_size; ++i, j += 4)
    {
        font_bitmap[j] = font_bitmap_temp[i];
    }
    free(font_bitmap_temp);

    TextureProperties texture_properties = {
        .width = width_atlas,
        .height = height_atlas,
        .bytes = font_bitmap,
    };
    u32 font_texture = texture_create(&texture_properties, GL_RGBA8, GL_RGBA);
    free(texture_properties.bytes);

    u32 default_texture = create_default_texture();
    u32 file_icon_texture = load_icon("res/icons/icon_sheet.png");
    u32 arrow_icon_texture =
        load_icon_as_white("res/icons/arrow_sprite_sheet.png");

    U32Array textures = { 0 };
    array_create(&textures, 10);
    array_push(&textures, default_texture);
    array_push(&textures, font_texture);
    array_push(&textures, file_icon_texture);
    array_push(&textures, arrow_icon_texture);

    ui_context.render = rendering_properties_initialize(
        shader, textures, &vertex_buffer_layout, 100 * 4, 100 * 4);
}

void ui_context_begin(V2 dimensions, f64 delta_time, b8 check_collisions)
{
    ui_context.delta_time = delta_time;
    ui_context.dimensions = dimensions;

    ui_context.mvp.projection =
        ortho(0.0f, dimensions.width, dimensions.height, 0.0f, -1.0f, 1.0f);
    ui_context.mvp.view = m4d();
    ui_context.mvp.model = m4d();

    ui_context.render.vertices.size = 0;
    ui_context.current_index_offset = 0;

    for (u32 i = 0; i < ui_context.window_hover_clicked_indices.size; ++i)
    {
        HoverClickedIndex* hover_clicked_index =
            ui_context.window_hover_clicked_indices.data + i;
        hover_clicked_index->index = -1;
        hover_clicked_index->clicked = false;
        hover_clicked_index->pressed = false;
        hover_clicked_index->hover = false;
        hover_clicked_index->double_clicked = false;
    }
    ui_context.any_window_hold = false;
    b8 any_top_bar_pressed = false;
    for (u32 i = 0; i < ui_context.windows.size; ++i)
    {
        UiWindow* window = ui_context.windows.data + i;
        window->rendering_index_count = 0;
        window->rendering_index_offset = 0;
        ui_context.any_window_hold |= window->top_bar_hold;
        any_top_bar_pressed |= window->top_bar_pressed;
    }
    if (check_collisions && !ui_context.any_window_hold && !any_top_bar_pressed)
    {
        const b8 mouse_button_clicked =
            is_mouse_button_clicked(FTIC_MOUSE_BUTTON_LEFT);

        const MouseButtonEvent* mouse_button_event =
            event_get_mouse_button_event();

        const b8 mouse_button_pressed =
            mouse_button_event->activated &&
            mouse_button_event->action == FTIC_PRESS;

        const V2 mouse_position = event_get_mouse_position();
        for (u32 window_index = 0; window_index < ui_context.windows.size;
             ++window_index)
        {
            const AABBArray* aabbs =
                ui_context.window_aabbs.data + window_index;

            for (i32 aabb_index = aabbs->size; aabb_index >= 0; --aabb_index)
            {
                const AABB* aabb = aabbs->data + aabb_index;
                if (collision_point_in_aabb(mouse_position, aabb))
                {
                    HoverClickedIndex* hover_clicked_index =
                        ui_context.window_hover_clicked_indices.data +
                        window_index;
                    hover_clicked_index->index = aabb_index;
                    hover_clicked_index->hover = true;
                    hover_clicked_index->double_clicked =
                        mouse_button_event->double_clicked;
                    if (mouse_button_clicked || mouse_button_pressed)
                    {
                        hover_clicked_index->pressed = mouse_button_pressed;
                        hover_clicked_index->clicked = mouse_button_clicked;
                        change_window_order(&ui_context, window_index);
                    }
                    goto collision_check_done;
                }
            }
        }
    }
collision_check_done:;
}

internal b8 set_docking(const V2 contracted_position, const V2 contracted_size,
                        const V2 expanded_position, const V2 expanded_size,
                        const u32 index, u32* index_count)
{
    AABB aabb = {
        .min = contracted_position,
        .size = contracted_size,
    };

    b8 result = false;
    if (collision_point_in_aabb(event_get_mouse_position(), &aabb))
    {
        ui_context.dock_expanded_spaces[index] = quad(
            &ui_context.render.vertices, expanded_position, expanded_size,
            v4f(secondary_color.r, secondary_color.g, secondary_color.b, 0.4f),
            0.0f);
        *index_count += 6;
        result = true;
    }
    quad_border_rounded(&ui_context.render.vertices, index_count, aabb.min,
                        aabb.size, secondary_color, 1.0f, 0.4f, 3, 0.0f);
    return result;
}

void ui_context_end()
{
    const u32 index_offset = ui_context.current_index_offset;
    u32 index_count = 0;

    if (ui_context.any_window_hold)
    {
        const V2 docking_size_top_bottom = v2f(50.0f, 30.0f);
        const V2 docking_size_left_right = v2f(30.0f, 50.0f);
        const V2 middle_top_bottom = v2f(
            middle(ui_context.dimensions.width, docking_size_top_bottom.width),
            middle(ui_context.dimensions.height,
                   docking_size_top_bottom.height));
        const V2 middle_left_right = v2f(
            middle(ui_context.dimensions.width, docking_size_top_bottom.width),
            middle(ui_context.dimensions.height,
                   docking_size_top_bottom.height));

        const V2 expanded_size_top_bottom = v2f(
            ui_context.dimensions.width, ui_context.dimensions.height * 0.3f);
        const V2 expanded_size_left_right = v2f(
            ui_context.dimensions.width * 0.5f, ui_context.dimensions.height);

        memset(ui_context.docking_hit, 0, sizeof(ui_context.docking_hit));

        // Top
        ui_context.docking_hit[0] = set_docking(
            v2f(middle_top_bottom.x, 10.0f), docking_size_top_bottom, v2d(),
            expanded_size_top_bottom, 0, &index_count);

        // Right
        ui_context.docking_hit[1] = set_docking(
            v2f(ui_context.dimensions.x -
                    (docking_size_left_right.width + 10.0f),
                middle_left_right.y),
            docking_size_left_right,
            v2f(ui_context.dimensions.x - expanded_size_left_right.width, 0.0f),
            expanded_size_left_right, 1, &index_count);

        // Bottom
        ui_context.docking_hit[2] =
            set_docking(v2f(middle_top_bottom.x,
                            ui_context.dimensions.y -
                                (docking_size_top_bottom.height + 10.0f)),
                        docking_size_top_bottom,
                        v2f(0.0f, ui_context.dimensions.y -
                                      expanded_size_top_bottom.height),
                        expanded_size_top_bottom, 2, &index_count);

        // Left
        ui_context.docking_hit[3] = set_docking(
            v2f(10.0f, middle_left_right.y), docking_size_left_right,
            v2f(0, 0.0f), expanded_size_left_right, 3, &index_count);

        ui_context.current_index_offset = index_offset + index_count;

        UiWindow* focused_window = ui_context.windows.data;
        const i32 docking_space = focused_window->docking_space;
        if (docking_space >= 0)
        {
            U32Array* free_indices =
                ui_context.dock_free_indices + docking_space;
            U32Array* id_to_index = ui_context.dock_id_to_index + docking_space;
            DockArray* docks = ui_context.docks + docking_space;

            if (docks->size > 1)
            {
                i32 index = id_to_index->data[focused_window->docking_id];
                Dock dock = docks->data[index];
                for (u32 i = index; i < docks->size - 1; ++i)
                {
                    docks->data[i] = docks->data[i + 1];
                    id_to_index->data[docks->data[i].id] = i;
                }
                docks->size--;

                if (--index >= 0)
                {
                    V2 new_size =
                        v2_sub(docks->data[index].aabb.min, dock.aabb.min);
                    docks->data[index].aabb.min = dock.aabb.min;
                    v2_add_equal(&docks->data[index].aabb.size, new_size);
                }

                array_push(free_indices, focused_window->docking_id);
            }
            else
            {
                docks->size--;
                id_to_index->size--;
            }
            focused_window->docking_id = -1;
            focused_window->docking_space = -1;
        }
    }
    else
    {
        if (ui_context.docking_hit[3])
        {
        }
        if (ui_context.docking_hit[3])
        {
            U32Array* free_indices = ui_context.dock_free_indices + 3;
            U32Array* id_to_index = ui_context.dock_id_to_index + 3;
            DockArray* docks = ui_context.docks + 3;

            u32 id = get_id(docks->size, free_indices, id_to_index);
            Dock dock = { .id = id };
            if (docks->size)
            {
                f32 highest_x = 0.0f;
                f32 highest_width = 0.0f;
                for (u32 i = 0; i < docks->size; ++i)
                {
                    const f32 current_x = docks->data[i].aabb.min.x +
                                          docks->data[i].aabb.size.width;
                    const f32 current_width = docks->data[i].aabb.size.width;
                    highest_x = max(highest_x, current_x);
                    highest_width = max(highest_width, current_width);
                }
                AABB aabb = {
                    .min = ui_context.dock_expanded_spaces[3].min,
                    .size = ui_context.dock_expanded_spaces[3].size,
                };
                aabb.size.width -= highest_x * 0.3f;

                const f32 total_width_decrease = aabb.size.width;
                highest_x += total_width_decrease;

                f32 precent = aabb.size.width / highest_x;

                aabb.size.width -= total_width_decrease * precent;
                AABB last = aabb;
                for (i32 i = docks->size - 1; i >= 0; --i)
                {
                    Dock* other_dock = docks->data + i;
                    precent = other_dock->aabb.size.width / highest_x;
                    other_dock->aabb.min.x = last.min.x + last.size.width;
                    other_dock->aabb.size.width -=
                        total_width_decrease * precent;
                    last = other_dock->aabb;
                }
                dock.aabb = aabb;
            }
            else
            {
                dock.aabb = ui_context.dock_expanded_spaces[3];
            }
            array_push(docks, dock);
            ui_context.windows.data[0].docking_space = 3;
            ui_context.windows.data[0].docking_id = id;
        }
        memset(ui_context.docking_hit, 0, sizeof(ui_context.docking_hit));
    }

    rendering_properties_check_and_grow_buffers(
        &ui_context.render, ui_context.current_index_offset);

    buffer_set_sub_data(ui_context.render.vertex_buffer_id, GL_ARRAY_BUFFER, 0,
                        sizeof(Vertex) * ui_context.render.vertices.size,
                        ui_context.render.vertices.data);

    rendering_properties_begin_draw(&ui_context.render, &ui_context.mvp);
    for (i32 i = ui_context.windows.size - 1; i >= 0; --i)
    {
        const UiWindow* window = ui_context.windows.data + i;
        rendering_properties_draw(window->rendering_index_offset,
                                  window->rendering_index_count);
    }
    rendering_properties_draw(index_offset, index_count);
    rendering_properties_end_draw(&ui_context.render);
}

u32 ui_window_create()
{
    u32 id = get_id(ui_context.windows.size, &ui_context.free_indices,
                    &ui_context.id_to_index);

    UiWindow window = {
        .id = id,
        .docking_space = -1,
        .docking_id = -1,
        .index = ui_context.windows.size,
        .dimensions = v2f(100.0f, 100.0f),
    };
    array_push(&ui_context.windows, window);

    AABBArray aabbs = { 0 };
    array_create(&aabbs, 10);
    array_push(&ui_context.window_aabbs, aabbs);

    HoverClickedIndex hover_clicked_index = { .index = -1 };
    array_push(&ui_context.window_hover_clicked_indices, hover_clicked_index);

    return id;
}

void ui_window_begin(u32 window_id, b8 top_bar)
{
    ui_context.current_window_id = window_id;
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;

    if (window->docking_space >= 0 && !window->top_bar_hold)
    {
        const Dock* dock = ui_context.docks[window->docking_space].data +
                           ui_context.dock_id_to_index[window->docking_space]
                               .data[window->docking_id];

        window->position = dock->aabb.min;
        window->dimensions = dock->aabb.size;
    }

    window->rendering_index_offset = ui_context.current_index_offset;
    aabbs->size = 0;
    array_push(aabbs, quad(&ui_context.render.vertices, window->position,
                           window->dimensions, clear_color, 0.0f));
    window->rendering_index_count += 6;
    window->area_hit =
        collision_point_in_aabb(event_get_mouse_position(), array_back(aabbs));

    window->first_item_position = window->position;
    window->first_item_position.y += 20.0f;
}

void ui_window_end(const f64 delta_time)
{
    const u32 window_index =
        ui_context.id_to_index.data[ui_context.current_window_id];
    UiWindow* window = ui_context.windows.data + window_index;
    AABBArray* aabbs = ui_context.window_aabbs.data + window_index;

    const V2 top_bar_dimensions = v2f(window->dimensions.width, 20.0f);
    HoverClickedIndex hover_clicked_index =
        ui_context.window_hover_clicked_indices.data[window_index];

    if (hover_clicked_index.index == (i32)aabbs->size)
    {
        if (hover_clicked_index.pressed && !window->top_bar_pressed)
        {
            window->top_bar_offset = event_get_mouse_position();
            window->top_bar_pressed = true;
        }
    }
    if (event_get_mouse_button_event()->action == FTIC_RELEASE)
    {
        window->top_bar_pressed = false;
        window->top_bar_hold = false;
    }

    if (window->top_bar_hold)
    {
        window->position =
            v2_add(event_get_mouse_position(), window->top_bar_offset);
    }
    else if (window->top_bar_pressed)
    {
        if (v2_distance(window->top_bar_offset, event_get_mouse_position()) >=
            10.0f)
        {
            window->top_bar_hold = true;
            window->top_bar_offset =
                v2_sub(window->position, event_get_mouse_position());
        }
    }

    array_push(aabbs, quad(&ui_context.render.vertices, window->position,
                           top_bar_dimensions, high_light_color, 0.0f));
    window->rendering_index_count += 6;

    quad_border(&ui_context.render.vertices, &window->rendering_index_count,
                window->position, top_bar_dimensions, border_color, 1.0f, 0.0f);

    /*
    if (!window->scroll_bar.dragging)
    {
        if (event_get_mouse_wheel_event()->activated && window->area_hit)
        {
            window->end_scroll_offset =
                set_scroll_offset(window->total_height, window->dimensions.y,
                                  window->end_scroll_offset);
        }
        window->current_scroll_offset =
            smooth_scroll(delta_time, window->end_scroll_offset,
                          window->current_scroll_offset);
    }
    */

    const b8 in_focus = window_index == 0;
    V4 color = border_color;
    if (in_focus)
    {
        color = secondary_color;
    }
    quad_border(&ui_context.render.vertices, &window->rendering_index_count,
                window->position, window->dimensions, color, 1.0f, 0.0f);

    /*
    render->scissor.min.x = window->position.x;
    render->scissor.min.y = context->dimensions.y - window->position.y;
    render->scissor.min.y -= window->dimensions.y;
    render->scissor.size = window->dimensions;
    */

    // NOTE(Linus): += also works
    ui_context.current_index_offset =
        window->rendering_index_offset + window->rendering_index_count;
}

void ui_window_add_directory_list(V2 position)
{
}

void ui_window_add_folder_list(V2 position)
{
}

void ui_window_add_file_list(V2 position)
{
}

/*
internal b8 item_in_view(const f32 position_y, const f32 height,
                         const f32 window_height)
{
    const f32 value = position_y + height;
    return closed_interval(-height, value, window_height + height);
}

internal void directory_item(i32 index, V2 starting_position, const f32 height,
                             const f32 icon_index, V4 texture_coordinates,
                             DirectoryItem* item,
                             SelectedItemValues* selected_item_values,
                             i32* hit_index, RenderingProperties* render)
{

    HoverClickedIndex hover_clicked

        b8 selected = false;
    u32* check_if_selected = NULL;
    if (selected_item_values)
    {
        check_if_selected = hash_table_get_char_u32(
            &selected_item_values->selected_items, item->path);
        selected = check_if_selected ? true : false;
    }

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
        const MouseButtonEvent* event = event_get_mouse_button_event();
        if (selected_item_values && !check_if_selected && event->activated &&
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

i32 ui_window_add_directory_item_item_list(
    UiWindow* window, V2 position, const DirectoryItemArray* items,
    const f32 icon_index, const V4 texture_coordinates, const f32 item_height,
    SelectedItemValues* selected_items_values)
{
    for (i32 i = 0; i < (i32)items->size; ++i)
    {
        if (item_in_view(position.y, item_height, window->dimensions.height))
        {
            directory_item(application, hit, i, starting_position,
                           application->font.pixel_height + padding, item_width,
                           item_height, icon_index, texture_coordinates,
                           check_collision, pulse_x, &items->data[i],
                           selected_item_values, &hit_index, render);
        }
        position.y += item_height;
    }
}
*/

void ui_window_add_input_field(V2 position)
{
}
