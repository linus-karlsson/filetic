#pragma once
#include "define.h"
#include "ftic_guid.h"

typedef struct TextureProperties
{
    int width;
    int height;
    int channels;
    u8* bytes;
} TextureProperties;

typedef struct IdTextureProperties{
    FticGUID id;
    TextureProperties texture_properties;
}IdTextureProperties;

typedef struct IdTexturePropertiesArray
{
    u32 size;
    u32 capacity;
    IdTextureProperties* data;
}IdTexturePropertiesArray;

typedef struct SafeIdTexturePropertiesArray
{
    IdTexturePropertiesArray array;
    FTicMutex mutex;
} SafeIdTexturePropertiesArray;

void texture_load(const char* file_path, TextureProperties* texture_properties);
void texture_resize(TextureProperties* texture_properties, int box_width, int box_height);
u32 texture_create(const TextureProperties* texture_properties, int internal_format, u32 format);
void texture_bind(u32 texture, int slot);
void texture_unbind(int slot);
void texture_delete(u32 texture);
