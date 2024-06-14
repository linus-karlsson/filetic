#pragma once
#include "define.h"
#include "math/ftic_math.h"

typedef struct ShaderProperties
{
    u32 shader;
    int projection_location;
    int view_location;
    int model_location;
} ShaderProperties;

typedef struct MVP
{
    M4 projection;
    M4 view;
    M4 model;
} MVP;

u32 shader_compile(u32 type, const u8* source);
u32 shader_create(const char* vertex_file_path, const char* fragment_file_path);
void shader_bind(u32 shader);
void shader_unbind();
void shader_destroy(u32 shader);
int shader_get_uniform_location(u32 shader, const char* name);
void shader_set_uniform_matrix4fv(u32 shader, int uniform_location, M4 value);
ShaderProperties shader_create_properties(u32 shader, const char* projection, const char* view, const char* model);
void shader_set_mvp(const ShaderProperties* shader_properties, const MVP* mvp);
