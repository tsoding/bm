#define BM_IMPLEMENTATION
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
    fprintf(stream, "Usage: %s [OPTIONS] <input.bm>\n", program);
    fprintf(stream, "OPTIONS:\n");
    fprintf(stream, "    -l <limit>      Limit the amount of steps of the emulation.\n");
    fprintf(stream, "                    -1 means not limitation\n");
    fprintf(stream, "    -h              Print this help to stdout\n");
}

int main(int argc, char **argv)
{
    // NOTE: The structure might be quite big due its arena. Better allocate it in the static memory.
    static Bm bm = {0};

    const char *program = shift(&argc, &argv);
    const char *input_file_path = NULL;
    int limit = -1;

    while (argc > 0) {
        const char *flag = shift(&argc, &argv);

        if (strcmp(flag, "-l") == 0) {
            if (argc == 0) {
                usage(stderr, program);
                fprintf(stderr, "ERROR: No argument is provided for flag `%s`\n", flag);
                exit(1);
            }

            limit = atoi(shift(&argc, &argv));
        } else if (strcmp(flag, "-h") == 0) {
            usage(stdout, program);
            exit(0);
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
        fprintf(stderr, "ERROR: input was not provided\n");
        exit(1);
    }

    bm_load_program_from_file(&bm, input_file_path);
    bm_load_standard_natives(&bm);

    Err err = bm_execute_program(&bm, limit);

    if (err != ERR_OK) {
        fprintf(stderr, "ERROR: %s\n", err_as_cstr(err));
        return 1;
    }

    return 0;
}
