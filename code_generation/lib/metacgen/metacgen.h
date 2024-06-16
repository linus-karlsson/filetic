#pragma once
#include <stdint.h>

/*
 * NOTE: This is my own library.
 * It is very porly written considering no thought was put into it 
 * and I wrote it in a weekend. But it gets the job done.
 */

typedef void Mcgen_Context;

#define mcgen_append_type(ctx, name, ...)                                      \
    do                                                                         \
    {                                                                          \
        char* TYPES_48793_NMAKSIELHSLDULA[] = { __VA_ARGS__ };                 \
        mcgen_append_type_(ctx, name, TYPES_48793_NMAKSIELHSLDULA,             \
                           sizeof(TYPES_48793_NMAKSIELHSLDULA) /               \
                               sizeof(TYPES_48793_NMAKSIELHSLDULA[0]),         \
                           __FILE__, __LINE__);                                \
    } while (0)

#define mcgen_append_types(ctx, name, types, size)                             \
    mcgen_append_type_(ctx, name, types, size, __FILE__, __LINE__);

#define mcgen_append_types_and_postfixs(ctx, name, types, size, pfix, psize)   \
    mcgen_append_type_(ctx, name, types, size, __FILE__, __LINE__);            \
    mcgen_set_postfixs(ctx, name, types, size, pfix, psize);                

#define mcgen_link_names(ctx, name, ...)                                       \
    do                                                                         \
    {                                                                          \
        char* NAMES_48793_NMAKSIELHSLDULA[] = { __VA_ARGS__ };                 \
        mcgen_link_name_(ctx, name, NAMES_48793_NMAKSIELHSLDULA,               \
                           sizeof(NAMES_48793_NMAKSIELHSLDULA) /               \
                               sizeof(NAMES_48793_NMAKSIELHSLDULA[0]));        \
    } while (0)

Mcgen_Context* mcgen_context_create(const char* const working_directory);
void mcgen_context_destroy(Mcgen_Context* const ctx);
void mcgen_change_working_directory(Mcgen_Context* const ctx, const char* const working_directory);
void mcgen_append_type_files(Mcgen_Context* const ctx, const char* const generation_file, const char* const output_definition_file, const char* const output_implementation_file);
void mcgen_append_type_file(Mcgen_Context* const ctx, const char* const generation_file, const char* const output_file);
void mcgen_append_file(Mcgen_Context* const ctx, const char* const generation_file, const char* const output_file);
void mcgen_append_type_(Mcgen_Context* const ctx, const char* const name, char** const types, const uint32_t type_count, const char* const file_name, const uint32_t line_number);
void mcgen_set_postfix(Mcgen_Context* const ctx, const char* const name, const char* const type, const char* const postfix);
void mcgen_set_postfixs(Mcgen_Context* const ctx, const char* const name, const char** const types, const uint32_t type_count, const char** const postfixs, const uint32_t postfix_count);
void mcgen_link_name_(Mcgen_Context* const ctx, const char* const name, const char** const names, const uint32_t name_count);
void mcgen_generate_code(Mcgen_Context* const ctx);

#ifdef METACGEN_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#define cmp(v1, v2) memcmp(*v1, *v2, sizeof(*v1))

#define mcgen_calloc(type, count) (type*)calloc(count, sizeof(type))

#define KILOBYTE(n) ((n) * 1024ULL)
#define MEGABYTE(n) (KILOBYTE((n)) * 1024ULL)
#define GIGABYTE(n) (MEGABYTE((n)) * 1024ULL)

#define MILLISECONDS(milli) ((milli) * 0.001);
#define MICROSECONDS(micro) ((micro) * 0.000001);
#define NANOSECONDS(nano) ((nano) * 0.000000001);

#define PI 3.1415936f
#define U8_MAX 0xFF
#define U16_MAX 0xFFFF
#define U32_MAX 0xFFFFFFFF
#define U64_MAX 0xFFFFFFFFFFFFFFFF

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define sy(...) __VA_ARGS__

#define sy_RGB(v) ((v) / 255.0f)

#define set_bit(val, bit) (val) |= (bit)
#define unset_bit(val, bit) (val) &= ~(bit)
#define switch_bit(val, bit) (val) ^= (bit)
#define check_bit(val, bit) (((val) & (bit)) == (bit))

#define BIT_64 0x8000000000000000
#define BIT_63 0x4000000000000000
#define BIT_62 0x2000000000000000
#define BIT_61 0x1000000000000000
#define BIT_60 0x800000000000000
#define BIT_59 0x400000000000000
#define BIT_58 0x200000000000000
#define BIT_57 0x100000000000000
#define BIT_56 0x80000000000000
#define BIT_55 0x40000000000000
#define BIT_54 0x20000000000000
#define BIT_53 0x10000000000000
#define BIT_52 0x8000000000000
#define BIT_51 0x4000000000000
#define BIT_50 0x2000000000000
#define BIT_49 0x1000000000000
#define BIT_48 0x800000000000
#define BIT_47 0x400000000000
#define BIT_46 0x200000000000
#define BIT_45 0x100000000000
#define BIT_44 0x80000000000
#define BIT_43 0x40000000000
#define BIT_42 0x20000000000
#define BIT_41 0x10000000000
#define BIT_40 0x8000000000
#define BIT_39 0x4000000000
#define BIT_38 0x2000000000
#define BIT_37 0x1000000000
#define BIT_36 0x800000000
#define BIT_35 0x400000000
#define BIT_34 0x200000000
#define BIT_33 0x100000000
#define BIT_32 0x80000000
#define BIT_31 0x40000000
#define BIT_30 0x20000000
#define BIT_29 0x10000000
#define BIT_28 0x8000000
#define BIT_27 0x4000000
#define BIT_26 0x2000000
#define BIT_25 0x1000000
#define BIT_24 0x800000
#define BIT_23 0x400000
#define BIT_22 0x200000
#define BIT_21 0x100000
#define BIT_20 0x80000
#define BIT_19 0x40000
#define BIT_18 0x20000
#define BIT_17 0x10000
#define BIT_16 0x8000
#define BIT_15 0x4000
#define BIT_14 0x2000
#define BIT_13 0x1000
#define BIT_12 0x800
#define BIT_11 0x400
#define BIT_10 0x200
#define BIT_9 0x100
#define BIT_8 0x80
#define BIT_7 0x40
#define BIT_6 0x20
#define BIT_5 0x10
#define BIT_4 0x8
#define BIT_3 0x4
#define BIT_2 0x2
#define BIT_1 0x1

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;

typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef int64_t b64;
typedef int32_t b32;
typedef int16_t b16;
typedef int8_t b8;

typedef double f64;
typedef float f32;

#define global static
#define internal static
#define presist static

typedef struct File_Attrib
{
    u32 size;
    char* buffer;
} File_Attrib;

typedef struct Line
{
    u32 size;
    char* buffer;
} Line, Scope, Statement;

typedef struct Tokens
{
    u32 count;
    u32* offsets;
    u32* sizes;
    char** buffer;
} Tokens;

typedef struct Generation_Output_File
{
    const char* generation;
    const char* output_definition;
    const char* output_implementation;
} Generation_Output_File;

typedef struct Char_Position
{
    u32 position;
    char value;
} Char_Position;

typedef struct Name_Type
{
    char* name;
    char* type;
    // For error messages
    const char* file_name;
    u32 line_number;
} Name_Type;

#define array_create_d(array) array_create(array, 10)
#define array_create(array, array_capacity)                                    \
    do                                                                         \
    {                                                                          \
        (array)->size = 0;                                                     \
        (array)->capacity = (array_capacity);                                  \
        (array)->data = calloc((array_capacity), sizeof((*(array)->data)));    \
    } while (0)

#define array_append(array, value)                                             \
    do                                                                         \
    {                                                                          \
        if ((array)->size >= (array)->capacity)                                \
        {                                                                      \
            (array)->capacity *= 2;                                            \
            (array)->data = realloc(                                           \
                (array)->data, (array)->capacity * sizeof((*(array)->data)));  \
        }                                                                      \
        (array)->data[(array)->size++] = (value);                              \
    } while (0)

#define array_pop(array, result)                                               \
    do                                                                         \
    {                                                                          \
        if ((array)->size > 0)                                                 \
        {                                                                      \
            (array)->size--;                                                   \
            *(result) = true;                                                  \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            *(result) = false;                                                 \
        }                                                                      \
    } while (0)


typedef struct Array_Char_Position
{ 
    u32 size; 
    u32 capacity; 
    Char_Position* data; 
} Array_Char_Position;

typedef struct Array_Name_Type
{ 
    u32 size; 
    u32 capacity; 
    Name_Type* data; 
} Array_Name_Type;

typedef struct Array_Gen_Output
{ 
    u32 size; 
    u32 capacity; 
    Generation_Output_File* data; 
} Array_Gen_Output;

typedef struct Array_Char
{ 
    u32 size; 
    u32 capacity; 
    char* data; 
} Array_Char;

typedef struct Array_Char_Ptr
{ 
    u32 size; 
    u32 capacity; 
    char** data; 
} Array_Char_Ptr;

u64 hash_murmur(const void* key, u32 len, u64 seed);
u64 hash_djb2(const void* key, u32 len, u64 seed);

void hash_table_set_seed(u64 seed);

typedef struct Cell_Cc
{ 
    char* key; 
    char* value; 
    bool active; 
    bool deleted; 
} Cell_Cc;

typedef struct Cell_Ca
{ 
    char* key; 
    Array_Char value; 
    bool active; 
    bool deleted; 
} Cell_Ca;

typedef struct Cell_Cap
{ 
    char* key; 
    Array_Char_Ptr value; 
    bool active; 
    bool deleted; 
} Cell_Cap;

typedef struct Hash_Table_Cc
{ 
    Cell_Cc* cells; 
    u32 size; 
    u32 capacity; 
 
    u64 (*hash_function)(const void* key, u32 len, u64 seed); 
} Hash_Table_Cc;

typedef struct Hash_Table_Ca
{ 
    Cell_Ca* cells; 
    u32 size; 
    u32 capacity; 
 
    u64 (*hash_function)(const void* key, u32 len, u64 seed); 
} Hash_Table_Ca;

typedef struct Hash_Table_Cap
{ 
    Cell_Cap* cells; 
    u32 size; 
    u32 capacity; 
 
    u64 (*hash_function)(const void* key, u32 len, u64 seed); 
} Hash_Table_Cap;

Hash_Table_Cc hash_table_create_cc(u32 capacity, u64 (*hash_function)(const void* key, u32 len, u64 seed));

Hash_Table_Ca hash_table_create_ca(u32 capacity, u64 (*hash_function)(const void* key, u32 len, u64 seed));

Hash_Table_Cap hash_table_create_cap(u32 capacity, u64 (*hash_function)(const void* key, u32 len, u64 seed));

void hash_table_insert_cc(Hash_Table_Cc* table, char* key, char* value);

void hash_table_insert_ca(Hash_Table_Ca* table, char* key, Array_Char value);

void hash_table_insert_cap(Hash_Table_Cap* table, char* key, Array_Char_Ptr value);

char** hash_table_get_cc(Hash_Table_Cc* table, const char* key);

Array_Char* hash_table_get_ca(Hash_Table_Ca* table, const char* key);

Array_Char_Ptr* hash_table_get_cap(Hash_Table_Cap* table, const char* key);

Cell_Cc* hash_table_remove_cc(Hash_Table_Cc* table, const char* key);

Cell_Ca* hash_table_remove_ca(Hash_Table_Ca* table, const char* key);

Cell_Cap* hash_table_remove_cap(Hash_Table_Cap* table, const char* key);


typedef struct Mcgen_Context_Internal
{
    char* working_directory;
    u32 working_directory_len;
    Array_Gen_Output types;
    Array_Gen_Output files;
    Array_Name_Type name_type;
    Hash_Table_Cc name_postfix;
    Hash_Table_Cap linked_names;
} Mcgen_Context_Internal;

bool check_valid_chars(char current)
{
    switch (current)
    {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        {
            return true;
        }
        default: return false;
    }
}

#define NULL_TERMINATOR 1

internal void array_append_string(Array_Char* const array,
                                  const char* const string, const u32 length)
{
    if (array->size + length >= array->capacity)
    {
        array->capacity *= 2;
        array->capacity = max(array->size + length + 10, array->capacity);
        array->data = realloc(array->data, array->capacity * sizeof(char));
    }
    memcpy(array->data + array->size, string, length);
    array->size += length;
}

internal char* allocate_and_copy_string(const char* const src,
                                        const u32 src_length, const u32 offset,
                                        const u32 padding)
{
    char* const result =
        mcgen_calloc(char, src_length + padding + NULL_TERMINATOR);
    memcpy(result + offset, src, src_length);
    return result;
}

internal u32 trim(char* const value, u32 size)
{
    u32 count = 0;
    for (u32 i = 0; i < size; ++i)
    {
        if (value[i] == ' ' || value[i] == '\t')
        {
            ++count;
        }
        else
        {
            break;
        }
    }
    if (count)
    {
        for (u32 i = 0; i < size - count; ++i)
        {
            value[i] = value[i + count];
            value[i + count] = ' ';
        }
    }
    for (; size > 0 && (value[size - 1] == ' ' || value[size - 1] == '\t');
         --size)
        ;
    value[size] = '\0';
    return size;
}

internal char* get_full_path(const Mcgen_Context_Internal* const ctx,
                             const char* const file_path)
{
    const u32 file_path_len = (u32)strlen(file_path);
    char* const result = mcgen_calloc(
        char, file_path_len + ctx->working_directory_len + NULL_TERMINATOR);

    memcpy(result, ctx->working_directory, ctx->working_directory_len);
    memcpy(result + ctx->working_directory_len, file_path, file_path_len);
    return result;
}

internal File_Attrib file_read(const char* const file_path)
{
    FILE* const file = fopen(file_path, "rb");

    File_Attrib file_attrib = { 0 };

    if (!file)
    {
        return file_attrib;
    }

    fseek(file, 0, SEEK_END);
    file_attrib.size = ftell(file);
    rewind(file);

    file_attrib.buffer = mcgen_calloc(char, file_attrib.size);

    if (fread(file_attrib.buffer, sizeof(char), file_attrib.size, file) !=
        file_attrib.size)
    {
        fprintf(stderr, "Failed reading file\n");
        exit(1);
    }
    fclose(file);
    return file_attrib;
}

internal void file_write(const char* const file_path, const char* const content,
                         const u32 size)
{
    FILE* const file = fopen(file_path, "wb");

    if (!file)
    {
        fprintf(stderr, "Failed to open file\n");
        exit(1);
    }

    fwrite(content, 1, (size_t)size, file);
    fclose(file);
}

internal bool end_of_file(const File_Attrib* const file, const u32 offset)
{
    return offset >= file->size;
}

internal Line read_to_character(const File_Attrib* const file,
                                u32* const offset_out,
                                const char* const ending_characters,
                                const u32 ending_character_count,
                                const bool remove_ending_character)
{
    const u32 offset = *offset_out;
    u32 size = offset;
    while (!end_of_file(file, size))
    {
        for (u32 i = 0; i < ending_character_count; ++i)
        {
            if (file->buffer[size] == ending_characters[i])
            {
                ++size;
                goto found_ending;
            }
        }
        ++size;
    }
found_ending:
    *offset_out = size;
    size -= remove_ending_character;

    Line result = { .size = size - offset };
    if (result.size == 0) return result;
    result.buffer = mcgen_calloc(char, result.size + NULL_TERMINATOR);

    u32 j = 0;
    u32 i = offset;
    for (; i < size; ++i, ++j)
    {
        result.buffer[j] = file->buffer[i];
    }

    const int index = j - (1 + !remove_ending_character);
    if (index > 0)
    {
        if (result.buffer[index] == '\r')
        {
            result.buffer[index] = '\n';
            j = index + !remove_ending_character;
        }
    }
    result.buffer[j] = '\0';
    result.size = j;
    return result;
}

internal Line line_read(const File_Attrib* const file, u32* const offset_out,
                        const bool remove_trailing_new_line)
{
    return read_to_character(file, offset_out, "\n", 1,
                             remove_trailing_new_line);
}

internal Statement statement_read(const File_Attrib* const file,
                                  u32* const offset_out)
{
    return read_to_character(file, offset_out, ";{", 2, false);
}

// Only gets called when { is found.
internal Scope scope_read(const File_Attrib* const file, u32* const offset_out,
                          u32* const line_skipped)
{
    const u32 offset = *offset_out;
    u32 line_skipped_in = 0;

    Array_Char_Position stack = { 0 };
    array_create_d(&stack);
    Char_Position c_p = { offset - 1, '{' };
    array_append(&stack, c_p);

    Scope result = { 0 };
    u32 size = offset;
    while (true)
    {
        if (end_of_file(file, size)) return result;
        if (file->buffer[size] == '}')
        {
            bool popped = false;
            array_pop(&stack, &popped);
            if (!popped) return result;
            if (stack.size == 0) break;
        }
        else if (file->buffer[size] == '{')
        {
            c_p.position = size - 1;
            c_p.value = '{';
            array_append(&stack, c_p);
        }
        else if (file->buffer[size] == '\n')
        {
            ++line_skipped_in;
        }
        ++size;
    }
    if (line_skipped)
    {
        *line_skipped = line_skipped_in;
    }
    if (stack.size) return result;
    *offset_out = size;

    result.size = size - offset;
    if (result.size == 0) return result;
    result.buffer = mcgen_calloc(char, result.size + NULL_TERMINATOR);

    u32 j = 0;
    u32 i = offset;
    for (; i < size; ++i, ++j)
    {
        result.buffer[j] = file->buffer[i];
        // TODO: this is fine
        if (result.buffer[j] == '\r') result.buffer[j] = ' ';
    }
    return result;
}

internal char* get_first_token(const char* const string,
                               const u32 string_length,
                               const char* const delims, const u32 delim_count,
                               u32* const offset_out)
{
    u32 offset = 0;
    for (u32 i = 0; i < string_length; ++i)
    {
        for (u32 j = 0; j < delim_count; ++j)
        {
            if (string[i] == delims[j])
            {
                offset = i + 1;
                goto found_offset;
            }
        }
    }
found_offset:
    if (offset < 2) return NULL;
    const u32 len = offset - 1;
    char* token = mcgen_calloc(char, len + NULL_TERMINATOR);
    for (u32 i = 0; i < len; ++i)
    {
        token[i] = string[i];
    }
    token[len] = '\0';
    if (offset_out) *offset_out = offset;
    return token;
}

internal Tokens tokens_read(const char* const string, const u32 string_length,
                            const char* const delims, const u32 delim_count)
{
    u32 token_count = 0;
    char* token_buffer = mcgen_calloc(char, string_length);
    for (u32 i = 0, count = 1; i < string_length; ++i, ++count)
    {
        token_buffer[i] = string[i];
        for (u32 j = 0; j < delim_count; ++j)
        {
            if (string[i] == delims[j])
            {
                token_buffer[i] = '\0';
                token_count += (count > 1);
                count = 0;
                break;
            }
        }
    }
    Tokens result = { .count = token_count };
    if (token_count == 0) return result;

    result.offsets = mcgen_calloc(u32, token_count);

    result.sizes = mcgen_calloc(u32, token_count);
    result.buffer = mcgen_calloc(char*, token_count);
    for (u32 i = 0, token = 0, count = 1, offset = 0; i < string_length;
         ++i, ++count)
    {
        if (token_buffer[i] == '\0')
        {
            if (count > 1)
            {
                result.offsets[token] = offset;
                result.sizes[token] = count;
                result.buffer[token++] = mcgen_calloc(char, count);
                if (token == token_count) break;
                offset = i + NULL_TERMINATOR;
            }
            else
            {
                offset = i + NULL_TERMINATOR;
            }
            count = 0;
        }
    }
    for (u32 i = 0; i < token_count; ++i)
    {
        char* current_token = result.buffer[i];
        const u32 offset = result.offsets[i];
        const u32 size = result.sizes[i];
        for (u32 k = 0; k < size; ++k)
        {
            current_token[k] = token_buffer[offset + k];
        }
        --result.sizes[i];
    }
    return result;
}

internal void tokens_free(const Tokens* const tokens)
{
    free(tokens->sizes);
    free(tokens->offsets);
    for (u32 i = 0; i < tokens->count; ++i)
    {
        free(tokens->buffer[i]);
    }
    free(tokens->buffer);
}

// TODO: remove before
internal u32 remove_spaces_before_and_after_delim(char* const string,
                                                  const u32 string_length,
                                                  const char delim)
{
    u32 new_length = string_length;
    bool found_comma = false;
    for (u32 i = 0; i < new_length; ++i)
    {
        if (found_comma)
        {
            if (string[i] == ' ')
            {
                const u32 last_index = new_length - 1;
                for (u32 j = i; j < last_index; ++j)
                {
                    string[j] = string[j + 1];
                }
                string[last_index] = '\0';
                --new_length;
                --i;
            }
            else
            {
                found_comma = false;
            }
        }
        else if (string[i] == delim)
        {
            found_comma = true;
        }
    }
    return new_length;
}

internal void line_print_error_message(const char* const file,
                                       const u32 line_number,
                                       const char* const error_message,
                                       const Line* const line, const u32 k,
                                       const bool should_exit)
{

    const u32 tab_space = 4;
    u32 message_size = (tab_space * 2) + line->size + (k + 1) + NULL_TERMINATOR;
    char* const message = mcgen_calloc(char, message_size + 1);

    u32 pos = 0;
    for (u32 d = 0; d < tab_space; ++d, ++pos)
    {
        message[pos] = ' ';
    }
    memcpy(message + pos, line->buffer, line->size);
    pos += line->size;
    if (message[pos - 1] != '\n')
    {
        message[pos++] = '\n';
        ++message_size;
    }
    for (u32 d = 0; d < tab_space; ++d, ++pos)
    {
        message[pos] = ' ';
    }
    u32 column_number = 0;
    for (; pos < message_size - 2; ++pos, ++column_number)
    {
        message[pos] = '~';
    }
    message[pos] = '^';
    fprintf(stderr, "%s:%u:%u: \nSyntax error: %s:\n\n%s\n\n", file,
            line_number, column_number + 1, error_message, message);

    free(message);
    if (should_exit) exit(1);
}

internal Tokens templates_read(const char* const file_buffer,
                               const u32 file_size_from_offset)
{
    const char* const delims = ",\n>";
    const u32 delim_count = (u32)strlen(delims);
    u32 size = 0;
    for (u32 i = 0; i < file_size_from_offset; ++i)
    {
        if (file_buffer[i] == '>')
        {
            size = i + 1;
            break;
        }
    }
    return tokens_read(file_buffer, size, delims, delim_count);
}

internal char* generate_postfix_from_type(const Tokens* const types,
                                          const u32 types_total_length,
                                          u32* const length)
{
    const u32 underscore = types->count;
    const u32 max_pointer_count = 4 * types->count;
    const u32 pointer_padding = 4 * max_pointer_count;

    u32 postfix_length = types_total_length + underscore;
    const u32 postfix_byte_size =
        postfix_length + pointer_padding + NULL_TERMINATOR;
    char* postfix = mcgen_calloc(char, postfix_byte_size);

    for (u32 j = 0, index = 0; j < types->count; ++j)
    {
        postfix[index++] = '_';
        memcpy(postfix + index, types->buffer[j], types->sizes[j]);
        postfix[index] &= ~32; // to upper
        index += types->sizes[j];
    }
    // TODO: Probably not necissary
    postfix_length = trim(postfix, postfix_length);
    u32 pointer_count = 0;
    for (u32 k = 2; k < postfix_length && postfix[k]; ++k)
    {
        if (postfix[k] == ' ')
        {
            u32 r = k + 1;
            while (r < postfix_length && postfix[r] == ' ')
            {
                for (u32 d = r; d < postfix_length - 1; ++d)
                {
                    postfix[d] = postfix[d + 1];
                }
                --postfix_length;
            }
            postfix[k] = '_';
            if (postfix[k + 1] != '*') postfix[k + 1] &= ~32;
        }
        else if (postfix[k] == '*')
        {
            const char* const ptr = "_Ptr";
            const u32 ptr_length = (u32)strlen(ptr);
            if (++pointer_count >= max_pointer_count)
            {
                fprintf(stderr, "Error\n");
                exit(1);
            }
            // -1 remove "*"
            const u32 ptr_length_minus_one = ptr_length - 1;
            if (k != postfix_length - 1)
            {
                // TODO: is it valid to not be "*" here?
                if (postfix[k + 1] != '*' && postfix[k + 1] != '_')
                {
                    postfix[k + 1] &= ~32;
                }
                const int iterations = (int)(k + ptr_length);
                for (int d = (int)postfix_length + ptr_length; d >= iterations;
                     --d)
                {
                    postfix[d] = postfix[d - ptr_length_minus_one];
                }
            }
            memcpy(postfix + k, ptr, ptr_length);
            k += ptr_length - 1;
            postfix_length += ptr_length_minus_one;
        }
    }
    postfix[postfix_length] = '\0';
    if (length)
    {
        *length = postfix_length;
    }
    return postfix;
}

internal u32 parse_first_token(Hash_Table_Cc* const struct_definition_table,
                               Hash_Table_Cc* const function_definition_table,
                               Hash_Table_Cc* const templates,
                               Hash_Table_Cc* const outputs,
                               const File_Attrib* const file,
                               const char* const full_path, char* output_path,
                               char* const output_implementation_path,
                               const Line* const first_line, char* first_token,
                               const u32 offset, u32 main_index,
                               u32* const line_count)
{
    const char* const delims = " <\n";
    const u32 delim_count = (u32)strlen(delims);
    if (!strcmp(first_token, "template"))
    {
        Array_Char_Position stack = { 0 };
        array_create_d(&stack);
        for (u32 i = offset; i < first_line->size; ++i)
        {
            if (first_line->buffer[i] == '<')
            {
                const Char_Position char_pos = { i, '<' };
                array_append(&stack, char_pos);
            }
            else if (first_line->buffer[i] == '>')
            {
                bool result = false;
                array_pop(&stack, &result);
                if (!result)
                {
                    line_print_error_message(full_path, *line_count,
                                             "too many \">\"", first_line, i,
                                             true);
                }
            }
        }
        if (stack.size)
        {
            line_print_error_message(full_path, *line_count, "too many \"<\"",
                                     first_line, stack.data[0].position, true);
        }
        int first_offset = -1;
        for (u32 i = offset; i < first_line->size; ++i)
        {
            if (first_line->buffer[i] == '<')
            {
                first_offset = (int)i + 1;
            }
        }
        if (first_offset == -1)
        {
            line_print_error_message(full_path, *line_count,
                                     "missing \"<...>\"", first_line, offset,
                                     true);
        }
        const u32 offest_to_first_template =
            main_index - ((first_line->size - first_offset) + 1);
        const Tokens template_args =
            templates_read(file->buffer + offest_to_first_template,
                           file->size - offest_to_first_template);

        if (!template_args.count)
        {
            line_print_error_message(full_path, *line_count,
                                     "can't have an empty template", first_line,
                                     first_offset - 1, true);
        }
        for (u32 k = 0; k < template_args.count; ++k)
        {
            template_args.sizes[k] =
                trim(template_args.buffer[k], template_args.sizes[k]);
        }

        const u32 main_index_before_line = main_index;

        Line next_line = line_read(file, &main_index, false);
        ++(*line_count);
        next_line.size = trim(next_line.buffer, next_line.size);
        u32 offset_next = 0;
        first_token = get_first_token(next_line.buffer, next_line.size, delims,
                                      delim_count, &offset_next);

        if (strcmp(first_token, "\n") == 0)
        {
            line_print_error_message(full_path, *line_count,
                                     "template needs to be right above",
                                     &next_line, first_offset - 1, true);
        }
        else if (strcmp(first_token, "struct") == 0)
        {
            bool found_scope = false;
            bool found_space = false;
            u32 size_of_name = 0;
            next_line.size = trim(next_line.buffer + offset_next,
                                  next_line.size - offset_next) +
                             offset_next;
            for (u32 k = offset_next; k < next_line.size; ++k)
            {
                char next_char = next_line.buffer[k];
                if (next_char == '{')
                {
                    size_of_name = k - offset_next;
                    // -1 to go forward one, skipping }
                    main_index -= (next_line.size - k) - 1;
                    found_scope = true;
                    break;
                }
                else if (next_char == ' ' || next_char == '\t')
                {
                    found_space = true;
                }
                else if (next_char == '\n')
                {
                    size_of_name = k - offset_next;
                }
                else if (found_space)
                {
                    line_print_error_message(full_path, *line_count,
                                             "Struct name wrong", &next_line, k,
                                             true);
                }
            }
            char* name_of_struct = mcgen_calloc(char, size_of_name);

            for (u32 i = offset_next, j = 0,
                     iterations = offset_next + size_of_name;
                 i < iterations; ++i, ++j)
            {
                name_of_struct[j] = next_line.buffer[i];
            }
            size_of_name = trim(name_of_struct, size_of_name);

            hash_table_insert_cc(outputs, name_of_struct, output_path);

            u32 template_args_size = 0;
            for (u32 i = 0; i < template_args.count; ++i)
            {
                template_args_size += template_args.sizes[i] + 1;
            }
            char* template_args_string = mcgen_calloc(char, template_args_size);
            for (u32 i = 0, j = 0; i < template_args.count; ++i)
            {
                for (u32 k = 0; k < template_args.sizes[i]; ++k, ++j)
                {
                    template_args_string[j] = template_args.buffer[i][k];
                }
                template_args_string[j++] = ';';
            }
            char** check = hash_table_get_cc(templates, name_of_struct);
            if (check)
            {
                line_print_error_message(full_path, *line_count,
                                         "repeated name", &next_line,
                                         offset_next, true);
            }
            hash_table_insert_cc(templates, name_of_struct,
                                 template_args_string);

            if (!found_scope)
            {
                Line checking_line = { 0 };
                for (; main_index < file->size; ++(*line_count))
                {
                    checking_line = line_read(file, &main_index, false);
                    checking_line.size =
                        trim(checking_line.buffer, checking_line.size);
                    for (u32 i = 0; i < checking_line.size; ++i)
                    {
                        if (checking_line.buffer[i] == '{')
                        {
                            main_index -= checking_line.size - i;
                            goto scope_get;
                        }
                        if (!check_valid_chars(checking_line.buffer[i]))
                        {
                            line_print_error_message(full_path, *line_count,
                                                     "wrong syntax",
                                                     &checking_line, i, true);
                        }
                    }
                }
            }
        scope_get:

            u32 line_skipped = 0;
            Scope scope = scope_read(file, &main_index, &line_skipped);
            if (scope.size == 0)
            {
                line_print_error_message(full_path, *line_count,
                                         "scope is wrong", &next_line, 0, true);
            }
            *line_count += line_skipped;

            hash_table_insert_cc(struct_definition_table, name_of_struct,
                                 scope.buffer);
        }
        else
        {
            main_index = main_index_before_line;

            Statement statement = statement_read(file, &main_index);
            statement.size = trim(statement.buffer, statement.size);

            char* name = NULL;
            bool space_found = false;
            u32 name_length = 1;
            for (u32 i = 0; i < statement.size; ++i)
            {
                char next_char = statement.buffer[i];
                if (next_char == ' ' || next_char == '\n' || next_char == '\t')
                {
                    space_found = true;
                }
                else if (next_char == '(')
                {
                    if (name_length)
                    {
                        name = mcgen_calloc(char,
                                            name_length + NULL_TERMINATOR + 1);
                        for (int j = i - 1; j >= 0; --j)
                        {
                            if (statement.buffer[j] == ' ') continue;
                            j -= (name_length - 1);
                            if (j >= 0)
                            {
                                for (u32 k = 0; k < name_length; ++k, ++j)
                                {
                                    name[k] = statement.buffer[j];
                                }
                            }
                            break;
                        }
                    }
                    break;
                }
                else if (space_found)
                {
                    name_length = 1;
                    space_found = false;
                }
                else
                {
                    ++name_length;
                }
            }
            if (name)
            {

                const char* output = output_path;
                if (file->buffer[main_index - 1] != '{')
                {
                    name[name_length++] = ';';
                }

                u32 template_args_size = 0;
                for (u32 i = 0; i < template_args.count; ++i)
                {
                    template_args_size += template_args.sizes[i] + 1;
                }
                char* template_args_string =
                    mcgen_calloc(char, template_args_size);
                for (u32 i = 0, j = 0; i < template_args.count; ++i)
                {
                    for (u32 k = 0; k < template_args.sizes[i]; ++k, ++j)
                    {
                        template_args_string[j] = template_args.buffer[i][k];
                    }
                    template_args_string[j++] = ';';
                }

                if (statement.buffer[statement.size - 1] == '{')
                {
                    if (output_implementation_path)
                    {
                        char* const definition_output_path = output_path;
                        output_path = output_implementation_path;

                        char* const definition_buffer =
                            allocate_and_copy_string(statement.buffer,
                                                     statement.size, 0, 1);
                        for (int i = statement.size - 2; i >= 0; --i)
                        {
                            if (definition_buffer[i] == ')')
                            {
                                definition_buffer[i + 1] = ';';
                                definition_buffer[i + 2] = '\0';
                                break;
                            }
                        }
                        char* const definition_name =
                            allocate_and_copy_string(name, name_length, 0, 1);
                        definition_name[name_length++] = ';';

                        hash_table_insert_cc(outputs, definition_name,
                                             definition_output_path);
                        hash_table_insert_cc(templates, definition_name,
                                             template_args_string);
                        hash_table_insert_cc(function_definition_table,
                                             definition_name,
                                             definition_buffer);
                    }
                    Scope scope = scope_read(file, &main_index, line_count);
                    statement.buffer = realloc(statement.buffer,
                                               statement.size + scope.size + 3);
                    memcpy(statement.buffer + statement.size, scope.buffer,
                           scope.size);
                    statement.size += scope.size;
                    statement.buffer[statement.size++] = '}';
                    // statement.buffer[statement.size++] = '\n';
                    statement.buffer[statement.size++] = '\0';
                    free(scope.buffer);
                }
                char** check = hash_table_get_cc(outputs, name);
                if (check)
                {
                    line_print_error_message(full_path, *line_count,
                                             "repeated name", &next_line,
                                             offset_next, true);
                }
                hash_table_insert_cc(outputs, name, output_path);
                hash_table_insert_cc(templates, name, template_args_string);
                hash_table_insert_cc(function_definition_table, name,
                                     statement.buffer);
            }
            // TODO: this is not correct if the statement ends with a \n
            --(*line_count);
        }
        tokens_free(&template_args);
        free(first_token);
        free(next_line.buffer);
        free(stack.data);
    }
    return main_index;
}

internal char* concatinate(const char* const first, const u32 first_length,
                           const char* const second, const u32 second_length,
                           const char delim_between, u32* const result_length)
{
    const u32 extra = delim_between != '\0';
    const u32 length = first_length + second_length + extra;
    char* result = mcgen_calloc(char, length + NULL_TERMINATOR);

    memcpy(result, first, first_length);
    if (delim_between) result[first_length] = delim_between;
    memcpy(result + first_length + extra, second, second_length);

    result[length] = '\0';
    if (result_length)
    {
        *result_length = length;
    }
    return result;
}

internal void generate_from_name(
    Hash_Table_Cc* const postfix_table, Hash_Table_Cc* const templates,
    Hash_Table_Cc* const struct_table, Hash_Table_Cc* const function_table,
    const bool is_struct, const char* const saved_scope, char* const name,
    const u32 name_length, const Tokens* const types, char* const postfix,
    const u32 postfix_length, Array_Char* const final_buffer)
{
    u32 len = (u32)strlen(saved_scope);
    char* temp = mcgen_calloc(char, len + 128);
    memcpy(temp, saved_scope, len);

    const char** ta0 = hash_table_get_cc(templates, name);
    if (!ta0) return;
    const char* ta = *ta0;

    const Tokens tokens = tokens_read(ta, (u32)strlen(ta), ";", 1);

    const char* const delims = " <>,;*()[]\n\r\t";
    const u32 delim_count = (u32)strlen(delims);
    const Tokens t =
        tokens_read(saved_scope, (u32)strlen(saved_scope), delims, delim_count);

    u32 offset_extra = 0;
    for (u32 i = 0; i < t.count; ++i)
    {
        const char* token = t.buffer[i];
        const u32 token_length = t.sizes[i];
        for (u32 j = 0; j < tokens.count; ++j)
        {
            if (token_length != tokens.sizes[j]) continue;

            bool same = true;
            for (u32 k = 0; k < token_length; ++k)
            {
                if (token[k] != tokens.buffer[j][k])
                {
                    same = false;
                    break;
                }
            }
            if (j < types->count)
            {
                if (same)
                {
                    // TODO: when token_length is larger than type
                    const int length_minus_one = types->sizes[j] - token_length;
                    const int offset = t.offsets[i] + (token_length - 1);
                    if (length_minus_one > 0)
                    {
                        for (int k = (int)len; k > offset; --k)
                        {
                            temp[k + length_minus_one] = temp[k];
                        }
                    }
                    else if (length_minus_one < 0)
                    {
                        for (u32 k = (u32)offset; k < len; ++k)
                        {
                            temp[k + length_minus_one] = temp[k];
                        }
                    }
                    memcpy(temp + t.offsets[i], types->buffer[j],
                           types->sizes[j]);
                    len += length_minus_one;
                    offset_extra += length_minus_one;
                    break;
                }
            }
            else
            {
                // TODO: ERROR
            }
        }
        const u32 next = i < (t.count - 1);
        t.offsets[i + next] += offset_extra * next;
    }
    for (u32 i = 0; i < t.count; ++i)
    {
        char* token = t.buffer[i];
        char* temp_token = allocate_and_copy_string(token, t.sizes[i], 0, 1);
        temp_token[t.sizes[i]] = ';';
        if (hash_table_get_cc(struct_table, token) ||
            hash_table_get_cc(function_table, token) ||
            hash_table_get_cc(function_table, temp_token))
        {
            if (i < t.count - 2)
            {
                const char* delims_ = "<>,";
                const u32 delim_count_ = (u32)strlen(delims_);

                Array_Char stack = { 0 };
                array_create(&stack, 20);
                bool wrong = false;
                u32 offset = 0;
                const u32 starting_index = t.offsets[i] + t.sizes[i];
                u32 count = starting_index;
                for (; count < len; ++count)
                {
                    if (temp[count] == '<')
                    {
                        array_append(&stack, '<');
                        offset = count;
                        ++count;
                        break;
                    }
                    else if (temp[count] == '>' || temp[count] != ' ' ||
                             temp[count] != '\t')
                    {
                        wrong = true;
                        break;
                    }
                }
                for (; count < len && stack.size != 0 && !wrong; ++count)
                {
                    if (temp[count] == '<')
                    {
                        array_append(&stack, '<');
                    }
                    else if (temp[count] == '>')
                    {
                        bool result = false;
                        array_pop(&stack, &result);
                        if (!result)
                        {
                            ++count;
                            break;
                        }
                    }
                }

                if (!wrong && stack.size == 0)
                {
                    const u32 length = count - starting_index;
                    Tokens tokens_ = tokens_read(temp + t.offsets[i + 1],
                                                 length, delims_, delim_count_);

                    if (tokens_.count)
                    {
                        const u32 index = t.sizes[i];
                        char stored = token[index];
                        token[index] = ';';

                        char* name_type_concat = token;
                        u32 token_length = index + 1;
                        u32 token_total_length = 0;
                        for (u32 j = 0; j < tokens_.count; j++)
                        {
                            tokens_.sizes[j] =
                                trim(tokens_.buffer[j], tokens_.sizes[j]);

                            name_type_concat = concatinate(
                                name_type_concat, token_length,
                                tokens_.buffer[j], tokens_.sizes[j] + 1, 0,
                                &token_length);

                            if (j + 1 < tokens_.count)
                            {
                                name_type_concat[token_length - 1] = ',';
                            }

                            token_total_length += tokens_.sizes[j];
                        }

                        char** struct_postfix0 =
                            hash_table_get_cc(postfix_table, name_type_concat);
                        token[index] = stored;
                        free(name_type_concat);

                        const bool should_free_struct_postfix =
                            !struct_postfix0;
                        char* struct_postfix = NULL;
                        u32 po_length = 0;
                        if (!struct_postfix0)
                        {
                            struct_postfix = generate_postfix_from_type(
                                &tokens_, token_total_length, &po_length);
                        }
                        else
                        {
                            struct_postfix = *struct_postfix0;
                            po_length = (u32)strlen(struct_postfix);
                        }
                        if (po_length < length)
                        {
                            memcpy(temp + offset + po_length,
                                   temp + offset + length,
                                   len - (offset + length));
                        }
                        else if (po_length > length)
                        {
                            const u32 diff = po_length - length;
                            const int end = offset + length;
                            for (int j = len; j >= end; --j)
                            {
                                temp[j + diff] = temp[j];
                            }
                        }
                        const int diff = (int)po_length - (int)length;
                        len += diff;
                        temp[len] = '\0';
                        for (u32 j = i + tokens_.count + 1; j < t.count; ++j)
                        {
                            t.offsets[j] += diff;
                        }

                        memcpy(temp + offset, struct_postfix, po_length);

                        tokens_free(&tokens_);
                        if (should_free_struct_postfix)
                        {
                            free(struct_postfix);
                        }
                    }
                }
            }
        }
        free(temp_token);
    }
    if (is_struct)
    {
        const char* typedef_struct = "typedef struct ";
        array_append_string(final_buffer, typedef_struct,
                            (u32)strlen(typedef_struct));
        array_append_string(final_buffer, name, name_length);
        array_append_string(final_buffer, postfix, postfix_length);
        array_append(final_buffer, '\n');
        array_append(final_buffer, '{');
        array_append_string(final_buffer, temp, (u32)strlen(temp));
        array_append(final_buffer, '}');
        array_append(final_buffer, ' ');
        array_append_string(final_buffer, name, name_length);
        array_append_string(final_buffer, postfix, postfix_length);
        const char* end = ";\n\n";
        array_append_string(final_buffer, end, (u32)strlen(end));
    }
    else
    {
        // TODO: do this outside of the function
        for (u32 i = 1; i < postfix_length; ++i)
        {
            if (postfix[i] != '_') postfix[i] |= 32;
        }
        const char stored = name[name_length - 1];
        if (name[name_length - 1] == ';')
        {
            name[name_length - 1] = '\0';
        }
        u32 prefix_offset_extra = 0;
        for (u32 i = 0; i < t.count; ++i)
        {
            const char* token = t.buffer[i];
            const u32 token_length = t.sizes[i];
            if (strcmp(token, name) == 0)
            {
                const int offset =
                    t.offsets[i] + token_length + prefix_offset_extra;
                if (memcmp(temp + offset, postfix, postfix_length) == 0)
                {
                    // Already fixed the postfix
                    continue;
                }
                for (int j = (int)len; j >= offset; --j)
                {
                    temp[j + postfix_length] = temp[j];
                }
                memcpy(temp + offset, postfix, postfix_length);
                len += postfix_length;
                prefix_offset_extra += postfix_length;
            }
        }
        name[name_length - 1] = stored;
        array_append_string(final_buffer, temp, (u32)strlen(temp));
        const char* end = "\n\n";
        array_append_string(final_buffer, end, (u32)strlen(end));
    }
    free(temp);
    tokens_free(&t);
    tokens_free(&tokens);
}

internal u32 remove_spaces(char* const string, u32 string_length)
{
    for (u32 i = 0; i < string_length; i++)
    {
        if (string[i] == ' ')
        {
            for (u32 j = i; j < string_length - 1; j++)
            {
                string[j] = string[j + 1];
            }
            --string_length;
            string[string_length] = '\0';
        }
    }
    return string_length;
}

internal void look_for_and_save_includes(Mcgen_Context_Internal* const ctx,
                                         Hash_Table_Ca* const file_buffers)
{
    for (u32 i = 0; i < ctx->types.size; ++i)
    {
        Generation_Output_File g_o = ctx->types.data[i];

        const char* output_file = g_o.output_definition;
        for (u32 file_count = 0; file_count < 2 && output_file;
             ++file_count, output_file = g_o.output_implementation)
        {
            Array_Char* file_buffer =
                hash_table_get_ca(file_buffers, output_file);
            if (file_buffer)
            {
                char* full_output_path = get_full_path(ctx, output_file);
                File_Attrib file = file_read(full_output_path);
                free(full_output_path);
                if (file.size == 0) continue;

                Line line = { 0 };
                for (u32 j = 0, line_count = 1; j < file.size; ++line_count)
                {
                    line = line_read(&file, &j, false);
                    // line.size = trim(line.buffer, line.size);

                    const char* const first_token_delim = " \"<\n";
                    const u32 first_token_delim_count =
                        (u32)strlen(first_token_delim);
                    u32 offset = 0;
                    char* first_token = get_first_token(
                        line.buffer, line.size, first_token_delim,
                        first_token_delim_count, &offset);

                    if (first_token)
                    {
                        u32 first_token_length = offset - 1;
                        first_token_length =
                            remove_spaces(first_token, first_token_length);
                        if (strcmp(first_token, "#include") == 0 ||
                            strcmp(first_token, "#pragma") == 0)
                        {
                            array_append_string(file_buffer, line.buffer,
                                                line.size);
                        }
                        else if (strcmp(first_token, "//") == 0)
                        {
                            char* next_token = get_first_token(
                                line.buffer + offset, line.size - offset,
                                first_token_delim, first_token_delim_count,
                                &offset);
                            if (next_token)
                            {
                                if (strcmp(next_token, "@save") == 0)
                                {
                                    array_append_string(file_buffer,
                                                        line.buffer, line.size);
                                    for (; j < file.size; ++line_count)
                                    {
                                        free(line.buffer);
                                        line = line_read(&file, &j, false);
                                        // line.size = trim(line.buffer,
                                        // line.size);
                                        array_append_string(file_buffer,
                                                            line.buffer,
                                                            line.size);

                                        char* first_token1 = get_first_token(
                                            line.buffer, line.size,
                                            first_token_delim,
                                            first_token_delim_count, &offset);

                                        if (!first_token1) continue;
                                        if (strcmp(first_token1, "//") == 0)
                                        {
                                            char* next_token1 = get_first_token(
                                                line.buffer + offset,
                                                line.size - offset,
                                                first_token_delim,
                                                first_token_delim_count,
                                                &offset);

                                            if (!next_token1) continue;
                                            if (strcmp(next_token1, "@end") ==
                                                0)
                                            {
                                                free(next_token1);
                                                free(first_token1);
                                                break;
                                            }
                                            free(next_token1);
                                        }
                                        free(first_token1);
                                    }
                                }
                                free(next_token);
                            }
                        }
                        free(first_token);
                    }
                    free(line.buffer);
                }
                array_append(file_buffer, '\n');
            }
        }
    }
}

Mcgen_Context* mcgen_context_create(const char* const working_directory)
{
    Mcgen_Context_Internal* const ctx = mcgen_calloc(Mcgen_Context_Internal, 1);
    mcgen_change_working_directory(ctx, working_directory);
    array_create_d(&ctx->types);
    array_create_d(&ctx->files);
    array_create_d(&ctx->name_type);
    ctx->name_postfix = hash_table_create_cc(128, hash_murmur);
    ctx->linked_names = hash_table_create_cap(128, hash_murmur);
    return ctx;
}

void mcgen_context_destroy(Mcgen_Context* const ctx)
{
    Mcgen_Context_Internal* const ctx_internal =
        (Mcgen_Context_Internal* const)ctx;
    free(ctx_internal->types.data);
    free(ctx_internal->files.data);
    free(ctx_internal->name_type.data);
    free(ctx_internal);
}

void mcgen_change_working_directory(Mcgen_Context* const ctx,
                                    const char* const working_directory)
{
    Mcgen_Context_Internal* const ctx_internal =
        (Mcgen_Context_Internal* const)ctx;
    if (working_directory)
    {
        const u32 working_directory_len = (u32)strlen(working_directory);
        ctx_internal->working_directory =
            mcgen_calloc(char, working_directory_len + 2);
        memcpy(ctx_internal->working_directory, working_directory,
               working_directory_len);
        ctx_internal->working_directory_len = working_directory_len;
        const char last_char =
            ctx_internal->working_directory[working_directory_len - 1];
        if (last_char != '\\' || last_char != '/')
        {
            // TODO: find what is being used
            ctx_internal->working_directory[working_directory_len] = '\\';
        }
    }
    else
    {
        ctx_internal->working_directory = mcgen_calloc(char, 3);
        memcpy(ctx_internal->working_directory, "./", 2);
        ctx_internal->working_directory_len = 2;
    }
}

void mcgen_append_type_files(Mcgen_Context* const ctx,
                             const char* const generation_file,
                             const char* const output_definition_file,
                             const char* const output_implementation_file)
{
    Mcgen_Context_Internal* const ctx_internal =
        (Mcgen_Context_Internal* const)ctx;
    const Generation_Output_File g_o = { .generation = generation_file,
                                         .output_definition =
                                             output_definition_file,
                                         .output_implementation =
                                             output_implementation_file };
    array_append(&ctx_internal->types, g_o);
}

void mcgen_append_type_file(Mcgen_Context* const ctx,
                            const char* const generation_file,
                            const char* const output_file)
{
    mcgen_append_type_files(ctx, generation_file, output_file, NULL);
}

void mcgen_append_file(Mcgen_Context* const ctx,
                       const char* const generation_file,
                       const char* const output_file)
{
    Mcgen_Context_Internal* const ctx_internal =
        (Mcgen_Context_Internal* const)ctx;
    const Generation_Output_File g_o = {
        .generation = generation_file,
        .output_definition = output_file,
    };
    array_append(&ctx_internal->files, g_o);
}

internal void append_types(Mcgen_Context_Internal* const ctx_internal,
                           const char* const name, char** const types,
                           const u32 type_count, const char* const file_name,
                           const u32 line_number)
{
    const u32 name_length = (u32)strlen(name);
    for (u32 i = 0; i < type_count; ++i)
    {
        char* type = types[i];
        const u32 type_length = (u32)strlen(type);
        type = allocate_and_copy_string(type, type_length, 0, 0);
        remove_spaces_before_and_after_delim(type, type_length, ',');
        const Name_Type n_t = {
            .name = allocate_and_copy_string(name, name_length, 0, 0),
            .type = type,
            .file_name = file_name,
            .line_number = line_number,
        };
        array_append(&ctx_internal->name_type, n_t);
    }
}

void mcgen_append_type_(Mcgen_Context* const ctx, const char* const name,
                        char** const types, const u32 type_count,
                        const char* const file_name, const u32 line_number)
{
    Mcgen_Context_Internal* const ctx_internal =
        (Mcgen_Context_Internal* const)ctx;

    append_types(ctx_internal, name, types, type_count, file_name, line_number);

    Array_Char_Ptr* array =
        hash_table_get_cap(&ctx_internal->linked_names, name);
    if (array)
    {
        for (u32 i = 0; i < array->size; ++i)
        {
            append_types(ctx_internal, array->data[i], types, type_count,
                         file_name, line_number);
        }
    }
}

internal void set_postfix(Mcgen_Context_Internal* const ctx_internal,
                          const char* const name, const char* const type,
                          const char* const postfix)
{
    u32 name_length = (u32)strlen(name);
    const u32 type_length = (u32)strlen(type);
    char* key =
        concatinate(name, name_length, type, type_length, ';', &name_length);
    name_length = remove_spaces_before_and_after_delim(key, name_length, ',');

    const u32 postfix_length = (u32)strlen(postfix);
    char* value = allocate_and_copy_string(postfix, postfix_length, 0, 0);
    trim(value, postfix_length);

    hash_table_insert_cc(&ctx_internal->name_postfix, key, value);
}

void mcgen_set_postfix(Mcgen_Context* const ctx, const char* const name,
                       const char* const type, const char* const postfix)
{
    Mcgen_Context_Internal* const ctx_internal =
        (Mcgen_Context_Internal* const)ctx;

    set_postfix(ctx_internal, name, type, postfix);

    Array_Char_Ptr* array =
        hash_table_get_cap(&ctx_internal->linked_names, name);
    if (array)
    {
        for (u32 i = 0; i < array->size; ++i)
        {
            set_postfix(ctx_internal, array->data[i], type, postfix);
        }
    }
}

void mcgen_set_postfixs(Mcgen_Context* const ctx, const char* const name,
                        const char** const types, const u32 type_count,
                        const char** const postfixs, const u32 postfix_count)
{
    const u32 min_count = min(type_count, postfix_count);
    for (u32 i = 0; i < min_count; ++i)
    {
        mcgen_set_postfix(ctx, name, types[i], postfixs[i]);
    }
}

// TODO: might need to have one for types and one for postfixs
void mcgen_link_name_(Mcgen_Context* const ctx, const char* const name,
                      const char** const names, const u32 name_count)
{
    Mcgen_Context_Internal* const ctx_internal =
        (Mcgen_Context_Internal* const)ctx;

    Array_Char_Ptr* array =
        hash_table_get_cap(&ctx_internal->linked_names, name);
    if (!array)
    {
        Array_Char_Ptr array0;
        array_create(&array0, name_count);
        array = &array0;
    }
    else
    {
        // NOTE: This should not happen if you use it correctly
        Cell_Cap* cell =
            hash_table_remove_cap(&ctx_internal->linked_names, name);
        free(cell->key);
    }

    for (u32 i = 0; i < name_count; ++i)
    {
        const char* name_temp = names[i];
        const u32 name_length = (u32)strlen(name_temp);
        char* name_to_link =
            allocate_and_copy_string(name_temp, name_length, 0, 0);
        array_append(array, name_to_link);
    }
    char* key = allocate_and_copy_string(name, (u32)strlen(name), 0, 0);
    hash_table_insert_cap(&ctx_internal->linked_names, key, *array);
}

void mcgen_generate_code(Mcgen_Context* const ctx)
{
    Mcgen_Context_Internal* const ctx_internal =
        (Mcgen_Context_Internal* const)ctx;

    Hash_Table_Cc struct_definition_table =
        hash_table_create_cc(128, hash_murmur);
    Hash_Table_Cc struct_types = hash_table_create_cc(128, hash_murmur);

    Hash_Table_Cc function_definition_table =
        hash_table_create_cc(128, hash_murmur);
    Hash_Table_Cc function_types = hash_table_create_cc(128, hash_murmur);

    Hash_Table_Cc templates = hash_table_create_cc(128, hash_murmur);
    Hash_Table_Cc outputs = hash_table_create_cc(128, hash_murmur);
    Hash_Table_Ca file_buffers = hash_table_create_ca(128, hash_murmur);

    const char* delims = " <\n";
    const u32 delim_count = (u32)strlen(delims);
    for (u32 i = 0; i < ctx_internal->types.size; ++i)
    {
        Generation_Output_File g_o = ctx_internal->types.data[i];
        char* const full_path = get_full_path(ctx_internal, g_o.generation);

        Array_Char* const temp =
            hash_table_get_ca(&file_buffers, g_o.output_definition);
        if (!temp)
        {
            if (!hash_table_get_ca(&file_buffers, (char*)g_o.output_definition))
            {
                Array_Char final_buffer = { 0 };
                array_create(&final_buffer, KILOBYTE(100));
                hash_table_insert_ca(
                    &file_buffers, (char*)g_o.output_definition, final_buffer);
            }
            if (g_o.output_implementation)
            {
                if (!hash_table_get_ca(&file_buffers,
                                       (char*)g_o.output_implementation))
                {
                    Array_Char final_buffer = { 0 };
                    array_create(&final_buffer, KILOBYTE(100));
                    hash_table_insert_ca(&file_buffers,
                                         (char*)g_o.output_implementation,
                                         final_buffer);
                }
            }
        }

        File_Attrib file = file_read(full_path);

        Line line = { 0 };
        for (u32 j = 0, line_count = 1; j < file.size; ++line_count)
        {
            line = line_read(&file, &j, false);
            line.size = trim(line.buffer, line.size);
            u32 offset = 0;
            char* const first_token = get_first_token(
                line.buffer, line.size, delims, delim_count, &offset);
            if (first_token)
            {
                j = parse_first_token(&struct_definition_table,
                                      &function_definition_table, &templates,
                                      &outputs, &file, full_path,
                                      (char*)g_o.output_definition,
                                      (char*)g_o.output_implementation, &line,
                                      first_token, offset, j, &line_count);
            }
            free(first_token);
            free(line.buffer);
        }
        free(full_path);
        free(file.buffer);
    }

    look_for_and_save_includes(ctx_internal, &file_buffers);

    for (u32 i = 0; i < ctx_internal->name_type.size; ++i)
    {
        char* name = (char*)ctx_internal->name_type.data[i].name;
        char** output_file0 = hash_table_get_cc(&outputs, name);
        char* output_file = NULL;
        u32 name_length = (u32)strlen(name);
        bool definition = false;
        if (!output_file0)
        {
            char* temp = name;
            name = allocate_and_copy_string(temp, name_length, 0, 1);
            name[name_length++] = ';';
            output_file0 = hash_table_get_cc(&outputs, name);
            definition = true;
        }
        if (output_file0)
        {
            output_file = *output_file0;
            char* function_name_check = NULL;
            Array_Char* definition_file_buffer = NULL;
            if (!definition)
            {

                function_name_check =
                    allocate_and_copy_string(name, name_length, 0, 1);
                function_name_check[name_length] = ';';
                const char** definition_output_file0 =
                    hash_table_get_cc(&outputs, function_name_check);

                const char* definition_output_file = NULL;
                if (definition_output_file0)
                {
                    definition_output_file = *definition_output_file0;
                    definition_file_buffer = hash_table_get_ca(
                        &file_buffers, definition_output_file);
                }
            }

            Array_Char* file_buffer =
                hash_table_get_ca(&file_buffers, output_file);
            if (file_buffer)
            {
                const char* type = ctx_internal->name_type.data[i].type;
                const u32 type_length_original = (u32)strlen(type);
                const char* type_delims = ",;\0";
                const u32 type_delim_count = (u32)strlen(delims);
                Tokens types = tokens_read(type, type_length_original + 1,
                                           type_delims, type_delim_count);
                u32 type_length = 0;
                for (u32 j = 0; j < types.count; ++j)
                {
                    u32* size = types.sizes + j;
                    *size = trim(types.buffer[j], *size);
                    type_length += *size;
                }

                const u32 index = name_length - definition;
                char temp = name[index];
                name[index] = ';';
                char* name_type_concat =
                    concatinate(name, name_length + !definition, type,
                                type_length_original, 0, NULL);
                char** postfix0 = hash_table_get_cc(&ctx_internal->name_postfix,
                                                    name_type_concat);
                char* postfix = NULL;
                name[index] = temp;
                free(name_type_concat);

                bool generated_postfix = false;
                u32 postfix_length = 0;
                if (!postfix0)
                {
                    postfix = generate_postfix_from_type(&types, type_length,
                                                         &postfix_length);
                    generated_postfix = true;
                }
                else
                {
                    postfix = *postfix0;
                    postfix_length = (u32)strlen(postfix);
                }

                bool is_struct = true;
                const char** sc0 =
                    hash_table_get_cc(&struct_definition_table, name);
                const char* sc = NULL;
                if (!sc0)
                {
                    is_struct = false;
                    sc0 = hash_table_get_cc(&function_definition_table, name);
                }
                if (sc0)
                {
                    sc = *sc0;
                    char* type_check = (char*)type;
                    u32 type_check_len = types.sizes[0];
                    if (types.count > 1)
                    {
                        type_check = types.buffer[0];
                        for (u32 j = 1; j < types.count; ++j)
                        {
                            char* type_check_result = concatinate(
                                type_check, type_check_len, types.buffer[j],
                                types.sizes[j], 0, &type_check_len);

                            if (j > 1) free(type_check);
                            type_check = type_check_result;
                        }
                    }
                    char* type_check_result = concatinate(
                        name, name_length, type_check, type_check_len, 0, NULL);

                    if (type_check != type) free(type_check);
                    type_check = type_check_result;

                    const char** te0 = NULL;
                    if (is_struct)
                    {
                        te0 = hash_table_get_cc(&struct_types, type_check);
                    }
                    else
                    {
                        te0 = hash_table_get_cc(&function_types, type_check);
                    }

                    if (te0)
                    {
                        free(type_check);
                    }
                    else
                    {
                        if (is_struct)
                        {
                            hash_table_insert_cc(&struct_types, type_check,
                                                 " ");
                        }
                        else
                        {
                            hash_table_insert_cc(&function_types, type_check,
                                                 " ");
                        }
                        generate_from_name(&ctx_internal->name_postfix,
                                           &templates, &struct_definition_table,
                                           &function_definition_table,
                                           is_struct, sc, name, name_length,
                                           &types, postfix, postfix_length,
                                           file_buffer);
                        if (definition_file_buffer)
                        {
                            sc = *hash_table_get_cc(&function_definition_table,
                                                    function_name_check);
                            generate_from_name(
                                &ctx_internal->name_postfix, &templates,
                                &struct_definition_table,
                                &function_definition_table, false, sc, name,
                                name_length, &types, postfix, postfix_length,
                                definition_file_buffer);
                            free(function_name_check);
                        }
                    }
                }
                tokens_free(&types);
                if (generated_postfix)
                {
                    free(postfix);
                }
            }
        }
        else
        {
            fprintf(stderr, "%s:%u: \nerror: \"%s\" is not defined.\n",
                    ctx_internal->name_type.data[i].file_name,
                    ctx_internal->name_type.data[i].line_number, name);

            exit(1);
        }
    }

#if 0
    for (u32 i = 0; i < ctx_internal->files.size; ++i)
    {
        char* full_path = get_full_path(ctx_internal, ctx_internal->files.data[i].generation);
        char* gen_full_path = get_full_path(ctx_internal, ctx_internal->files.data[i].output);
        File_Attrib file = file_read(full_path);

        Line line = { 0 };
        for (u32 j = 0, line_count = 1; j < file.size; ++line_count)
        {
            line = line_read(&file, &j, false);
            line.size = trim(line.buffer, line.size);

            u32 offset = 0;
            char* first_token =
                get_first_token(line.buffer, line.size, "<", 1, &offset);

            if (first_token)
            {
                const u32 token_length = trim(first_token, offset - 1);

                const u32 offest_to_first_template =
                    j - ((line.size - offset) + 1);
                const Tokens types =
                    templates_read(file.buffer + offest_to_first_template,
                                   file.size - offest_to_first_template);

                types.sizes[0] = trim(types.buffer[0], types.sizes[0]);
                const u32 underscore = 1;
                const u32 max_pointer_count = 8;
                const u32 pointer_padding = 3 * max_pointer_count;
                u32 postfix_length = types.sizes[0] + underscore;
                const u32 postfix_byte_size =
                    postfix_length + pointer_padding + NULL_TERMINATOR;
                char* postfix = mcgen_calloc(char, postfix_byte_size);
                postfix[0] = '_';
                memcpy(postfix + 1, types.buffer[0], types.sizes[0]);
                postfix[1] &= ~32; // to upper
                u32 pointer_count = 0;
                for (u32 k = 2; k < postfix_length && postfix[k]; ++k)
                {
                    if (postfix[k] == ' ')
                    {
                        for (u32 d = k; d < postfix_length - 1; ++d)
                        {
                            postfix[d] = postfix[d + 1];
                        }
                        postfix[k] &= ~32;
                        --k;
                        --postfix_length;
                    }
                    else if (postfix[k] == '*')
                    {
                        const char* ptr = "Ptr";
                        const u32 ptr_length = (u32)strlen(ptr);
                        if (++pointer_count >= max_pointer_count)
                        {
                            line_print_error_message(
                                full_path, line_count,
                                "To many pointers in template args \"*\"",
                                &line, offset, true);
                        }
                        if (k != postfix_length - 1)
                        {
                            // TODO: is it valid to not be "*" here?
                            if (postfix[k + 1] != '*') postfix[k + 1] &= ~32;
                            for (int d = (int)postfix_byte_size - 1; d > (int)k;
                                 --d)
                            {
                                postfix[d] = postfix[d - ptr_length];
                            }
                        }
                        memcpy(postfix + k, ptr, ptr_length);
                        k += ptr_length - 1;
                        postfix_length += (ptr_length - 1); // -1 remove "*"
                    }
                }
                postfix[postfix_length] = '\0';

                const char* output_file =
                    hash_table_get_cc(&struct_outputs, first_token);
                if (output_file)
                {
                    Array_Char* file_buffer =
                        hash_table_get_ca(&file_buffers, output_file);

                    if (file_buffer)
                    {
                        generate_struct_from_name(
                            &struct_definition_table, &struct_templates,
                            &struct_types, first_token, token_length,
                            types.buffer[0], types.sizes[0], postfix,
                            postfix_length, file_buffer);
                    }
                }
                free(postfix);
            }

            free(first_token);
            free(line.buffer);
        }
        free(full_path);
        free(gen_full_path);
        free(file.buffer);
    }
#endif

    for (u32 i = 0; i < ctx_internal->types.size; ++i)
    {
        Generation_Output_File g_o = ctx_internal->types.data[i];

        Array_Char* file_buffer =
            hash_table_get_ca(&file_buffers, g_o.output_definition);
        if (file_buffer) // Should always be true
        {
            char* full_output_path =
                get_full_path(ctx_internal, g_o.output_definition);
            file_write(full_output_path, file_buffer->data, file_buffer->size);
            free(full_output_path);
        }
        if (g_o.output_implementation)
        {
            file_buffer =
                hash_table_get_ca(&file_buffers, g_o.output_implementation);
            if (file_buffer)
            {
                char* full_output_path =
                    get_full_path(ctx_internal, g_o.output_implementation);
                file_write(full_output_path, file_buffer->data,
                           file_buffer->size);
                free(full_output_path);
            }
        }
    }
}

u64 hash_murmur(const void* key, u32 len, u64 seed)
{
    u64 h = seed;
    const char* key2 = (const char*)key;
    if (len > 3)
    {
        const u32* key_x4 = (const u32*)key;
        u32 i, n = len >> 2;
        for (i = 0; i < n; i++)
        {
            u32 k = key_x4[i];
            k *= 0xcc9e2d51;
            k = (k << 15) | (k >> 17);
            k *= 0x1b873593;
            h ^= k;
            h = (h << 13) | (h >> 19);
            h = (h * 5) + 0xe6546b64;
        }
        key2 = (const char*)(key_x4 + n);
        len &= 3;
    }

    u64 k1 = 0;
    switch (len)
    {
        case 3:
        {
            k1 ^= key2[2] << 16;
        }
        case 2:
        {
            k1 ^= key2[1] << 8;
        }
        case 1:
        {
            k1 ^= key2[0];
            k1 *= 0xcc9e2d51;
            k1 = (k1 << 15) | (k1 >> 17);
            k1 *= 0x1b873593;
            h ^= k1;
        }
    }

    h ^= len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}

u64 hash_djb2(const void* key, u32 len, u64 seed)
{
    u64 hash = seed;

    const u8* key_u8 = (const u8*)key;
    for(u32 i = 0; i < len; i++)
    {
        hash = ((hash << 5) + hash) + key_u8[i];
    }
    return hash;
}

global u64 HASH_SEED = 0;

void hash_table_set_seed(u64 seed)
{
    // | 1 to make it odd
    HASH_SEED = seed | 1;
}

internal u32 round_up_power_of_two(u32 capacity)
{
    capacity--;
    capacity |= capacity >> 1;
    capacity |= capacity >> 2;
    capacity |= capacity >> 4;
    capacity |= capacity >> 8;
    capacity |= capacity >> 16;
    return capacity + 1;
}

Hash_Table_Cc hash_table_create_cc(u32 capacity, u64 (*hash_function)(const void* key, u32 len, u64 seed))
{ 
    capacity = max(round_up_power_of_two(capacity), 32); 
    Hash_Table_Cc out = { 
        .cells = mcgen_calloc(Cell_Cc, capacity), 
        .capacity = capacity, 
        .hash_function = hash_function, 
    }; 
    return out; 
}

Hash_Table_Ca hash_table_create_ca(u32 capacity, u64 (*hash_function)(const void* key, u32 len, u64 seed))
{ 
    capacity = max(round_up_power_of_two(capacity), 32); 
    Hash_Table_Ca out = { 
        .cells = mcgen_calloc(Cell_Ca, capacity), 
        .capacity = capacity, 
        .hash_function = hash_function, 
    }; 
    return out; 
}

Hash_Table_Cap hash_table_create_cap(u32 capacity, u64 (*hash_function)(const void* key, u32 len, u64 seed))
{ 
    capacity = max(round_up_power_of_two(capacity), 32); 
    Hash_Table_Cap out = { 
        .cells = mcgen_calloc(Cell_Cap, capacity), 
        .capacity = capacity, 
        .hash_function = hash_function, 
    }; 
    return out; 
}

void hash_table_insert_cc(Hash_Table_Cc* table, char* key, char* value)
{ 
    u32 capacity_mask = table->capacity - 1; 
    u64 hashed_index = 
        table->hash_function(key, (u32)strlen(key), HASH_SEED) & capacity_mask; 
 
    Cell_Cc* cell = table->cells + hashed_index; 
    if (cell->active) 
    { 
        if (strcmp(cell->key, key) != 0) 
        { 
            cell = table->cells + (++hashed_index & capacity_mask); 
            for (u32 i = 0; i < table->size && cell->active; i++) 
            { 
                if (strcmp(cell->key, key) == 0) 
                { 
                    goto add_node; 
                } 
                cell = table->cells + (++hashed_index & capacity_mask); 
            } 
        } 
        else 
        { 
            goto add_node; 
        } 
    } 
    cell->active = true; 
    cell->deleted = false; 
    cell->key = key; 
add_node: 
    cell->value = value; 
    if (table->size++ >= (u32)(table->capacity * 0.4f)) 
    { 
        u32 old_capacity = table->capacity; 
        Cell_Cc* old_storage = table->cells; 
 
        table->size = 0; 
        table->capacity *= 2; 
        table->cells = mcgen_calloc(Cell_Cc, table->capacity); 
 
        for (u32 i = 0; i < old_capacity; i++) 
        { 
            cell = old_storage + i; 
            if (cell->active) 
            { 
                hash_table_insert_cc(table, cell->key, cell->value); 
            } 
        } 
        free(old_storage); 
    } 
}

void hash_table_insert_ca(Hash_Table_Ca* table, char* key, Array_Char value)
{ 
    u32 capacity_mask = table->capacity - 1; 
    u64 hashed_index = 
        table->hash_function(key, (u32)strlen(key), HASH_SEED) & capacity_mask; 
 
    Cell_Ca* cell = table->cells + hashed_index; 
    if (cell->active) 
    { 
        if (strcmp(cell->key, key) != 0) 
        { 
            cell = table->cells + (++hashed_index & capacity_mask); 
            for (u32 i = 0; i < table->size && cell->active; i++) 
            { 
                if (strcmp(cell->key, key) == 0) 
                { 
                    goto add_node; 
                } 
                cell = table->cells + (++hashed_index & capacity_mask); 
            } 
        } 
        else 
        { 
            goto add_node; 
        } 
    } 
    cell->active = true; 
    cell->deleted = false; 
    cell->key = key; 
add_node: 
    cell->value = value; 
    if (table->size++ >= (u32)(table->capacity * 0.4f)) 
    { 
        u32 old_capacity = table->capacity; 
        Cell_Ca* old_storage = table->cells; 
 
        table->size = 0; 
        table->capacity *= 2; 
        table->cells = mcgen_calloc(Cell_Ca, table->capacity); 
 
        for (u32 i = 0; i < old_capacity; i++) 
        { 
            cell = old_storage + i; 
            if (cell->active) 
            { 
                hash_table_insert_ca(table, cell->key, cell->value); 
            } 
        } 
        free(old_storage); 
    } 
}

void hash_table_insert_cap(Hash_Table_Cap* table, char* key, Array_Char_Ptr value)
{ 
    u32 capacity_mask = table->capacity - 1; 
    u64 hashed_index = 
        table->hash_function(key, (u32)strlen(key), HASH_SEED) & capacity_mask; 
 
    Cell_Cap* cell = table->cells + hashed_index; 
    if (cell->active) 
    { 
        if (strcmp(cell->key, key) != 0) 
        { 
            cell = table->cells + (++hashed_index & capacity_mask); 
            for (u32 i = 0; i < table->size && cell->active; i++) 
            { 
                if (strcmp(cell->key, key) == 0) 
                { 
                    goto add_node; 
                } 
                cell = table->cells + (++hashed_index & capacity_mask); 
            } 
        } 
        else 
        { 
            goto add_node; 
        } 
    } 
    cell->active = true; 
    cell->deleted = false; 
    cell->key = key; 
add_node: 
    cell->value = value; 
    if (table->size++ >= (u32)(table->capacity * 0.4f)) 
    { 
        u32 old_capacity = table->capacity; 
        Cell_Cap* old_storage = table->cells; 
 
        table->size = 0; 
        table->capacity *= 2; 
        table->cells = mcgen_calloc(Cell_Cap, table->capacity); 
 
        for (u32 i = 0; i < old_capacity; i++) 
        { 
            cell = old_storage + i; 
            if (cell->active) 
            { 
                hash_table_insert_cap(table, cell->key, cell->value); 
            } 
        } 
        free(old_storage); 
    } 
}

char** hash_table_get_cc(Hash_Table_Cc* table, const char* key)
{ 
    u32 capacity_mask = table->capacity - 1; 
    u64 hashed_index = 
        table->hash_function(key, (u32)strlen(key), HASH_SEED) & capacity_mask; 
 
    Cell_Cc* cell = table->cells + hashed_index; 
    if (cell->active) 
    { 
        if (strcmp(cell->key, key) != 0) 
        { 
            cell = table->cells + (++hashed_index & capacity_mask); 
            for (u32 i = 0; i < table->size && (cell->active || cell->deleted); 
                 i++) 
            { 
                if (strcmp(cell->key, key) == 0) 
                { 
                    return &cell->value; 
                } 
                cell = table->cells + (++hashed_index & capacity_mask); 
            } 
        } 
        else 
        { 
            return &cell->value; 
        } 
    } 
    return NULL; 
}

Array_Char* hash_table_get_ca(Hash_Table_Ca* table, const char* key)
{ 
    u32 capacity_mask = table->capacity - 1; 
    u64 hashed_index = 
        table->hash_function(key, (u32)strlen(key), HASH_SEED) & capacity_mask; 
 
    Cell_Ca* cell = table->cells + hashed_index; 
    if (cell->active) 
    { 
        if (strcmp(cell->key, key) != 0) 
        { 
            cell = table->cells + (++hashed_index & capacity_mask); 
            for (u32 i = 0; i < table->size && (cell->active || cell->deleted); 
                 i++) 
            { 
                if (strcmp(cell->key, key) == 0) 
                { 
                    return &cell->value; 
                } 
                cell = table->cells + (++hashed_index & capacity_mask); 
            } 
        } 
        else 
        { 
            return &cell->value; 
        } 
    } 
    return NULL; 
}

Array_Char_Ptr* hash_table_get_cap(Hash_Table_Cap* table, const char* key)
{ 
    u32 capacity_mask = table->capacity - 1; 
    u64 hashed_index = 
        table->hash_function(key, (u32)strlen(key), HASH_SEED) & capacity_mask; 
 
    Cell_Cap* cell = table->cells + hashed_index; 
    if (cell->active) 
    { 
        if (strcmp(cell->key, key) != 0) 
        { 
            cell = table->cells + (++hashed_index & capacity_mask); 
            for (u32 i = 0; i < table->size && (cell->active || cell->deleted); 
                 i++) 
            { 
                if (strcmp(cell->key, key) == 0) 
                { 
                    return &cell->value; 
                } 
                cell = table->cells + (++hashed_index & capacity_mask); 
            } 
        } 
        else 
        { 
            return &cell->value; 
        } 
    } 
    return NULL; 
}

Cell_Cc* hash_table_remove_cc(Hash_Table_Cc* table, const char* key)
{ 
    u32 capacity_mask = table->capacity - 1; 
    u64 hashed_index = 
        table->hash_function(key, (u32)strlen(key), HASH_SEED) & capacity_mask; 
 
    Cell_Cc* cell = table->cells + hashed_index; 
    if (cell->active) 
    { 
        if (strcmp(cell->key, key) != 0) 
        { 
            cell = table->cells + (++hashed_index & capacity_mask); 
            for (u32 i = 0; i < table->size && (cell->active || cell->deleted); 
                 i++) 
            { 
                if (strcmp(cell->key, key) == 0) 
                { 
                    cell->active = false; 
                    cell->deleted = true; 
                    table->size--; 
                    return cell; 
                } 
                cell = table->cells + (++hashed_index & capacity_mask); 
            } 
        } 
        else 
        { 
            cell->active = false; 
            cell->deleted = true; 
            table->size--; 
            return cell; 
        } 
    } 
    return NULL; 
}

Cell_Ca* hash_table_remove_ca(Hash_Table_Ca* table, const char* key)
{ 
    u32 capacity_mask = table->capacity - 1; 
    u64 hashed_index = 
        table->hash_function(key, (u32)strlen(key), HASH_SEED) & capacity_mask; 
 
    Cell_Ca* cell = table->cells + hashed_index; 
    if (cell->active) 
    { 
        if (strcmp(cell->key, key) != 0) 
        { 
            cell = table->cells + (++hashed_index & capacity_mask); 
            for (u32 i = 0; i < table->size && (cell->active || cell->deleted); 
                 i++) 
            { 
                if (strcmp(cell->key, key) == 0) 
                { 
                    cell->active = false; 
                    cell->deleted = true; 
                    table->size--; 
                    return cell; 
                } 
                cell = table->cells + (++hashed_index & capacity_mask); 
            } 
        } 
        else 
        { 
            cell->active = false; 
            cell->deleted = true; 
            table->size--; 
            return cell; 
        } 
    } 
    return NULL; 
}

Cell_Cap* hash_table_remove_cap(Hash_Table_Cap* table, const char* key)
{ 
    u32 capacity_mask = table->capacity - 1; 
    u64 hashed_index = 
        table->hash_function(key, (u32)strlen(key), HASH_SEED) & capacity_mask; 
 
    Cell_Cap* cell = table->cells + hashed_index; 
    if (cell->active) 
    { 
        if (strcmp(cell->key, key) != 0) 
        { 
            cell = table->cells + (++hashed_index & capacity_mask); 
            for (u32 i = 0; i < table->size && (cell->active || cell->deleted); 
                 i++) 
            { 
                if (strcmp(cell->key, key) == 0) 
                { 
                    cell->active = false; 
                    cell->deleted = true; 
                    table->size--; 
                    return cell; 
                } 
                cell = table->cells + (++hashed_index & capacity_mask); 
            } 
        } 
        else 
        { 
            cell->active = false; 
            cell->deleted = true; 
            table->size--; 
            return cell; 
        } 
    } 
    return NULL; 
}


#endif
