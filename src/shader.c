#include "shader.h"
#include "logging.h"
#include "util.h"
#include <glad/glad.h>

internal char* get_shader_error_prefix(u32 type)
{
    const char* prefix = "Failed to compile";
    const char* what_file =
        type == GL_VERTEX_SHADER ? " vertex shader: " : " fragment shader: ";
    size_t prefix_message_length = 0;
    char* prefix_message =
        concatinate(prefix, strlen(prefix), what_file, strlen(what_file), 0, 0,
                    &prefix_message_length);
    return prefix_message;
}

u32 shader_compile(u32 type, const u8* source)
{
    u32 id = glCreateShader(type);
    glShaderSource(id, 1, (const char**)&source, NULL);
    glCompileShader(id);

    int result = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        char* prefix_message = get_shader_error_prefix(type);

        int length = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)calloc(length, sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);

        char* error_message =
            concatinate(prefix_message, strlen(prefix_message), message, length,
                        0, 0, NULL);

        log_error_message(error_message, strlen(error_message));

        free(prefix_message);
        free(message);
        free(error_message);
        glDeleteShader(id);
        return 0;
    }

    return id;
}

u32 shader_create(const char* vertex_file_path, const char* fragment_file_path)
{
    FileAttrib vertex_file = read_file(vertex_file_path);
    FileAttrib fragment_file = read_file(fragment_file_path);
    u32 vertex_shader = shader_compile(GL_VERTEX_SHADER, vertex_file.buffer);
    u32 fragemnt_shader =
        shader_compile(GL_FRAGMENT_SHADER, fragment_file.buffer);
    free(vertex_file.buffer);
    free(fragment_file.buffer);

    u32 program = glCreateProgram();

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragemnt_shader);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragemnt_shader);

    return program;
}

void shader_bind(u32 shader)
{
    glUseProgram(shader);
}

void shader_unbind()
{
    glUseProgram(0);
}

void shader_destroy(u32 shader)
{
    glDeleteProgram(shader);
}

int shader_get_uniform_location(u32 shader, const char* name)
{
    int location = glGetUniformLocation(shader, name);
    ftic_assert(location != -1);
    return location;
}

void shader_set_uniform_matrix4fv(u32 shader, int uniform_location, M4 value)
{
    glUniformMatrix4fv(uniform_location, 1, GL_FALSE, &value.data[0][0]);
}


ShaderProperties shader_create_properties(u32 shader, const char* projection,
                                          const char* view, const char* model)
{
    ShaderProperties shader_properties = {
        .shader = shader,
        .projection_location = shader_get_uniform_location(shader, projection),
        .view_location = shader_get_uniform_location(shader, view),
        .model_location = shader_get_uniform_location(shader, model),
    };
    return shader_properties;
}

void shader_set_mvp(const ShaderProperties* shader_properties, const MVP* mvp)
{
    shader_set_uniform_matrix4fv(shader_properties->shader,
                                 shader_properties->projection_location,
                                 mvp->projection);
    shader_set_uniform_matrix4fv(shader_properties->shader,
                                 shader_properties->view_location, mvp->view);
    shader_set_uniform_matrix4fv(shader_properties->shader,
                                 shader_properties->model_location, mvp->model);
}
