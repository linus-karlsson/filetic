#include "texture.h"
#include <glad/glad.h>
#include <stb/stb_image.h>

void texture_load(const char* file_path, TextureProperties* texture_properties)
{
    stbi_set_flip_vertically_on_load(1);
    texture_properties->bytes = (u8*)stbi_load(
        file_path, &texture_properties->width, &texture_properties->height,
        &texture_properties->channels, 4);
}

u32 texture_create(const TextureProperties* texture_properties,
                   int internal_format, u32 format)
{
    uint32_t texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, texture_properties->width,
                 texture_properties->height, 0, format, GL_UNSIGNED_BYTE,
                 texture_properties->bytes);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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

