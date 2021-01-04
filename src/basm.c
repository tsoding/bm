#define BM_IMPLEMENTATION
#include "./bm.h"

Bm bm = {0};
Basm basm = {0};

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
    fprintf(stream, "Usage: %s <input.basm> <output.bm>\n", program);
}

int main(int argc, char **argv)
{
    const char *program = shift(&argc, &argv);

    if (argc == 0) {
        usage(stderr, program);
        fprintf(stderr, "ERROR: expected input\n");
        exit(1);
    }
    const char *input_file_path = shift(&argc, &argv);

    if (argc == 0) {
        usage(stderr, program);
        fprintf(stderr, "ERROR: expected output\n");
        exit(1);
    }
    const char *output_file_path = shift(&argc, &argv);

    String_View source = sv_slurp_file(input_file_path);

    bm_translate_source(source, &bm, &basm, input_file_path);
    bm_save_program_to_file(&bm, output_file_path);

    return 0;
}
