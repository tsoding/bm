#define BM_IMPLEMENTATION
#include "./bm.h"
#include <limits.h>

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
    fprintf(stream, "Usage: %s [-g] <input.basm> <output.bm>\n", program);
}

int main(int argc, char **argv)
{
    int have_symbol_table = 0;
    const char *program = shift(&argc, &argv);

    if (argc == 0) {
        usage(stderr, program);
        fprintf(stderr, "ERROR: expected input\n");
        exit(1);
    }
    const char *input_file_path = shift(&argc, &argv);

    if (!strcmp(input_file_path, "-g")) {
        have_symbol_table = 1;
        input_file_path = shift(&argc, &argv);
    }

    if (argc == 0) {
        usage(stderr, program);
        fprintf(stderr, "ERROR: expected output\n");
        exit(1);
    }
    const char *output_file_path = shift(&argc, &argv);

    basm_translate_source(&basm, sv_from_cstr(input_file_path), 0);
    basm_save_to_file(&basm, output_file_path);

    if (have_symbol_table) {
        char sym_file_name[PATH_MAX];

        int output_file_path_len = strlen(output_file_path);

        memcpy(sym_file_name, output_file_path, output_file_path_len);
        memcpy(sym_file_name + output_file_path_len,
               ".sym", 5);
        FILE *symbol_file = fopen(sym_file_name, "w");
        if (!symbol_file)
        {
            fprintf(stderr, "ERROR: Unable to open symbol table file\n");
            return EXIT_FAILURE;
        }

        for (size_t i = 0; i < basm.labels_size; ++i) {
            fprintf(symbol_file, "%"PRIu64"\t%.*s\n",
                    basm.labels[i].word.as_u64,
                    (int)basm.labels[i].name.count,
                    basm.labels[i].name.data);
        }
        fclose(symbol_file);
    }

    return 0;
}
