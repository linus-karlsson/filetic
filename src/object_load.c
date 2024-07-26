#include "object_load.h"
#include "util.h"
#include "ftic_window.h"
#include <string.h>

#define GAP(x) (((x) == ' ') || ((x) == '\t'))

internal void init(u32 v, u32 vn, u32 vt, u32 f, ObjectLoad* object_load)
{
    array_create(&object_load->vertex_positions, v);
    array_create(&object_load->normals, vn);
    array_create(&object_load->texture_coordinates, vt);
    array_create(&object_load->indices, f);
}

internal void parse_sizes(FileAttrib* file, u32* v, u32* vt, u32* vn, u32* f)
{
    *v = 0;
    *vt = 0;
    *vn = 0;
    *f = 0;

    const char* delims = "\n\r ";
    CharArray line = { 0 };
    array_create(&line, 1000);
    while (!file_end_of_file(file))
    {
        file_line_read(file, false, &line);
        if (line.size < 1) continue;
        Token token = file_read_token(&line, delims, 3);
        if (token.start)
        {
            if (token.start[0] == 'v')
            {
                if (!strcmp(token.start, "v"))
                {
                    (*v)++;
                }
                else if (!strcmp(token.start, "vn"))
                {
                    (*vn)++;
                }
                else if (!strcmp(token.start, "vt"))
                {
                    (*vt)++;
                }
            }
            else if (!strcmp(token.start, "f"))
            {
                u32 current_count = 0;
                for (u32 offset = 2; offset < line.size; ++offset)
                {
                    if (GAP(line.data[offset]))
                    {
                        ++current_count;
                        while (GAP(line.data[offset]))
                        {
                            ++offset;
                        }
                    }
                }
                ++current_count;
                (*f) += (current_count - 2) * 3;
            }
        }
        line.size = 0;
    }
    array_free(&line);
}

internal V3 vec3f(const char* line)
{
    V3 vec = v3d();
    string_to_value(line, "%f %f %f", &vec.x, &vec.y, &vec.z);
    return vec;
}

internal V2 vec2f(const char* line)
{
    V2 vec = v2d();
    string_to_value(line, "%f %f", &vec.x, &vec.y);
    return vec;
}

internal void f_parse(ObjectLoad* object_load, char* line)
{
    u32 i0 = 0, i1 = 0, i2 = 0;

    char* current_pos = line;

    IndicesArray indices_array = { 0 };
    array_create(&indices_array, 100);
    while (*current_pos)
    {
        while (GAP(*current_pos))
        {
            current_pos++;
        }
        Indices indices = { 0 };
        if (string_to_value(current_pos, "%u/%u/%u", &i0, &i1, &i2) == 3)
        {
            indices.vertex_index = i0 - 1;
            indices.texture_index = i1 - 1;
            indices.normal_index = i2 - 1;
            array_push(&indices_array, indices);
        }
        else if (string_to_value(current_pos, "%u//%u", &i0, &i1) == 2)
        {
            indices.vertex_index = i0 - 1;
            indices.normal_index = i1 - 1;
            array_push(&indices_array, indices);
        }
        else if (string_to_value(current_pos, "%u/%u", &i0, &i1) == 2)
        {
            indices.vertex_index = i0 - 1;
            indices.texture_index = i1 - 1;
            array_push(&indices_array, indices);
        }
        else if (string_to_value(current_pos, "%u", &i0) == 1)
        {
            indices.vertex_index = i0 - 1;
            array_push(&indices_array, indices);
        }
        while (!GAP(*current_pos) && *current_pos != '\0')
        {
            current_pos++;
        }
    }

    for (u32 i = 0; i < indices_array.size - 2; i++)
    {
        u32 h = 0;
        for (u32 j = 0; j < 3; j++)
        {
            if (j > 0 && i > 0) h = i;
            array_push(&object_load->indices, indices_array.data[h + j]);
        }
    }
    array_free(&indices_array);
}

internal void parse_buffer(ObjectLoad* object_load, FileAttrib* file)
{
#if 0
    u32 v = 0, vt = 0, vn = 0, f = 0;
    parse_sizes(file, &v, &vt, &vn, &f);
#else
    u32 v = 1000, vt = 1000, vn = 1000, f = 1000;
#endif

    init(v, vn, vt, f, object_load);

    file->current_pos = 0;

    const char* delims = "\n\r ";
    CharArray line = { 0 };
    array_create(&line, 1000);
    while (!file_end_of_file(file))
    {
        file_line_read(file, true, &line);
        if (file->size < 1) continue;
        Token token = file_read_token(&line, delims, 3);
        if (token.start)
        {
            if (token.start[0] == 'v')
            {
                if (!strcmp(token.start, "v"))
                {
                    array_push(&object_load->vertex_positions, vec3f(line.data + 2));
                }
                else if (!strcmp(token.start, "vn"))
                {
                    array_push(&object_load->normals, vec3f(line.data + 3));
                }
                else if (!strcmp(token.start, "vt"))
                {
                    array_push(&object_load->texture_coordinates,
                               vec2f(line.data + 3));
                }
            }
            else if (!strcmp(token.start, "f"))
            {
                f_parse(object_load, line.data + 2);
            }
        }
        line.size = 0;
    }
}

void object_load_model(ObjectLoad* object_load, const char* model_path)
{
    FileAttrib file = file_read_full_path(model_path);
    if(file.buffer)
    {
        f64 start = window_get_time();
        parse_buffer(object_load, &file);
        log_f32(" ", (f32)(window_get_time() - start));
        free(file.buffer);
    }
}

void object_load_free(ObjectLoad* object_load)
{
    array_free(&object_load->vertex_positions);
    array_free(&object_load->normals);
    array_free(&object_load->texture_coordinates);
    array_free(&object_load->indices);
}
