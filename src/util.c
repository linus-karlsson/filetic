#include "util.h"
#include "logging.h"
#include <stdio.h>

FileAttrib read_file(const char* file_path)
{
    FILE* file = fopen(file_path, "rb");

    if (!file)
    {
        log_file_error(file_path);
        ftic_assert(false);
    }

    FileAttrib file_attrib = { 0 };
    fseek(file, 0, SEEK_END);
    file_attrib.size = (u64)ftell(file);
    rewind(file);

    file_attrib.buffer = (u8*)calloc(file_attrib.size + 1, sizeof(u8));

    if (fread(file_attrib.buffer, 1, file_attrib.size, file) !=
        file_attrib.size)
    {
        log_last_error();
        ftic_assert(false);
    }
    fclose(file);
    return file_attrib;
}

f32 lerp_f32(const f32 a, const f32 b, const f32 t)
{
    return a + (t * (b - a));
}

V4 v4_lerp(V4 v1, V4 v2, f32 t)
{
    return v4_add(v1, v4_s_multi(v4_sub(v2, v1), t));
}
