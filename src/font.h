#pragma once
#include "define.h"
#include "math/ftic_math.h"
#include "util.h"
#include "collision.h"

typedef struct CharacterTTF
{
    V2 dimensions;
    V2 offset;
    V4 text_coords;
    f32 x_advance;
} CharacterTTF;

typedef struct FontTTF
{
    f32 tex_index;
    f32 line_height;
    f32 pixel_height;
    u32 char_count;
    CharacterTTF* chars;
} FontTTF;

typedef struct SelectionCharacter
{
    char character;
    AABB aabb;
} SelectionCharacter;

typedef struct SelectionCharacterArray
{
    u32 size;
    u32 capacity;
    SelectionCharacter* data;
} SelectionCharacterArray;

void init_ttf_atlas(i32 width_atlas, i32 height_atlas, f32 pixel_height, u32 glyph_count, u32 glyph_offset, const char* font_file_path, u8* bitmap, FontTTF* font_out);

#define text_generation(c_ttf, text, texture_index, pos, scale, line_height, new_lines_count, x_advance, aabbs, array) text_generation_color(c_ttf, text, texture_index, pos, scale, line_height, v4i(1.0f), new_lines_count, x_advance, aabbs, array)
u32 text_generation_color(const CharacterTTF* c_ttf, const char* text, float texture_index, V2 pos, f32 scale, f32 line_height, V4 color, u32* new_lines_count, f32* x_advance, SelectionCharacterArray* selection_chars, VertexArray* array);
f32 text_x_advance(const CharacterTTF* c_ttf, const char* text, u32 text_len, f32 scale);
i32 text_check_length_within_boundary(const CharacterTTF* c_ttf, const char* text, u32 text_len, f32 scale, float boundary);
