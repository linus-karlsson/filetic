#include "directory.h"
#include "hash.h"
#include "logging.h"
#include "hash.h"
#include <string.h>

DirectoryPage* directory_current(DirectoryHistory* history)
{
    return history->history.data + history->current_index;
}

internal void look_for_same_items(const DirectoryItemArray* existing_items,
                                  DirectoryItemArray* reloaded_items)
{
    u32 count = 0;
    for (u32 i = 0; i < reloaded_items->size; ++i)
    {
        DirectoryItem* reloaded_item = reloaded_items->data + i;
        for (u32 j = 0; j < existing_items->size; ++j)
        {
            DirectoryItem* existing_item = existing_items->data + j;
            if (guid_compare(reloaded_item->id, existing_item->id) == 0)
            {
                reloaded_item->before_animation =
                    existing_item->before_animation;
                reloaded_item->after_animation = existing_item->after_animation;
                reloaded_item->animation_precent =
                    existing_item->animation_precent;
                reloaded_item->animation_on = existing_item->animation_on;
                reloaded_item->rename = existing_item->rename;
                ++count;
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
    Directory reloaded_directory = platform_get_directory(path, length);
    path[length - 2] = '\0';
    path[length - 1] = '\0';

    look_for_same_items(&directory_page->directory.sub_directories,
                        &reloaded_directory.sub_directories);
    look_for_same_items(&directory_page->directory.files,
                        &reloaded_directory.files);

    platform_reset_directory(&directory_page->directory);
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
        platform_paste_to_directory(&pasted_paths,
                                    current_directory->directory.parent);
        for (u32 i = 0; i < pasted_paths.size; ++i)
        {
            free(pasted_paths.data[i]);
        }
        directory_reload(current_directory);
    }
    free(pasted_paths.data);
}

internal void merge(DirectoryItem* array, u32 left, u32 mid, u32 right)
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

internal void merge_sort(DirectoryItem* array, u32 left, u32 right)
{
    if (left < right)
    {
        u32 mid = left + (right - left) / 2;
        merge_sort(array, left, mid);
        merge_sort(array, mid + 1, right);
        merge(array, left, mid, right);
    }
}

void directory_sort_by_name(DirectoryItemArray* array)
{
    merge_sort(array->data, 0, array->size - 1);
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

void directory_sort(DirectoryPage* directory_page)
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
        default:
        {
            // directory_sort_by_name(&directory_page->directory.sub_directories);
            // directory_sort_by_name(&directory_page->directory.files);
            break;
        }
    }
}

void directory_history_update_directory_change_handle(
    DirectoryHistory* directory_history)
{
    directory_unlisten_to_directory_changes(directory_history->change_handle);
    directory_history->change_handle = directory_listen_to_directory_changes(
        directory_current(directory_history)->directory.parent);
}

b8 directory_go_to(char* path, u32 length, DirectoryHistory* directory_history)
{
    b8 result = false;
    char saved_chars[3];
    saved_chars[0] = path[length];
    path[length] = '\0';
    if (string_compare_case_insensitive(
            path, directory_current(directory_history)->directory.parent) &&
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
        directory_history_update_directory_change_handle(directory_history);
    }
    path[length] = saved_chars[0];
    return result;
}

void directory_open_folder(char* folder_path,
                           DirectoryHistory* directory_history)
{
    u32 length = (u32)strlen(folder_path);
    directory_go_to(folder_path, length, directory_history);
}

void directory_move_in_history(const i32 index_add,
                               SelectedItemValues* selected_item_values,
                               DirectoryHistory* directory_history)
{
    directory_history->current_index += index_add;
    DirectoryPage* current = directory_current(directory_history);
    current->scroll_offset = 0.0f;
    directory_clear_selected_items(selected_item_values);
    directory_reload(current);
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

DirectoryTab directory_tab_add(const char* dir)
{
    DirectoryHistory directory_history = { 0 };
    array_create(&directory_history.history, 10);

    DirectoryPage page = { 0 };
    page.directory = platform_get_directory(dir, (u32)strlen(dir));
    array_push(&directory_history.history, page);

    directory_history.change_handle =
        directory_listen_to_directory_changes(page.directory.parent);

    SelectedItemValues selected_item_values = { 0 };
    array_create(&selected_item_values.paths, 10);
    selected_item_values.selected_items =
        hash_table_create_guid(100, hash_guid);

    return (DirectoryTab){
        .directory_list = {
            .input = ui_input_buffer_create(),
            .selected_item_values = selected_item_values,
        },
        .directory_history = directory_history,
        .list_view = true,
    };
}

void directory_tab_clear(DirectoryTab* tab)
{
    for (u32 j = 0; j < tab->directory_history.history.size; j++)
    {
        platform_reset_directory(
            &tab->directory_history.history.data[j].directory);
    }
    array_free(&tab->directory_history.history);
    directory_unlisten_to_directory_changes(
        tab->directory_history.change_handle);
    ui_input_buffer_delete(&tab->directory_list.input);
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

void directory_remove_selected_item(SelectedItemValues* selected_item_values,
                                    const FticGUID guid)
{
    CellGuid* cell =
        hash_table_remove_guid(&selected_item_values->selected_items, guid);
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

