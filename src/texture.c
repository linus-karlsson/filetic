#include "texture.h"
#include <glad/glad.h>
#include <stb/stb_image.h>
#include <stb/stb_image_resize2.h>

void texture_load(const char* file_path, TextureProperties* texture_properties)
{
    // stbi_set_flip_vertically_on_load(1);
    texture_properties->bytes = (u8*)stbi_load(
        file_path, &texture_properties->width, &texture_properties->height,
        &texture_properties->channels, 4);
    texture_properties->channels = 4;
}

void texture_scale_down(i32 width, i32 height, i32* new_width, i32* new_height)
{
    const f32 aspect_ratio = (f32)width / height;
    if (width < height)
    {
        *new_width = (int)((*new_height) * aspect_ratio);
    }
    else
    {
        *new_height = (int)((*new_width) / aspect_ratio);
    }
}

void texture_resize(TextureProperties* texture_properties, int box_width,
                    int box_height)
{

    TextureProperties result = { 0 };

    i32 new_width = box_width;
    i32 new_height = box_height;
    texture_scale_down(texture_properties->width, texture_properties->height,
                       &new_width, &new_height);

    result.bytes =
        (u8*)malloc(new_width * new_height * texture_properties->channels);

    stbir_resize_uint8_linear(
        texture_properties->bytes, texture_properties->width,
        texture_properties->height, 0, result.bytes, new_width, new_height, 0,
        texture_properties->channels);

    result.width = new_width;
    result.height = new_height;
    result.channels = texture_properties->channels;

    free(texture_properties->bytes);
    *texture_properties = result;
}

u32 texture_create(const TextureProperties* texture_properties,
                   int internal_format, u32 format, int param)
{
    uint32_t texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, texture_properties->width,
                 texture_properties->height, 0, format, GL_UNSIGNED_BYTE,
                 texture_properties->bytes);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, param);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, param);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return texture;
}

void texture_bind(u32 texture, int slot)
{
    // glActiveTexture(GL_TEXTURE0 + slot);
    // glBindTexture(GL_TEXTURE_2D, texture);
    glBindTextureUnit(slot, texture);
}

void texture_unbind(int slot)
{
    glBindTextureUnit(slot, 0);
}

void texture_delete(u32 texture)
{
    glDeleteTextures(1, &texture);
}

