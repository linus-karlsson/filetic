#pragma once
#include "define.h"
#include "math/ftic_math.h"
#include "hash_table.h"

typedef struct Vertex
{
    V4 color;
    V2 position;
    V2 texture_coordinates;
    f32 texture_index;
} Vertex;

typedef struct Vertex3D
{
    V4 color;
    V3 position;
    V3 normal;
    V2 texture_coordinates;
    f32 texture_index;
} Vertex3D;

typedef struct VP
{
    M4 projection;
    M4 view;
} VP;

typedef struct MVP
{
    M4 projection;
    M4 view;
    M4 model;
} MVP;

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

typedef struct Vertex3DArray
{
    u32 size;
    u32 capacity;
    Vertex3D* data;
} Vertex3DArray;

typedef struct FileAttrib
{
    u8* buffer;
    u64 size;
    u64 current_pos;
} FileAttrib;

typedef struct Token
{
    char* start;
    u32 buffer_len;
    u32 delim_position;
    char delim_used;
} Token;

typedef struct CharArray
{
    u32 size;
    u32 capacity;
    char* data;
} CharArray;

typedef struct SelectedItemValues
{
    CharPtrArray paths;
    HashTableGuid selected_items;
    char* last_selected;
} SelectedItemValues;

typedef struct Mesh3D
{
    Vertex3DArray vertices;
    IndexArray indices;
} Mesh3D;

typedef struct ObjectThumbnail
{
    FticGUID id;
    Mesh3D mesh;
} ObjectThumbnail;

typedef struct ObjectThumbnailArray
{
    u32 size;
    u32 capacity;
    ObjectThumbnail* data;
} ObjectThumbnailArray;

typedef struct SafeObjectThumbnailArray
{
    ObjectThumbnailArray array;
    FTicMutex mutex;
} SafeObjectThumbnailArray;

typedef struct ObjectThumbnailData
{
    FticGUID file_id;
    char* file_path;
    SafeObjectThumbnailArray* array;
    i32 size;
} ObjectThumbnailData;

FileAttrib file_read(const char* file_path);
void file_write(const char* file_path, const char* content, u32 size);
b8 file_end_of_file(const FileAttrib* file);
void file_buffer_read(FileAttrib* file, char* delims, u32 delim_len, b8 remove_character, CharArray* buffer);
void file_line_read(FileAttrib* file, b8 remove_newline, CharArray* line);
const char* file_get_extension(const char* path, const u32 path_length);

Token file_read_token(CharArray* line, const char* delims, u32 delims_len);

void file_format_size(u64 size_in_bytes, char* output, size_t output_size);

f32 lerp_f32(const f32 a, const f32 b, const f32 t);
V4 v4_lerp(V4 v1, V4 v2, f32 t);
V2 v2_lerp(V2 v1, V2 v2, f32 t);
#define string_copy_d(string) string_copy(string, (u32)strlen(string), 0)
char* string_copy(const char* string, const u32 string_length, const u32 extra_length);
i32 string_compare_case_insensitive(const char* first, const char* second);
u32 string_span_case_insensitive(const char* first, const char* second);
b8 string_contains_case_insensitive(const char* string, const char* value);
b8 string_contains(const char* string, const u32 string_length, const char* value, const u32 value_length);
void string_swap(char* first, char* second);
f32 clampf32_low(f32 value, f32 low);
f32 clampf32_high(f32 value, f32 high);

f64 clampf64_low(f64 value, f64 low);
f64 clampf64_high(f64 value, f64 high);

f32 middle(const f32 area_size, const f32 object_size);

void log_f32(const char* message, const f32 value);
void log_u64(const char* message, const u64 value);

f32 ease_out_elastic(const f32 x);
f32 ease_out_sine(const f32 x);
f32 ease_out_cubic(const f32 x);

u32 get_path_length(const char* path, u32 path_length);

f32 radians(f32 deg);
f32 abs_f32(f32 in);
f32 round_f32(f32 value);

void mesh_3d_load(Mesh3D* mesh, const char* object_path);
void object_load_thumbnail(void* data);

