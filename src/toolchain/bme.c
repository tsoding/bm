#define BM_IMPLEMENTATION
#include "./bm.h"
#include "./native_loader.h"

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
    fprintf(stream, "    -n <.so|.DLL>   File path to a dynamic library to load native\n");
    fprintf(stream, "                    functions from. You can provide several of them.\n");
    fprintf(stream, "    -h              Print this help to stdout\n");
}

int main(int argc, char **argv)
{
    // NOTE: The structure might be quite big due its arena. Better allocate it in the static memory.
    static Arena arena = {0};
    static Bm bm = {0};
    static Native_Loader native_loader = {0};

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
        } else if (strcmp(flag, "-n") == 0) {
            if (argc == 0) {
                usage(stderr, program);
                fprintf(stderr, "ERROR: No argument is provide for flag `%s`\n", flag);
                exit(1);
            }

            const char *object_path = shift(&argc, &argv);
            native_loader_add_object(&native_loader, object_path);
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

    for (size_t i = 0; i < bm.externals_size; ++i) {
        if (strcmp(bm.externals[i].name, "write") == 0) {
            bm_push_native(&bm, native_write);
        } else {
            Bm_Native native = native_loader_find_function(&native_loader, &arena, bm.externals[i].name);
            if (native == NULL) {
                fprintf(stderr, "ERROR: could not find external native function `%s`. Make sure you attached all the necessary dynamic libraries via the `-n` flag.\n", bm.externals[i].name);
                exit(1);
            }

            bm_push_native(&bm, native);
        }
    }

    Err err = bm_execute_program(&bm, limit);

    if (err != ERR_OK) {
        fprintf(stderr, "ERROR: %s\n", err_as_cstr(err));
        return 1;
    }

    native_loader_unload_all(&native_loader);

    return 0;
}
