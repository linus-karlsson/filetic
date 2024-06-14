#pragma once
#include "define.h"
#include "math/ftic_math.h"
#include "util.h"


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

void init_ttf_atlas(i32 width_atlas, i32 height_atlas, f32 pixel_height, u32 glyph_count, u32 glyph_offset, const char* font_file_path, u8* bitmap, FontTTF* font_out);

u32 text_gen(const CharacterTTF* c_ttf, const char* text, float texture_index, V3 pos, f32 scale, f32 line_height, u32* new_lines_count, f32* x_advance, VertexArray* array);
