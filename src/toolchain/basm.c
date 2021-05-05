#include "./basm.h"
#include "./bm.h"
#include "./path.h"

typedef enum {
    BASM_OUTPUT_BM = 0,
    BASM_OUTPUT_NASM,
    COUNT_BASM_OUTPUTS
} Basm_Output_Format;

static String_View output_format_file_ext(Basm_Output_Format format)
{
    switch (format) {
    case BASM_OUTPUT_BM:
        return SV(".bm");
    case BASM_OUTPUT_NASM:
        return SV(".asm");
    case COUNT_BASM_OUTPUTS:
    default:
        assert(false && "output_format_file_ext: unreachable");
        exit(1);
    }
}

static bool output_format_by_name(const char *name, Basm_Output_Format *format)
{
    static_assert(
        COUNT_BASM_OUTPUTS == 2,
        "Please add a condition branch for a new output "
        "and increment the counter above");
    if (strcmp(name, "bm") == 0) {
        *format = BASM_OUTPUT_BM;
        return true;
    } else if (strcmp(name, "nasm") == 0) {
        *format = BASM_OUTPUT_NASM;
        return true;
    } else {
        return false;
    }
}

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
    fprintf(stream, "    -I <include/path/>    Add include path\n");
    fprintf(stream, "    -o <output.bm>        Provide output path\n");
    fprintf(stream, "    -f <bm|nasm>          Output format. Default is bm\n");
    fprintf(stream, "    -verify               Verify the bytecode instructions after the translation\n");
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
    Basm_Output_Format output_format = BASM_OUTPUT_BM;
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
        } else if (strcmp(flag, "-f") == 0) {
            const char *name = get_flag_value(&argc, &argv, flag, program);
            if (!output_format_by_name(name, &output_format)) {
                usage(stderr, program);
                fprintf(stderr, "ERROR: unknown output format `%s`\n", name);
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
                      output_format_file_ext(output_format));
        output_file_path = arena_sv_to_cstr(&basm.arena, output_file_path_sv);
    }

    basm_translate_root_source_file(&basm, sv_from_cstr(input_file_path));

    if (verify) {
        basm_verify_program(&basm);
    }

    switch (output_format) {
    case BASM_OUTPUT_BM: {
        basm_save_to_file_as_bm(&basm, output_file_path);
    }
    break;

    case BASM_OUTPUT_NASM: {
        basm_save_to_file_as_nasm(&basm, output_file_path);
    }
    break;

    case COUNT_BASM_OUTPUTS:
    default:
        assert(false && "basm: unreachable");
        exit(1);
    }

    arena_free(&basm.arena);

    return 0;
}
