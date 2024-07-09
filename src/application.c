#include "application.h"
#include "hash.h"
#include "logging.h"
#include "texture.h"
#include "opengl_util.h"
#include <stdio.h>
#include <string.h>
#include <glad/glad.h>

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
                *text_color = lighter_color;
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
                    // directory_reload(arguments->directory);
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

internal void open_window(ApplicationContext* app, u32 window_id)
{
    UiWindow* window = ui_window_get(window_id);
    V2 end_size = v2i(200.0f);
    V2 end_position = v2f(middle(app->dimensions.width, end_size.width),
                          middle(app->dimensions.height, end_size.height));

    window->size = v2i(0.0f);
    window->position = v2f(middle(app->dimensions.width, window->size.width),
                           middle(app->dimensions.height, window->size.height));

    ui_window_start_position_animation(window, window->position, end_position);
    ui_window_start_size_animation(window, window->size, end_size);
}

internal b8 top_bar_menu_selection(u32 index, b8 hit, b8 should_close,
                                   b8 item_clicked, V4* text_color, void* data)
{
    ApplicationContext* app = (ApplicationContext*)data;
    if (item_clicked)
    {
        switch (index)
        {
            case 0:
            {
                if (!app->show_quick_access)
                {
                    app->show_quick_access = true;
                    open_window(app, app->quick_access_window);
                }
                return true;
            }
            case 1:
            {
                if (!app->show_search_page)
                {
                    app->show_search_page = true;
                    open_window(app, app->quick_access_window);
                }
                return true;
            }
        }
    }
    return should_close;
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

        if (platform_directory_exists(path))
        {
            path[path_length++] = '\\';
            path[path_length++] = '*';

            array_push(&app->tabs, directory_tab_add(path));

            path[path_length - 2] = '\0';
            path[path_length - 1] = '\0';
        }
        free(path);
    }
    fclose(file);
}

internal void quick_access_save(DirectoryItemArray* array)
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

internal void quick_access_load(DirectoryItemArray* array)
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
    u32 font_texture = texture_create(texture_properties, GL_RGBA8, GL_RGBA);
    free(texture_properties->bytes);

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

    const u32 main_texture_count = 4;
    U32Array main_textures = { 0 };
    array_create(&main_textures, main_texture_count);
    array_push(&main_textures, default_texture);
    array_push(&main_textures, font_texture);
    array_push(&main_textures, file_icon_texture);
    array_push(&main_textures, arrow_icon_texture);

    *main_render = rendering_properties_initialize(
        main_shader, main_textures, &vertex_buffer_layout, 100 * 4, 100 * 6);
}

u8* application_initialize(ApplicationContext* app)
{
    app->window = window_create("FileTic", 1250, 800);
    event_initialize(app->window);
    platform_init_drag_drop();
    thread_initialize(100000, 8, &app->thread_queue);

    app->font = (FontTTF){ 0 };
    const i32 width_atlas = 512;
    const i32 height_atlas = 512;
    const f32 pixel_height = 16;
    const u32 bitmap_size = width_atlas * height_atlas;
    u8* font_bitmap_temp = (u8*)calloc(bitmap_size, sizeof(u8));
    init_ttf_atlas(width_atlas, height_atlas, pixel_height, 96, 32,
                   "res/fonts/arial.ttf", font_bitmap_temp, &app->font);

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

    array_create(&app->tabs, 10);
    app->tab_index = 0;

    load_application_state(app);
    if (app->tabs.size == 0)
    {
        array_push(&app->tabs, directory_tab_add("C:\\*"));
    }
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
    app->top_bar_window = app->windows.data[0];
    app->bottom_bar_window = app->windows.data[1];
    app->quick_access_window = app->windows.data[2];
    app->search_result_window = app->windows.data[3];
    app->preview_window = app->windows.data[4];

    array_create(&app->free_window_ids, 10);
    array_create(&app->tab_windows, 20);
    for (u32 i = 0; i < 20; ++i)
    {
        array_push(&app->tab_windows, ui_window_create());
    }

    app->current_tab_window_index = 0;
    for (u32 i = 0; i < app->tabs.size; ++i)
    {
        app->tabs.data[i].window_id =
            app->tab_windows.data[app->current_tab_window_index++];
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
        [PROPERTIES_OPTION_INDEX] = "Properties",
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
               string_copy_d(options[PROPERTIES_OPTION_INDEX]));

    array_create(&app->quick_access_folders, 10);
    quick_access_load(&app->quick_access_folders);
    app->show_quick_access = true;

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

    app->top_bar_menu = (DropDownMenu2){
        .index_count = &app->main_index_count,
        .tab_index = -1,
        .menu_options_selection = top_bar_menu_selection,
        .render = &app->main_render,
    };
    array_create(&app->top_bar_menu.options, static_array_size(options));
    char* top_bar_menu_options[] = {
        "Quick access",
        "Search result",
    };
    array_push(&app->top_bar_menu.options,
               string_copy_d(top_bar_menu_options[0]));
    array_push(&app->top_bar_menu.options,
               string_copy_d(top_bar_menu_options[1]));

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
    for (u32 i = 0; i < app->top_bar_menu.options.size; ++i)
    {
        free(app->top_bar_menu.options.data[i]);
    }
    free(app->windows.data);
    free(app->tab_windows.data);
    free(app->free_window_ids.data);

    ui_context_destroy();

    buffer_delete(app->main_render.vertex_buffer_id);
    buffer_delete(app->main_render.index_buffer_id);
    for (u32 i = 0; i < app->main_render.textures.size; ++i)
    {
        texture_delete(app->main_render.textures.data[i]);
    }
    free(app->main_render.textures.data);

    quick_access_save(&app->quick_access_folders);

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
    glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    app->mvp.projection = ortho(0.0f, app->dimensions.width,
                                app->dimensions.height, 0.0f, -1.0f, 1.0f);

    app->main_index_count = 0;
    rendering_properties_clear(&app->main_render);

    if (event_is_ctrl_and_key_pressed(FTIC_KEY_T))
    {
        array_push(&app->tabs, directory_tab_add("C:\\*"));
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
        open_window(app, window_id);
    }

    if (event_is_ctrl_and_key_range_pressed(FTIC_KEY_1, FTIC_KEY_9))
    {
        const u32 index = event_get_key_event()->key - FTIC_KEY_1;
        if (index < app->tabs.size)
        {
            app->tab_index = index;
        }
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
        platform_reset_directory(&directory);
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
