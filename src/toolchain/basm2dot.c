#include <stdlib.h>
#include <stdio.h>

#include "./arena.h"
#include "./linizer.h"
#include "./statement.h"

static void usage(FILE *stream)
{
    fprintf(stream, "Usage: ./basm2dot <input.basm>\n");
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        usage(stderr);
        fprintf(stderr, "ERROR: no input is provided\n");
        exit(1);
    }

    const char *input_file_path = argv[1];

    Arena arena = {0};
    Linizer linizer = {0};
    if (!linizer_from_file(&linizer, &arena, sv_from_cstr(input_file_path))) {
        fprintf(stderr, "ERROR: could not read file `%s`: %s",
                input_file_path, strerror(errno));
        exit(1);
    }

    Statement statement = {0};
    statement.kind = STATEMENT_KIND_BLOCK;
    statement.value.as_block = parse_block_from_lines(&arena, &linizer);

    dump_statement_as_dot(stdout, statement);

    return 0;
}
