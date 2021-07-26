#include "./compiler.h"
#include "./bm.h"
#include "./path.h"
#include "./verifier.h"
#include "./target.h"

static void usage(FILE *stream, const char *program)
{
    fprintf(stream, "Usage: %s [OPTIONS] <input.basm>\n", program);
    fprintf(stream, "OPTIONS:\n");
    fprintf(stream, "    -I <include/path/>    Add include path\n");
    fprintf(stream, "    -o <output.bm>        Provide output path\n");
    fprintf(stream, "    -t <target>           Output target. Default is `bm`.\n");
    fprintf(stream, "                          Provide `list` to get the list of all available targets.\n");
    fprintf(stream, "    -verify               Verify the bytecode instructions after the translation.\n");
    fprintf(stream, "    -h                    Print this help to stdout\n");
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

    const char *program = shift(&argc, &argv);
    const char *input_file_path = NULL;
    const char *output_file_path = NULL;
    Target output_target = TARGET_BM;
    bool verify = false;

    while (argc > 0) {
        const char *flag = shift(&argc, &argv);

        if (strcmp(flag, "-o") == 0) {
            output_file_path = get_flag_value(&argc, &argv, flag, program);
        } else if (strcmp(flag, "-I") == 0) {
            basm_push_include_path(&basm, sv_from_cstr(get_flag_value(&argc, &argv, flag, program)));
        } else if (strcmp(flag, "-h") == 0) {
            usage(stdout, program);
            exit(0);
        } else if (strcmp(flag, "-t") == 0) {
            const char *name = get_flag_value(&argc, &argv, flag, program);

            if (strcmp(name, "list") == 0) {
                printf("Available targets:\n");
                for (Target target = 0; target < COUNT_TARGETS; ++target) {
                    printf("  %s\n", target_name(target));
                }
                exit(0);
            }

            if (!target_by_name(name, &output_target)) {
                usage(stderr, program);
                fprintf(stderr, "ERROR: unknown target: `%s`\n", name);
                exit(1);
            }
        } else if (strcmp(flag, "-verify") == 0) {
            verify = true;
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
                      sv_from_cstr(target_file_ext(output_target)));
        output_file_path = arena_sv_to_cstr(&basm.arena, output_file_path_sv);
    }

    basm_translate_root_source_file(&basm, sv_from_cstr(input_file_path));

    if (verify) {
        static Verifier verifier = {0};
        verifier_verify(&verifier, &basm);
    }

    basm_save_to_file_as_target(&basm, output_file_path, output_target);

    arena_free(&basm.arena);

    return 0;
}
