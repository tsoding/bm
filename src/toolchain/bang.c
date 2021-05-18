#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "./arena.h"
#include "./path.h"
#include "./tokenizer.h"

static void usage(FILE *stream, const char *program)
{
    fprintf(stream, "Usage: %s [OPTIONS] <input.bang>\n", program);
    fprintf(stream, "OPTIONS:\n");
    fprintf(stream, "    -o <output>       Provide output path\n");
    fprintf(stream, "    -h                Print this help to stdout\n");
}

static char *shift(int *argc, char ***argv)
{
    assert(*argc > 0);
    char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

int main(int argc, char **argv)
{
    Arena arena = {0};

    const char * const program = shift(&argc, &argv);
    const char *input_file_path = NULL;
    const char *output_file_path = NULL;

    while (argc > 0) {
        const char *flag = shift(&argc, &argv);

        if (strcmp(flag, "-o") == 0) {
            if (argc <= 0) {
                usage(stderr, program);
                fprintf(stderr, "ERROR: no value is provided for flag `%s`\n", flag);
                exit(1);
            }

            output_file_path = shift(&argc, &argv);
        } else if (strcmp(flag, "-h") == 0) {
            usage(stdout, program);
            exit(0);
        } else {
            input_file_path = flag;
        }
    }

    if (input_file_path == NULL) {
        usage(stderr, program);
        fprintf(stderr, "ERROR: no input file path was provided\n");
        exit(1);
    }

    if (output_file_path == NULL) {
        const String_View output_file_path_sv =
            SV_CONCAT(&arena, SV("./"), file_name_of_path(input_file_path), SV(".bm"));
        output_file_path = arena_sv_to_cstr(&arena, output_file_path_sv);
    }

    String_View content = {0};
    if (arena_slurp_file(&arena, sv_from_cstr(input_file_path), &content) < 0) {
        fprintf(stderr, "ERROR: could not read file `%s`: %s",
                input_file_path,
                strerror(errno));
        exit(1);
    }

    Tokenizer tokenizer = tokenizer_from_sv(content);
    Token token = {0};
    File_Location dummy_fl = file_location(sv_from_cstr(input_file_path), 0);

    while (tokenizer_next(&tokenizer, &token, dummy_fl)) {
        printf("%s -> \""SV_Fmt"\"\n",
               token_kind_name(token.kind),
               SV_Arg(token.text));
    }

    arena_free(&arena);

    return 0;
}
