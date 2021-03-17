#define BM_IMPLEMENTATION
#include "./bm.h"
#include "./arena.h"
#include "./basm.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "USAGE: expr2dot <expression>\n");
        fprintf(stderr, "ERROR: expression is not provided\n");
        exit(1);
    }

    Arena arena = {0};
    // TODO(#198): expr2dot does not have a proper location reporting on errors
    File_Location location = {0};

    dump_expr_as_dot(
        stdout,
        parse_expr_from_sv(
            &arena,
            sv_from_cstr(argv[1]),
            location));

    arena_free(&arena);

    return 0;
}
