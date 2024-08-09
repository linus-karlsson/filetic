#include "directory.h"
#include "hash.h"
#include "logging.h"
#include "hash.h"
#include <string.h>

DirectoryPage* directory_current(DirectoryHistory* history)
{
    return history->history.data + history->current_index;
}

void load_thumpnails(void* data)
{
    LoadThumpnailData* arguments = (LoadThumpnailData*)data;

    IdTextureProperties value = { .id = guid_copy(&arguments->file_id) };
    texture_load_full_path(arguments->file_path, &value.texture_properties);
    if (!value.texture_properties.bytes)
    {
        free(arguments->file_path);
        free(arguments);
        return;
    }

    if (value.texture_properties.width > arguments->size ||
        value.texture_properties.height > arguments->size)
    {
        texture_resize(&value.texture_properties, arguments->size, arguments->size);
    }

    platform_mutex_lock(&arguments->array->mutex);
    if (arguments->array->array.data == NULL)
    {
        free(value.texture_properties.bytes);
        free(arguments->file_path);
        free(arguments);
        return;
    }
    array_push(&arguments->array->array, value);

    platform_mutex_unlock(&arguments->array->mutex);
    free(arguments->file_path);
    free(arguments);
}

internal void look_for_same_items(const DirectoryItemArray* existing_items,
                                  DirectoryItemArray* reloaded_items)
{
    for (u32 i = 0; i < reloaded_items->size; ++i)
    {
        DirectoryItem* reloaded_item = reloaded_items->data + i;
        for (u32 j = 0; j < existing_items->size; ++j)
        {
            DirectoryItem* existing_item = existing_items->data + j;
            if (guid_compare(reloaded_item->id, existing_item->id) == 0)
            {
                reloaded_item->animation_offset = existing_item->animation_offset;
                reloaded_item->texture_id = existing_item->texture_id;
                reloaded_item->texture_width = existing_item->texture_width;
                reloaded_item->texture_height = existing_item->texture_height;

                reloaded_item->reload_thumbnail = existing_item->reload_thumbnail;
                reloaded_item->rename = existing_item->rename;
                break;
            }
        }
    }
}

void directory_reload(DirectoryPage* directory_page)
{
    char* path = directory_page->directory.parent;
    u32 length = (u32)strlen(path);
    path[length++] = '\\';
    path[length++] = '*';
    Directory reloaded_directory = platform_get_directory(path, length, true);
    path[length - 2] = '\0';
    path[length - 1] = '\0';

    look_for_same_items(&directory_page->directory.items, &reloaded_directory.items);

    platform_reset_directory(&directory_page->directory, false);
    directory_page->directory = reloaded_directory;
    directory_sort(directory_page);
}

void directory_paste_in_directory(DirectoryPage* current_directory)
{
    CharPtrArray pasted_paths = { 0 };
    array_create(&pasted_paths, 10);
    platform_paste_from_clipboard(&pasted_paths);
    if (pasted_paths.size)
    {
        platform_paste_to_directory(&pasted_paths, current_directory->directory.parent);
        for (u32 i = 0; i < pasted_paths.size; ++i)
        {
            free(pasted_paths.data[i]);
        }
        directory_reload(current_directory);
    }
    free(pasted_paths.data);
}

internal i32 name_compare_function(const DirectoryItem* first, const DirectoryItem* second)
{
    return string_compare_case_insensitive(item_namec(first), item_namec(second));
}

internal i32 date_compare_function(const DirectoryItem* first, const DirectoryItem* second)
{
    PlatformTime first_time = platform_time_from_u64(first->last_write_time);
    PlatformTime second_time = platform_time_from_u64(second->last_write_time);
    return platform_time_compare(&first_time, &second_time);
}

internal void merge(DirectoryItem* array,
                    i32 (*compare_function)(const DirectoryItem*, const DirectoryItem*), u32 left,
                    u32 mid, u32 right)
{
    u32 n1 = mid - left + 1;
    u32 n2 = right - mid;

    DirectoryItem* left_array = (DirectoryItem*)malloc(n1 * sizeof(DirectoryItem));
    DirectoryItem* right_array = (DirectoryItem*)malloc(n2 * sizeof(DirectoryItem));

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
        if (compare_function(left_array + i, right_array + j) <= 0)
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

internal void merge_sort(DirectoryItem* array,
                         i32 (*compare_function)(const DirectoryItem*, const DirectoryItem*),
                         u32 left, u32 right)
{
    if (left < right)
    {
        u32 mid = left + (right - left) / 2;
        merge_sort(array, compare_function, left, mid);
        merge_sort(array, compare_function, mid + 1, right);
        merge(array, compare_function, left, mid, right);
    }
}

void directory_sort_by_name(DirectoryItemArray* array)
{
    merge_sort(array->data, name_compare_function, 0, array->size - 1);
}

void directory_merge_sort_by_date(DirectoryItemArray* array)
{
    merge_sort(array->data, date_compare_function, 0, array->size - 1);
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

    DirectoryItem* output = (DirectoryItem*)calloc(array->size, sizeof(DirectoryItem));
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

void directory_sort_by_date(DirectoryItemArray* array)
{
    if (array->size <= 1) return;

    DirectoryItem* output = (DirectoryItem*)calloc(array->size, sizeof(DirectoryItem));
    u32 count[256] = { 0 };

    for (u32 shift = 0, s = 0; shift < 8; ++shift, s += 8)
    {
        memset(count, 0, sizeof(count));

        for (u32 i = 0; i < array->size; ++i)
        {
            count[(array->data[i].last_write_time >> s) & 0xff]++;
        }

        for (u32 i = 1; i < 256; ++i)
        {
            count[i] += count[i - 1];
        }

        for (i32 i = array->size - 1; i >= 0; --i)
        {
            u32 index = (array->data[i].last_write_time >> s) & 0xff;
            output[--count[index]] = array->data[i];
        }
        DirectoryItem* tmp = array->data;
        array->data = output;
        output = tmp;
    }
    free(output);
}

void directory_sort(DirectoryPage* directory_page)
{
    DirectoryItemArray* items = &directory_page->directory.items;
    switch (directory_page->sort_by)
    {
        case SORT_NAME:
        {
            directory_sort_by_name(items);
            if (directory_page->sort_count == 2)
            {
                directory_flip_array(items);
            }
            break;
        }
        case SORT_SIZE:
        {
            directory_sort_by_size(items);
            if (directory_page->sort_count == 2)
            {
                directory_flip_array(items);
            }
            break;
        }
        case SORT_DATE:
        {
            directory_sort_by_date(items);
            if (directory_page->sort_count == 2)
            {
                directory_flip_array(items);
            }
            break;
        }
        default:
        {
            directory_sort_by_name(items);

            // NOTE: very inefficient but does not happen very often.
            DirectoryItemArray temp = { 0 };
            array_create(&temp, items->size);

            i32 iterations = (i32)items->size;
            for (i32 i = 0; i < iterations; ++i)
            {
                DirectoryItem* item = items->data + i;
                if (item->type == FOLDER_DEFAULT)
                {
                    array_push(&temp, *item);
                    for (i32 j = i; j < iterations - 1; ++j)
                    {
                        items->data[j] = items->data[j + 1];
                    }
                    --i;
                    --iterations;
                }
            }
            u32 files_to_move = items->size - temp.size;
            memmove(items->data + temp.size, items->data, files_to_move * sizeof(DirectoryItem));
            memcpy(items->data, temp.data, temp.size * sizeof(DirectoryItem));

            array_free(&temp);
            break;
        }
    }
}

void directory_history_update_directory_change_handle(DirectoryHistory* directory_history)
{
    directory_unlisten_to_directory_changes(directory_history->change_handle);
    directory_history->change_handle = directory_listen_to_directory_changes(
        directory_current(directory_history)->directory.parent);
}

internal u32 look_for_and_get_thumbnails(const DirectoryItemArray* files,
                                         ThreadTaskQueue* task_queue,
                                         SafeIdTexturePropertiesArray* textures)
{
    u32 count = 0;
    for (u32 i = 0; i < files->size; ++i)
    {
        const DirectoryItem* item = files->data + i;
        const char* extension = file_get_extension(item->path, (u32)strlen(item->path));

        if (extension && (!strcmp(extension, "jpg") || !strcmp(extension, "png")))
        {
            LoadThumpnailData* thump_nail_data =
                (LoadThumpnailData*)calloc(1, sizeof(LoadThumpnailData));
            thump_nail_data->file_id = guid_copy(&item->id);
            thump_nail_data->array = textures;
            thump_nail_data->file_path = item->path;
            ThreadTask task = {
                .data = thump_nail_data,
                .task_callback = load_thumpnails,
            };
            thread_tasks_push(task_queue, &task, 1, NULL);
            ++count;
        }
    }
    return count;
}

internal u32 count_image_obj_files(const DirectoryItemArray* items)
{
    u32 count = 0;
    for (u32 i = 0; i < items->size; ++i)
    {
        DirectoryItem* item = items->data + i;
        count += (item->type == FILE_PNG || item->type == FILE_JPG || item->type == FILE_OBJ);
    }
    return count;
}

internal b8 should_be_grid_view(const DirectoryPage* page)
{
    u32 count = count_image_obj_files(&page->directory.items);

    if (count)
    {
        if (((f32)count / (f32)(page->directory.items.size)) >= 0.8f)
        {
            return true;
        }
    }
    return false;
}

b8 directory_go_to(char* path, u32 length, DirectoryHistory* directory_history)
{
    b8 result = false;
    char saved_chars[3];
    saved_chars[0] = path[length];
    path[length] = '\0';
    if (string_compare_case_insensitive(path,
                                        directory_current(directory_history)->directory.parent) &&
        platform_directory_exists(path))
    {
        path[length++] = '\\';
        saved_chars[1] = path[length];
        path[length++] = '*';
        saved_chars[2] = path[length];
        path[length] = '\0';
        DirectoryPage new_page = { 0 };
        new_page.directory = platform_get_directory(path, length, true);
        for (i32 i = directory_history->history.size - 1;
             i >= (i32)directory_history->current_index + 1; --i)
        {
            platform_reset_directory(&directory_history->history.data[i].directory, true);
        }
        directory_history->history.size = ++directory_history->current_index;
        array_push(&directory_history->history, new_page); // size + 1
                                                           //
        DirectoryPage* page = array_back(&directory_history->history);
#if 0
        u32 count = look_for_and_get_thumbnails(&page->directory.files,
                                                task_queue, texures);

#endif
        page->grid_view = should_be_grid_view(page);

        path[length--] = saved_chars[2];
        path[length--] = saved_chars[1];
        result = true;
        directory_history_update_directory_change_handle(directory_history);
    }
    path[length] = saved_chars[0];
    return result;
}

void directory_open_folder(char* folder_path, DirectoryHistory* directory_history)
{
    u32 length = (u32)strlen(folder_path);
    directory_go_to(folder_path, length, directory_history);
}

void directory_move_in_history(const i32 index_add, SelectedItemValues* selected_item_values,
                               DirectoryHistory* directory_history)
{
    directory_history->current_index += index_add;
    DirectoryPage* current = directory_current(directory_history);
    directory_clear_selected_items(selected_item_values);
    directory_reload(current);
    for (u32 i = 0; i < current->directory.items.size; ++i)
    {
        DirectoryItem* item = current->directory.items.data + i;
        if (item->texture_id)
        {
            texture_delete(item->texture_id);
            item->texture_id = 0;
            item->reload_thumbnail = false;
        }
    }
    directory_history_update_directory_change_handle(directory_history);
}

b8 directory_can_go_up(char* parent)
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

void directory_go_up(DirectoryHistory* directory_history)
{
    DirectoryPage* current = directory_current(directory_history);
    char* parent = current->directory.parent;
    u32 parent_length = get_path_length(parent, (u32)strlen(parent)) - 1;

    char* new_parent = (char*)calloc(parent_length + 1, sizeof(char));
    memcpy(new_parent, parent, parent_length);
    directory_go_to(new_parent, parent_length, directory_history);
    free(new_parent);
}

void directory_tab_add(const char* dir, ThreadTaskQueue* task_queue, DirectoryTab* tab)
{
    array_create(&tab->directory_history.history, 10);

    DirectoryPage page = { 0 };
    page.directory = platform_get_directory(dir, (u32)strlen(dir), true);
    array_push(&tab->directory_history.history, page);

    tab->directory_history.change_handle =
        directory_listen_to_directory_changes(page.directory.parent);

    safe_array_create(&tab->textures, 10);
    safe_array_create(&tab->objects, 10);

    array_create(&tab->directory_list.selected_item_values.paths, 10);
    tab->directory_list.selected_item_values.selected_items =
        hash_table_create_guid(100, hash_guid);

    array_create(&tab->directory_list.inputs, 10);

    DirectoryPage* last_page = array_back(&tab->directory_history.history);
#if 0
    look_for_and_get_thumbnails(&last_page->directory.files, task_queue,
                                &tab->textures);
#endif
    last_page->grid_view = should_be_grid_view(last_page);
}

void directory_tab_clear(DirectoryTab* tab)
{
    for (u32 i = 0; i < tab->directory_history.history.size; i++)
    {
        platform_reset_directory(&tab->directory_history.history.data[i].directory, true);
    }
    array_free(&tab->directory_history.history);

    platform_mutex_lock(&tab->textures.mutex);
    free(tab->textures.array.data);
    tab->textures.array.data = NULL;
    platform_mutex_unlock(&tab->textures.mutex);
    platform_mutex_destroy(&tab->textures.mutex);

    directory_unlisten_to_directory_changes(tab->directory_history.change_handle);
    for (u32 i = 0; i < tab->directory_list.inputs.size; ++i)
    {
        ui_input_buffer_delete(tab->directory_list.inputs.data + i);
    }
    directory_clear_selected_items(&tab->directory_list.selected_item_values);
}

void directory_clear_selected_items(SelectedItemValues* selected_item_values)
{
    for (u32 i = 0; i < selected_item_values->paths.size; ++i)
    {
        free(selected_item_values->paths.data[i]);
    }
    hash_table_clear_guid(&selected_item_values->selected_items);
    selected_item_values->paths.size = 0;
}

void directory_remove_selected_item(SelectedItemValues* selected_item_values, const FticGUID guid)
{
    CellGuid* cell = hash_table_remove_guid(&selected_item_values->selected_items, guid);
    if (cell)
    {
        CharPtrArray* paths = &selected_item_values->paths;
        for (u32 i = 0; i < paths->size; ++i)
        {
            if (strcmp(paths->data[i], cell->value) == 0)
            {
                char* temp = paths->data[i];
                for (u32 j = i; j < paths->size - 1; ++j)
                {
                    paths->data[j] = paths->data[j + 1];
                }
                paths->size--;
                free(temp);
                break;
            }
        }
    }
}

