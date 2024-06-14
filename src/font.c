#include "font.h"
#include "opengl_util.h"
#include <stb/stb_truetype.h>

void init_ttf_atlas(i32 width_atlas, i32 height_atlas, f32 pixel_height,
                    u32 glyph_count, u32 glyph_offset,
                    const char* font_file_path, u8* bitmap, FontTTF* font_out)
{
    FileAttrib file = read_file(font_file_path);
    stbtt_bakedchar* data =
        (stbtt_bakedchar*)calloc(glyph_count, sizeof(stbtt_bakedchar));
    stbtt_BakeFontBitmap(file.buffer, 0, pixel_height, bitmap, 512, 512,
                         glyph_offset, glyph_count, data);

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

u32 text_generation(const CharacterTTF* c_ttf, const char* text, float texture_index,
             V3 pos, f32 scale, f32 line_height, u32* new_lines_count,
             f32* x_advance, VertexArray* array)
{
    u32 count = 0;
    f32 start_x = pos.x;
    u32 new_lines = 0;
    for (; *text; text++)
    {
        char current_char = *text;
        if (current_char == '\n')
        {
            pos.y += line_height * scale;
            pos.x = start_x;
            new_lines++;
            continue;
        }
        if (closed_interval(0, (current_char - 32), 96))
        {
            const CharacterTTF* c = c_ttf + (current_char - 32);

            V2 size = v2_s_multi(c->dimensions, scale);
            V3 curr_pos = v3_add(pos, v3_v2(v2_s_multi(c->offset, scale)));
            quad_co(array, curr_pos, size, v4i(1.0f), c->text_coords,
                    texture_index);

            pos.x += c->x_advance * scale;
            count++;
        }
    }
    if (new_lines_count)
    {
        *new_lines_count = new_lines;
    }
    if (x_advance)
    {
        *x_advance = pos.x - start_x;
    }
    return count;
}
