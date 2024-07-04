#include "application.h"
#include "hash.h"
#include "logging.h"
#include <stdio.h>
#include <string.h>
#include <glad/glad.h>

DirectoryPage* current_directory(DirectoryHistory* history)
{
    return history->history.data + history->current_index;
}

internal void save_application_state(ApplicationContext* application)
{
    FILE* file = fopen("saved/application.txt", "wb");
    if (file == NULL)
    {
        log_file_error("saved/application.txt");
        return;
    }

    fwrite(&application->tabs.size, sizeof(application->tabs.size), 1, file);
    for (u32 i = 0; i < application->tabs.size; ++i)
    {
        char* path =
            current_directory(&application->tabs.data[i].directory_history)
                ->directory.parent;
        u32 path_length = (u32)strlen(path);
        fwrite(&path_length, sizeof(u32), 1, file);
        fwrite(path, sizeof(char), path_length, file);
    }
    fclose(file);
}

internal void load_application_state(ApplicationContext* application)
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

        if (platform_directory_exists(path))
        {
            path[path_length++] = '\\';
            path[path_length++] = '*';

            array_push(&application->tabs, tab_add(path));

            path[path_length - 2] = '\0';
            path[path_length - 1] = '\0';
        }
        free(path);
    }
    fclose(file);
}

u8* application_initialize(ApplicationContext* application)
{
    application->window = window_create("FileTic", 1250, 800);
    event_initialize(application->window);

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
    free(font_bitmap_temp);

    array_create(&application->tabs, 10);
    application->tab_index = 0;

    load_application_state(application);
    if (application->tabs.size == 0)
    {
        array_push(&application->tabs, tab_add("C:\\*"));
    }
    application->last_time = platform_get_time();
    application->delta_time = 0.0f;
    application->mvp = (MVP){ 0 };
    application->mvp.view = m4d();
    application->mvp.model = m4d();

    application->last_moved_time = window_get_time();

    return font_bitmap;
}

void application_uninitialize(ApplicationContext* application)
{
    window_destroy(application->window);

    save_application_state(application);
    for (u32 i = 0; i < application->tabs.size; ++i)
    {
        DirectoryTab* tab = application->tabs.data + i;
        for (u32 j = 0; j < tab->directory_history.history.size; j++)
        {
            platform_reset_directory(
                &tab->directory_history.history.data[j].directory);
        }
        free(tab->directory_history.history.data);
        reset_selected_items(&tab->selected_item_values);
    }
    event_uninitialize();
}

void application_begin_frame(ApplicationContext* application)
{
    int width, height;
    window_get_size(application->window, &width, &height);
    application->dimensions = v2f((f32)width, (f32)height);
    double x, y;
    window_get_mouse_position(application->window, &x, &y);
    application->mouse_position = v2f((f32)x, (f32)y);
    if (event_get_mouse_move_event()->activated)
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

f64 application_get_last_mouse_move_time(const ApplicationContext* application)
{
    return window_get_time() - application->last_moved_time;
}

DirectoryTab tab_add(const char* dir)
{
    DirectoryHistory directory_history = { 0 };
    array_create(&directory_history.history, 10);

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

void reset_selected_items(SelectedItemValues* selected_item_values)
{
    for (u32 i = 0; i < selected_item_values->paths.size; ++i)
    {
        free(selected_item_values->paths.data[i]);
    }
    hash_table_clear_char_u32(&selected_item_values->selected_items);
    selected_item_values->paths.size = 0;
}
