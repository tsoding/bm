#define BM_IMPLEMENTATION
#include "./basm.h"
#include "./bm.h"

static char *shift(int *argc, char ***argv)
{
    assert(*argc > 0);
    char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

static void usage(FILE *stream, const char *program)
{
    fprintf(stream, "Usage: %s [-g] [-I <include-path>...] -i <input.basm> -o <output.bm>\n", program);
}

static char *get_flag_value(int *argc, char ***argv,
                            const char *flag,
                            const char *program)
{
    if (*argc == 0) {
        usage(stderr, program);
        fprintf(stderr, "ERROR: no value provided for flag `%s`\n", flag);
        exit(1);
    }

    return shift(argc, argv);
}

int main(int argc, char **argv)
{
    // NOTE: The structure might be quite big due its arena. Better allocate it in the static memory.
    static Basm basm = {0};

    bool have_symbol_table = false;
    const char *program = shift(&argc, &argv);
    const char *input_file_path = NULL;
    const char *output_file_path = NULL;

    while (argc > 0) {
        const char *flag = shift(&argc, &argv);

        if (strcmp(flag, "-i") == 0) {
            input_file_path = get_flag_value(&argc, &argv, flag, program);
        } else if (strcmp(flag, "-o") == 0) {
            output_file_path = get_flag_value(&argc, &argv, flag, program);
        } else if (strcmp(flag, "-g") == 0) {
            have_symbol_table = true;
        } else if (strcmp(flag, "-I") == 0) {
            basm_push_include_path(&basm, sv_from_cstr(get_flag_value(&argc, &argv, flag, program)));
        } else {
            usage(stderr, program);
            fprintf(stderr, "ERROR: unknown flag `%s`\n", flag);
            exit(1);
        }
    }

    basm_translate_source(&basm, sv_from_cstr(input_file_path));

    if (!basm.has_entry) {
        fprintf(stderr, "%s: ERROR: entry point for a BM program is not provided. Use translation directive %%entry to provide the entry point.\n", input_file_path);
        fprintf(stderr, "  main:\n");
        fprintf(stderr, "     push 69\n");
        fprintf(stderr, "     halt\n");
        fprintf(stderr, "  %%entry main\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "You can also mark an existing label as the entry point like so:\n");
        fprintf(stderr, "  %%entry main:\n");
        fprintf(stderr, "     push 69\n");
        fprintf(stderr, "     halt\n");
        exit(1);
    }

    basm_save_to_file(&basm, output_file_path);

    if (have_symbol_table) {
        const char *sym_file_name = CSTR_CONCAT(&basm.arena, output_file_path, ".sym");

        FILE *symbol_file = fopen(sym_file_name, "w");
        if (!symbol_file) {
            fprintf(stderr, "ERROR: Unable to open symbol table file\n");
            return EXIT_FAILURE;
        }

        /*
         * Note: This will dump out *ALL* symbols, no matter whether
         * they are jump labels or not. However, since the
         * preprocessor runs before the jump mark resolution, all the
         * labels are allocated in a way that enables us to just
         * overwrite prerocessor labels with a value equal to the
         * address of a jump label.
         *
         */
        for (size_t i = 0; i < basm.bindings_size; ++i) {
            fprintf(symbol_file, "%"PRIu64"\t%u\t"SV_Fmt"\n",
                    basm.bindings[i].value.as_u64,
                    basm.bindings[i].kind,
                    SV_Arg(basm.bindings[i].name));
        }
        fclose(symbol_file);
    }

    arena_free(&basm.arena);

    return 0;
}
