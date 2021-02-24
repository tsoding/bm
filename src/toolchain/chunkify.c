#include "./chunk.h"


int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "ERROR: no input is provided\n");
        exit(1);
    }

    const char *input_file_path = argv[1];

    // NOTE: The structure might be quite big due its arena. Better allocate it in the static memory.
    static Basm basm = {0};
    basm_push_include_path(&basm, sv_from_cstr("./examples/stdlib/"));

    Linizer linizer = {0};
    if (!linizer_from_file(&linizer, &basm.arena, sv_from_cstr(input_file_path))) {
        fprintf(stderr, "ERROR: Could not read file `%s`\n", input_file_path);
        exit(1);
    }

    Line line = {0};
    while (linizer_next(&linizer, &line)) {
        line_dump(stdout, &line);
    }

    // Chunk *chunk = chunk_translate_file(&basm, sv_from_cstr(input_file_path));

    // for (size_t i = 0; i < basm.bindings_size; ++i) {
    //     printf(FL_Fmt": BINDING: "SV_Fmt"\n",
    //            FL_Arg(basm.bindings[i].location),
    //            SV_Arg(basm.bindings[i].name));
    //     dump_expr(stdout, basm.bindings[i].expr, 1);
    // }

    // chunk_dump(stdout, chunk, 0);

    arena_free(&basm.arena);

    return 0;
}
