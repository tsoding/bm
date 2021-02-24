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

    Chunk_List chunk = chunk_translate_file(&basm, sv_from_cstr(input_file_path));
    chunk_dump(stdout, chunk.begin, 0);
    arena_free(&basm.arena);

    return 0;
}
