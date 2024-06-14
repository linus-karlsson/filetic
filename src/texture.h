#pragma once
#include "define.h"

typedef struct TextureProperties
{
    int width;
    int height;
    int channels;
    u8* bytes;
} TextureProperties;

void texture_load(const char* file_path, TextureProperties* texture_properties);
u32 texture_create(const TextureProperties* texture_properties, int internal_format, u32 format);
void texture_bind(u32 texture, int slot);
void texture_unbind(int slot);
void texture_delete(u32 texture);
