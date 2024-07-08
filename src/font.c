#include "font.h"
#include "opengl_util.h"
#include <stb/stb_truetype.h>

void init_ttf_atlas(i32 width_atlas, i32 height_atlas, f32 pixel_height,
                    u32 glyph_count, u32 glyph_offset,
                    const char* font_file_path, u8* bitmap, FontTTF* font_out)
{
    FileAttrib file = file_read(font_file_path);
    stbtt_bakedchar* data =
        (stbtt_bakedchar*)calloc(glyph_count, sizeof(stbtt_bakedchar));
    stbtt_BakeFontBitmap(file.buffer, 0, pixel_height, bitmap, width_atlas,
                         height_atlas, glyph_offset, glyph_count, data);

    FontTTF font = { 0 };
    font.line_height = pixel_height;
    font.pixel_height = pixel_height;
    font.char_count = glyph_count;
    font.chars = (CharacterTTF*)calloc(glyph_count, sizeof(CharacterTTF));
    for (u32 i = 0; i < glyph_count; i++)
    {
        CharacterTTF* c_ttf = &font.chars[i];
        stbtt_bakedchar bc = data[i];
        c_ttf->dimensions = v2f((f32)(bc.x1 - bc.x0), (f32)(bc.y1 - bc.y0));
        c_ttf->offset = v2f(bc.xoff, bc.yoff);
        c_ttf->x_advance = bc.xadvance;
        c_ttf->text_coords =
            v4f((f32)bc.x0 / width_atlas, (f32)bc.y0 / height_atlas,
                (f32)bc.x1 / width_atlas, (f32)bc.y1 / height_atlas);
    }
    *font_out = font;
    free(data);
    free(file.buffer);
}

internal void render_character(const char character, const CharacterTTF* c_ttf,
                               const float texture_index, const f32 scale,
                               const f32 line_height, const f32 start_x,
                               const V4 color, u32* new_lines,
                               f32* x_max_advance, u32* count, V2* pos,
                               SelectionCharacterArray* selection_chars,
                               VertexArray* array)
{
    if (character == '\n')
    {
        pos->y += line_height * scale;
        pos->x = start_x;
        *new_lines += 1;
        return;
    }
    else if (character == '\t')
    {
        const CharacterTTF* c = c_ttf + (' ' - 32);
        pos->x += c->x_advance * scale * 4;
    }
    if (closed_interval(0, (character - 32), 96))
    {
        const CharacterTTF* c = c_ttf + (character - 32);

        V2 size = v2_s_multi(c->dimensions, scale);
        V2 curr_pos = v2_add(*pos, v2_s_multi(c->offset, scale));
        AABB aabb = quad_co(array, curr_pos, size, color, c->text_coords,
                            texture_index);

        // TODO: do a check for every character is slow.
        if (selection_chars)
        {
            SelectionCharacter selection_char = {
                .character = character,
                .aabb = aabb,
            };
            array_push(selection_chars, selection_char);
        }
        pos->x += c->x_advance * scale;
        *x_max_advance = max(*x_max_advance, pos->x);
        *count += 1;
    }
}

u32 text_generation_color(const CharacterTTF* c_ttf, const char* text,
                          float texture_index, V2 pos, f32 scale,
                          f32 line_height, V4 color, u32* new_lines_count,
                          f32* x_advance,
                          SelectionCharacterArray* selection_chars,
                          VertexArray* array)
{
    u32 count = 0;
    f32 start_x = pos.x;
    u32 new_lines = 0;
    f32 x_max_advance = 0.0f;
    for (; *text; text++)
    {
        char current_char = *text;
        render_character(current_char, c_ttf, texture_index, scale, line_height,
                         start_x, color, &new_lines, &x_max_advance, &count,
                         &pos, selection_chars, array);
    }
    if (new_lines_count)
    {
        *new_lines_count = new_lines;
    }
    if (x_advance)
    {
        *x_advance = x_max_advance - start_x;
    }
    return count * 6;
}

u32 text_generation_colored_char(const CharacterTTF* c_ttf,
                                 const ColoredCharacterArray* text,
                                 float texture_index, V2 pos, f32 scale,
                                 f32 line_height, u32* new_lines_count,
                                 f32* x_advance,
                                 SelectionCharacterArray* selection_chars,
                                 VertexArray* array)
{
    u32 count = 0;
    f32 start_x = pos.x;
    u32 new_lines = 0;
    f32 x_max_advance = 0.0f;
    for (u32 i = 0; i < text->size; ++i)
    {
        ColoredCharacter* current = text->data + i;
        render_character(current->character, c_ttf, texture_index, scale,
                         line_height, start_x, current->color, &new_lines,
                         &x_max_advance, &count, &pos, selection_chars, array);
    }
    if (new_lines_count)
    {
        *new_lines_count = new_lines;
    }
    if (x_advance)
    {
        *x_advance = x_max_advance - start_x;
    }
    return count * 6;
}

f32 text_x_advance(const CharacterTTF* c_ttf, const char* text, u32 text_len,
                   f32 scale)
{
    f32 result = 0;
    for (u32 i = 0; i < text_len; ++i)
    {
        char current_char = text[i];
        if (closed_interval(0, (current_char - 32), 96))
        {
            const CharacterTTF* c = c_ttf + (current_char - 32);
            result += c->x_advance * scale;
        }
    }
    return result;
}

i32 text_check_length_within_boundary(const CharacterTTF* c_ttf,
                                      const char* text, u32 text_len, f32 scale,
                                      float boundary)
{
    f32 x_advance = 0;
    for (i32 i = 0; i < (i32)text_len; ++i)
    {
        char current_char = text[i];
        if (closed_interval(0, (current_char - 32), 96))
        {
            const CharacterTTF* c = c_ttf + (current_char - 32);
            x_advance += c->x_advance * scale;
            if (x_advance > boundary)
            {
                return i;
            }
        }
    }
    return -1;
}
