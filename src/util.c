#include "util.h"
#include "logging.h"
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

FileAttrib file_read(const char* file_path)
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

void file_write(const char* file_path, const char* content, u32 size)
{
    FILE* file = fopen(file_path, "w");

    if (file == NULL)
    {
        log_file_error(file_path);
    }
    fwrite(content, 1, (size_t)size, file);
    fclose(file);
}

const char* file_get_extension(const char* path, const u32 path_length)
{
    for (i32 i = path_length - 1; i >= 0; --i)
    {
        char current_char = path[i];
        if (current_char == '\\' || current_char == '/')
        {
            return NULL;
        }
        else if (current_char == '.')
        {
            return path + i;
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

f32 clampf32_low(f32 value, f32 low)
{
    return value < low ? low : value;
}

f32 clampf32_high(f32 value, f32 high)
{
    return value > high ? high : value;
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
