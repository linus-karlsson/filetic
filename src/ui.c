#include "ui.h"
#include "event.h"
#include "application.h"
#include "opengl_util.h"
#include <glad/glad.h>

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
    if (window_in_focus_index == context->windows.size - 1)
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
    move_to_back(RenderingProperties, &context->window_rendering_properties,
                 window_in_focus_index);
}

void ui_context_create(UiContext* context)
{
    array_create(&context->id_to_index, 100);
    array_create(&context->free_indices, 100);
    array_create(&context->windows, 10);
    array_create(&context->window_aabbs, 10);
    array_create(&context->window_hover_clicked_indices, 10);
    array_create(&context->window_rendering_properties, 10);
}

void ui_context_begin(UiContext* context, V2 dimensions, f64 delta_time,
                      b8 check_collisions)
{
    context->delta_time = delta_time;
    context->dimensions = dimensions;

    context->mvp.projection =
        ortho(0.0f, dimensions.width, dimensions.height, 0.0f, -1.0f, 1.0f);
    context->mvp.view = m4d();
    context->mvp.model = m4d();

    for (u32 i = 0; i < context->window_hover_clicked_indices.size; ++i)
    {
        HoverClickedIndex* hover_clicked_index =
            context->window_hover_clicked_indices.data + i;
        hover_clicked_index->index = -1;
        hover_clicked_index->clicked = false;
        hover_clicked_index->hover = false;
        hover_clicked_index->double_clicked = false;
    }
    for (u32 i = 0; i < context->window_rendering_properties.size; ++i)
    {
        rendering_properties_clear(context->window_rendering_properties.data +
                                   i);
    }
    if (check_collisions)
    {
        const V2 mouse_position = event_get_mouse_position();
        for (u32 window_index = 0; window_index < context->windows.size;
             ++window_index)
        {
            const AABBArray* aabbs = context->window_aabbs.data + window_index;

            for (i32 aabb_index = aabbs->size; aabb_index >= 0; --aabb_index)
            {
                const AABB* aabb = aabbs->data + aabb_index;
                if (collision_point_in_aabb(mouse_position, aabb))
                {
                    HoverClickedIndex* hover_clicked_index =
                        context->window_hover_clicked_indices.data +
                        window_index;
                    hover_clicked_index->index = aabb_index;
                    hover_clicked_index->hover = true;
                    hover_clicked_index->double_clicked =
                        event_get_mouse_button_event()->double_clicked;
                    if (is_mouse_button_clicked(FTIC_MOUSE_BUTTON_LEFT))
                    {
                        hover_clicked_index->clicked = true;
                        change_window_order(context, window_index);
                    }
                    goto collision_check_done;
                }
            }
        }
    }
collision_check_done:;
}

void ui_context_end(UiContext* context)
{
    for (i32 i = context->window_rendering_properties.size - 1; i >= 0; --i)
    {
        RenderingProperties* render =
            context->window_rendering_properties.data + i;

        rendering_properties_check_and_grow_buffers(render);

        buffer_set_sub_data(render->vertex_buffer_id, GL_ARRAY_BUFFER, 0,
                            sizeof(Vertex) * render->vertices.size,
                            render->vertices.data);

        rendering_properties_draw(render, &context->mvp);
    }
}

u32 ui_window_create(UiContext* context, RenderingProperties render)
{
    u32 id = 0;
    if (context->free_indices.size)
    {
        id = *array_back(&context->free_indices);
        context->free_indices.size--;
    }
    else
    {
        id = context->id_to_index.size;
    }
    context->id_to_index.data[id] = context->windows.size;

    UiWindow window = { .id = id, .index = context->windows.size };
    array_push(&context->windows, window);

    AABBArray aabbs = { 0 };
    array_create(&aabbs, 10);
    array_push(&context->window_aabbs, aabbs);

    HoverClickedIndex hover_clicked_index = { .index = -1 };
    array_push(&context->window_hover_clicked_indices, hover_clicked_index);

    array_push(&context->window_rendering_properties, render);

    return id;
}

UiWindow* ui_window_begin(u64 window_id, UiContext* context)
{
    const u32 window_index = context->id_to_index.data[window_id];
    UiWindow* window = context->windows.data + window_index;
    RenderingProperties* render =
        context->window_rendering_properties.data + window_index;
    AABBArray* aabbs = context->window_aabbs.data + window_index;

    aabbs->size = 0;

    array_push(aabbs, quad(&render->vertices, window->position,
                           window->dimensions, clear_color, 0.0f));

    render->index_count++;
    window->area_hit =
        collision_point_in_aabb(event_get_mouse_position(), array_back(aabbs));

    return window;
}

void ui_window_end(UiWindow* window, UiContext* context, const f64 delta_time)
{
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

    const u32 window_index = context->id_to_index.data[window->id];
    const b8 in_focus = window_index == 0;

    RenderingProperties* render =
        context->window_rendering_properties.data + window_index;
    AABBArray* aabbs = context->window_aabbs.data + window_index;

    if (in_focus)
    {
        array_push(aabbs, quad_border(&render->vertices, &render->index_count,
                                      window->position, window->dimensions,
                                      border_color, 1.0f, 0.0f));
    }

    render->scissor.min.x = window->position.x;
    render->scissor.min.y = context->dimensions.y - window->position.y;
    render->scissor.min.y -= window->dimensions.y;
    render->scissor.size = window->dimensions;
}

void ui_window_add_directory_list(UiWindow* window, V2 position)
{
}

void ui_window_add_folder_list(UiWindow* window, V2 position)
{
}

void ui_window_add_file_list(UiWindow* window, V2 position)
{
}

void ui_window_add_input_field(UiWindow* window, V2 position)
{
}
