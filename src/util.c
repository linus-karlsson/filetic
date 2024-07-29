#include "util.h"
#include "logging.h"
#include "object_load.h"
#include "platform/platform.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

b8 file_end_of_file(const FileAttrib* file)
{
    return file->current_pos >= file->size;
}

internal b8 is_delim(char character, char* delims, u32 delim_len)
{
    for (u32 i = 0; i < delim_len; i++)
    {
        if (delims[i] == character)
        {
            return true;
        }
    }
    return false;
}

void file_buffer_read(FileAttrib* file, char* delims, u32 delim_len,
                      b8 remove_character, CharArray* buffer)
{
    u32 count = 0;
    while (!file_end_of_file(file) &&
           !is_delim(file->buffer[file->current_pos], delims, delim_len))
    {
        array_push(buffer, file->buffer[file->current_pos++]);
    }
    if (is_delim(file->buffer[file->current_pos], delims, delim_len))
    {
        if (!remove_character)
        {
            array_push(buffer, file->buffer[file->current_pos]);
        }
        file->current_pos++;
    }
    array_push(buffer, '\0');
    buffer->size--;
}

void file_line_read(FileAttrib* file, b8 remove_newline, CharArray* line)
{
    file_buffer_read(file, "\n", 1, remove_newline, line);
}

FileAttrib file_read_full_path(const char* file_path)
{
    FILE* file = fopen(file_path, "rb");

    FileAttrib file_attrib = { 0 };
    if (!file)
    {
        log_file_error(file_path);
        return file_attrib;
    }

    fseek(file, 0, SEEK_END);
    file_attrib.size = (u64)ftell(file);
    rewind(file);

    file_attrib.buffer = (u8*)calloc(file_attrib.size + 1, sizeof(u8));

    if (fread(file_attrib.buffer, 1, file_attrib.size, file) !=
        file_attrib.size)
    {
        log_last_error();
        free(file_attrib.buffer);
        file_attrib = (FileAttrib){ 0 };
        fclose(file);
    }
    fclose(file);
    return file_attrib;

}

FileAttrib file_read(const char* file_path)
{
    char full_path_buffer[FTIC_MAX_PATH] = {0};
    append_full_path(file_path, full_path_buffer);
    return file_read_full_path(full_path_buffer);
}

void file_write(const char* file_path, const char* content, u32 size)
{
    char full_path_buffer[FTIC_MAX_PATH] = {0};
    append_full_path(file_path, full_path_buffer);
    FILE* file = fopen(full_path_buffer, "w");

    if (file == NULL)
    {
        log_file_error(file_path);
    }
    fwrite(content, 1, (size_t)size, file);
    fclose(file);
}

const char* file_get_extension(const char* path, const u32 path_length)
{
    for (i32 i = (i32)path_length - 1; i >= 0; --i)
    {
        char current_char = path[i];
        if (current_char == '\\' || current_char == '/')
        {
            return NULL;
        }
        else if (current_char == '.')
        {
            return path + i + (i != ((i32)path_length - 1));
        }
    }
    return NULL;
}

Token file_read_token(CharArray* buffer, const char* delims, u32 delims_len)
{
    Token result = { 0 };
    if (buffer->size == 0)
    {
        return result;
    }
    for (u32 i = 0; i < buffer->size; i++)
    {
        if (buffer->data[i] == '\0')
        {
            break;
        }
        for (u32 j = 0; j < delims_len; j++)
        {
            if (buffer->data[i] == delims[j])
            {
                buffer->data[i] = '\0';

                result.start = buffer->data;
                // TODO:
                result.buffer_len = buffer->size;
                result.delim_position = i;
                result.delim_used = delims[j];
                return result;
            }
        }
    }
    return result;
}

void file_format_size(u64 size_in_bytes, char* output, size_t output_size)
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

void file_rename(const char* full_path, const char* new_name, const u32 name_length)
{
    u32 path_length = get_path_length(full_path, (u32)strlen(full_path));

    char* to = string_copy(full_path, path_length, name_length + 2);
    memcpy(to + path_length, new_name, name_length);

    rename(full_path, to);

    free(to);
}

f32 lerp_f32(const f32 a, const f32 b, const f32 t)
{
    return a + (t * (b - a));
}

V4 v4_lerp(V4 v1, V4 v2, f32 t)
{
    return v4_add(v1, v4_s_multi(v4_sub(v2, v1), t));
}
V2 v2_lerp(V2 v1, V2 v2, f32 t)
{
    return v2_add(v1, v2_s_multi(v2_sub(v2, v1), t));
}

char* string_copy(const char* string, const u32 string_length,
                  const u32 extra_length)
{
    char* result =
        (char*)calloc(string_length + extra_length + 1, sizeof(char));
    memcpy(result, string, string_length);
    return result;
}

i32 string_compare_case_insensitive(const char* first, const char* second)
{
    while (*first && *second)
    {
        i32 value =
            tolower((unsigned char)*first) - tolower((unsigned char)*second);
        if (value != 0)
        {
            return value;
        }
        first++;
        second++;
    }
    return tolower((unsigned char)*first) - tolower((unsigned char)*second);
}

u32 string_span_case_insensitive(const char* first, const char* second)
{
    u32 count = 0;
    const char* p;

    while (*first)
    {
        for (p = second; *p; p++)
        {
            if (tolower((unsigned char)*first) == tolower((unsigned char)*p))
            {
                break;
            }
        }
        if (*p == '\0')
        {
            break;
        }
        count++;
        first++;
    }

    return count;
}

b8 string_contains(const char* string, const u32 string_length,
                   const char* value, const u32 value_length)
{
    const char* begining = value;
    for (u32 i = 0, j = 0; i < string_length; ++i)
    {
        j = string[i] == value[j] ? j + 1 : 0;
        if (j == value_length) return true;
    }
    return false;
}

b8 string_contains_case_insensitive(const char* string, const char* value)
{
    const char* begining = value;
    while (*string && *value)
    {
        value =
            tolower((unsigned char)*string) == tolower((unsigned char)*value)
                ? value + 1
                : begining;
        string++;
    }
    return !(*value);
}

void string_swap(char* first, char* second)
{
    char temp[4] = { 0 };
    const size_t length = sizeof(temp);
    memcpy(temp, first, length);
    memcpy(first, second, length);
    memcpy(second, temp, length);
}

f32 middle(const f32 area_size, const f32 object_size)
{
    return (area_size * 0.5f) - (object_size * 0.5f);
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

f32 ease_out_sine(const f32 x)
{
    return sinf((x * PI) / 2);
}

f32 ease_out_cubic(const f32 x)
{
    return 1 - powf(1 - x, 3);
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

f32 radians(f32 deg)
{
    return (f32)((deg * PI) / 180.0f);
}

f32 abs_f32(f32 in)
{
    return in < 0.0f ? in * -1.0f : in;
}

f32 round_f32(f32 value)
{
    return (f32)((int)(value + (0.5f - (f32)(value < 0.0f))));
}

V2 round_v2(V2 v2)
{
    return v2f(round_f32(v2.x), round_f32(v2.y));
}

internal void mesh_3d_aabb_check_min_max(const V3 vertex_position, AABB3D* aabb,
                                         V3* current_max)
{
    if (vertex_position.x < aabb->min.x)
    {
        aabb->min.x = vertex_position.x;
    }
    if (vertex_position.x > current_max->x)
    {
        current_max->x = vertex_position.x;
    }
    if (vertex_position.y < aabb->min.y)
    {
        aabb->min.y = vertex_position.y;
    }
    if (vertex_position.y > current_max->y)
    {
        current_max->y = vertex_position.y;
    }
    if (vertex_position.z < aabb->min.z)
    {
        aabb->min.z = vertex_position.z;
    }
    if (vertex_position.z > current_max->z)
    {
        current_max->z = vertex_position.z;
    }
}

AABB3D mesh_3d_load(Mesh3D* mesh, const char* object_path,
                    const f32 texture_index)
{
    ObjectLoad object_load = { 0 };
    object_load_model(&object_load, object_path);

    AABB3D result = { .min = v3i(INFINITY) };
    V3 max = v3i(-INFINITY);

    array_create(&mesh->vertices, object_load.indices.size);
    array_create(&mesh->indices, object_load.indices.size);
    for (u32 i = 0; i < object_load.indices.size; ++i)
    {
        Vertex3D vertex = { 0 };
        const u32 current_vert_index = object_load.indices.data[i].vertex_index;
        vertex.position = object_load.vertex_positions.data[current_vert_index];
        vertex.color = v4f(1.0f, 1.0f, 1.0f, 1.0f);

        vertex.normal =
            object_load.normals.data[object_load.indices.data[i].normal_index];

        const u32 current_tex_index = object_load.indices.data[i].texture_index;
        vertex.texture_coordinates.x =
            object_load.texture_coordinates.data[current_tex_index].x;
        vertex.texture_coordinates.y =
            1.0f - object_load.texture_coordinates.data[current_tex_index].y;

        vertex.texture_index = texture_index;

        mesh_3d_aabb_check_min_max(vertex.position, &result, &max);

        array_push(&mesh->vertices, vertex);
        array_push(&mesh->indices, i);
    }
    object_load_free(&object_load);

    result.size = v3_sub(max, result.min);
    return result;
}

void object_load_thumbnail(void* data)
{
    ObjectThumbnailData* arguments = (ObjectThumbnailData*)data;

    ObjectThumbnail thumbnail = { .id = guid_copy(&arguments->file_id) };
    thumbnail.mesh_aabb =
        mesh_3d_load(&thumbnail.mesh, arguments->file_path, 0.0f);

    platform_mutex_lock(&arguments->array->mutex);
    if (arguments->array->array.data == NULL)
    {
        array_free(&thumbnail.mesh.vertices);
        array_free(&thumbnail.mesh.indices);
        free(data);
        return;
    }
    array_push(&arguments->array->array, thumbnail);
    platform_mutex_unlock(&arguments->array->mutex);
    free(data);
}

u32 append_full_path(const char* path, char* destination)
{
    const char* executable_dir = platform_get_executable_directory();
    const u32 length = platform_get_executable_directory_length();
    memcpy(destination, executable_dir, length);
    const u32 path_length = (u32)strlen(path);
    memcpy(destination + length, path, path_length);
    destination[length + path_length] = '\0';
    return length + path_length;
}
