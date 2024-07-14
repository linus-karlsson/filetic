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

    mcgen_append_type_files(ctx, "code_generation\\src\\set_gen.h",
                            "src\\set.h", "src\\set.c");

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
            "u64,u64",
            "char*,u32",
            "FticGUID,char*",
        };
        const uint32_t type_count = sizeof(types) / sizeof(types[0]);
        char* postfixs[] = { "UU64", "CharU32", "Guid" };
        const uint32_t postfix_count = sizeof(postfixs) / sizeof(postfixs[0]);
        mcgen_link_names(ctx, "Cell", "HashTable");
        mcgen_append_types_and_postfixs(ctx, "Cell", types, type_count,
                                        postfixs, postfix_count);

        char* postfixs_functions[] = { "_uu64", "_char_u32", "_guid" };
        mcgen_append_types_and_postfixs(ctx, "hash_table_create", types, type_count,
                                        postfixs_functions, postfix_count);
        mcgen_append_types_and_postfixs(ctx, "hash_table_clear", types, type_count,
                                        postfixs_functions, postfix_count);

        char* types2[] = {
            "u64,u64,sizeof,value_cmp",
            "char*,u32,strlen,strcmp",
            "FticGUID,char*,sizeof,guid_compare",
        };
        mcgen_link_names(ctx, "hash_table_insert", "hash_table_get", "hash_table_remove");
        mcgen_append_types_and_postfixs(ctx, "hash_table_insert", types2, type_count,
                                        postfixs_functions, postfix_count);
    }

    //////////////// Set ////////////////////
    {
        char* types[] = {
            "u64",
            "char*",
            "FticGUID",
        };
        const uint32_t type_count = sizeof(types) / sizeof(types[0]);
        char* postfixs[] = { "U64", "CharPtr", "Guid" };
        const uint32_t postfix_count = sizeof(postfixs) / sizeof(postfixs[0]);
        mcgen_link_names(ctx, "SetCell", "Set");
        mcgen_append_types_and_postfixs(ctx, "SetCell", types, type_count,
                                        postfixs, postfix_count);

        char* postfixs_functions[] = { "_u64", "_char_ptr", "_guid" };
        mcgen_append_types_and_postfixs(ctx, "set_create", types, type_count,
                                        postfixs_functions, postfix_count);
        mcgen_append_types_and_postfixs(ctx, "set_clear", types, type_count,
                                        postfixs_functions, postfix_count);

        char* types2[] = {
            "u64,sizeof,value_cmp",
            "char*,strlen,strcmp",
            "FticGUID,sizeof,guid_compare",
        };
        mcgen_link_names(ctx, "set_insert", "set_contains", "set_remove");
        mcgen_append_types_and_postfixs(ctx, "set_insert", types2, type_count,
                                        postfixs_functions, postfix_count);
    }

    mcgen_generate_code(ctx);
}
