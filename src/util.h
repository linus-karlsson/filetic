#pragma once
#include "define.h"
#include "math/ftic_math.h"

typedef struct Vertex
{
    V4 color;
    V2 position;
    V2 texture_coordinates;
    f32 texture_index;
} Vertex;

typedef struct IndexArray
{
    u32 size;
    u32 capacity;
    u32* data;
} IndexArray, U32Array;


typedef struct VertexArray
{
    u32 size;
    u32 capacity;
    Vertex* data;
} VertexArray;

typedef struct FileAttrib
{
    u8* buffer;
    u64 size;
    u64 current_pos;
} FileAttrib;

typedef struct CharArray
{
    u32 size;
    u32 capacity;
    char* data;
} CharArray;

FileAttrib file_read(const char* file_path);
void file_write(const char* file_path, const char* content, u32 size);
b8 file_end_of_file(const FileAttrib* file);
void file_buffer_read(FileAttrib* file, char* delims, u32 delim_len, b8 remove_character, CharArray* buffer);
void file_line_read(FileAttrib* file, b8 remove_newline, CharArray* line);
f32 lerp_f32(const f32 a, const f32 b, const f32 t);
V4 v4_lerp(V4 v1, V4 v2, f32 t);
char* string_copy(const char* string, const u32 string_length, const u32 extra_length);
i32 string_compare_case_insensitive(const char* first, const char* second);
u32 string_span_case_insensitive(const char* first, const char* second);
b8 string_contains_case_insensitive(const char* string, const char* value);
b8 string_contains(const char* string, const u32 string_length, const char* value, const u32 value_length);
f32 clampf32_low(f32 value, f32 low);
f32 clampf32_high(f32 value, f32 high);

f32 middle(const f32 area_size, const f32 object_size);
