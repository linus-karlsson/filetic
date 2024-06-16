#include "metacgen/metacgen.h"
#include <time.h>
#include <stdio.h>

static double get_time_sec(void)
{
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    return now.tv_sec + (now.tv_nsec * 0.000000001);
}

int main(int argc, char** argv)
{
    // TODO: generate this file path through code (Platform specific)
    Mcgen_Context* ctx =
        mcgen_context_create("C:\\Users\\linus\\dev\\filetic\\");

    mcgen_append_type_file(ctx, "code_generation\\src\\array_gen.h",
                           "src\\array.h");
    mcgen_append_type_files(ctx, "code_generation\\src\\hash_table_gen.h",
                            "src\\hash_table.h", "src\\hash_table.c");

    //////////////// Arrays ////////////////////
    {
        char* types[] = { "char", "u32", "Vertex" };
        const uint32_t type_count = sizeof(types) / sizeof(types[0]);
        char* postfixs[] = { "Char", "U32", "Vertex" };
        const uint32_t postfix_count = sizeof(postfixs) / sizeof(postfixs[0]);
        mcgen_append_types_and_postfixs(ctx, "Array", types, type_count,
                                        postfixs, postfix_count);
    }

    //////////////// Hash Table ////////////////////
    {
        char* types[] = {
            "u64*,u64",
        };
        const uint32_t type_count = sizeof(types) / sizeof(types[0]);
        char* postfixs[] = { "UU64" };
        const uint32_t postfix_count = sizeof(postfixs) / sizeof(postfixs[0]);
        mcgen_link_names(ctx, "Cell", "HashTable", "hash_table_create");
        mcgen_append_types_and_postfixs(ctx, "Cell", types, type_count,
                                        postfixs, postfix_count);

        char* types2[] = {
            "u64*,u64,sizeof,value_cmp",
        };
        mcgen_link_names(ctx, "hash_table_insert", "hash_table_get", "hash_table_remove");
        mcgen_append_types_and_postfixs(ctx, "hash_table_insert", types2, type_count,
                                        postfixs, postfix_count);
    }

    mcgen_generate_code(ctx);
}
