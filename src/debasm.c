#define BM_IMPLEMENTATION
#include "./bm.h"

Bm bm = {0};

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: ./debasm <input.bm>\n");
        fprintf(stderr, "ERROR: no input is provided\n");
        exit(1);
    }

    const char *input_file_path = argv[1];

    bm_load_program_from_file(&bm, input_file_path);

    for (Inst_Addr i = 0; i < bm.program_size; ++i) {
        printf("%s", inst_name(bm.program[i].type));
        if (inst_has_operand(bm.program[i].type)) {
            printf(" %lld", bm.program[i].operand.as_i64);
        }
        printf("\n");
    }

    return 0;
}
