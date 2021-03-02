#include "./basm.h"
#include "./bm.h"
#include "./path.h"

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
    fprintf(stream, "Usage: %s [OPTIONS] <input.basm>\n", program);
    fprintf(stream, "OPTIONS:\n");
    fprintf(stream, "    -g                    Generate symbol table\n");
    fprintf(stream, "    -I <include/path/>    Add include path\n");
    fprintf(stream, "    -o <output.bm>        Provide output path\n");
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

    // TODO: separate flag for basm to use new AST translation mechanism

    while (argc > 0) {
        const char *flag = shift(&argc, &argv);

        if (strcmp(flag, "-o") == 0) {
            output_file_path = get_flag_value(&argc, &argv, flag, program);
        } else if (strcmp(flag, "-g") == 0) {
            have_symbol_table = true;
        } else if (strcmp(flag, "-I") == 0) {
            basm_push_include_path(&basm, sv_from_cstr(get_flag_value(&argc, &argv, flag, program)));
        } else {
            if (input_file_path != NULL) {
                usage(stderr, program);
                fprintf(stderr, "ERROR: input file is already provided as `%s`. Only a single input file is not supported\n", input_file_path);
                exit(1);
            }
            input_file_path = flag;
        }
    }

    if (input_file_path == NULL) {
        usage(stderr, program);
        fprintf(stderr, "ERROR: no input file is provided\n");
        exit(1);
    }

    if (output_file_path == NULL) {
        const String_View output_file_path_sv =
            SV_CONCAT(&basm.arena,
                      SV("./"),
                      file_name_of_path(input_file_path),
                      SV(".bm"));
        output_file_path = arena_sv_to_cstr(&basm.arena, output_file_path_sv);
    }

    basm_translate_source_file(&basm, sv_from_cstr(input_file_path));

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
