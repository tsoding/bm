#define BM_IMPLEMENTATION
#include "./bm.h"
#include <inttypes.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: ./debasm <input.bm>\n");
        fprintf(stderr, "ERROR: no input is provided\n");
        exit(1);
    }

    const char *input_file_path = argv[1];

    // NOTE: The structure might be quite big due its arena. Better allocate it in the static memory.
    static Bm bm = {0};
    bm_load_program_from_file(&bm, input_file_path);

    for (size_t i = 0; i < bm.externals_size; ++i) {
        printf("%%native %s\n", bm.externals[i].name);
    }

    printf("%%const MEMORY = \"");
    for (size_t i = 0; i < bm.expected_memory_size; ++i) {
        if (32 <= bm.memory[i] && bm.memory[i] < 127) {
            printf("%c", bm.memory[i]);
        } else {
            printf("\\x%02x", bm.memory[i]);
        }
    }
    printf("\"\n");
    printf("%%assert MEMORY == 0\n");

    for (Inst_Addr i = 0; i < bm.program_size; ++i) {
        if (i == bm.ip) {
            printf("%%entry main:\n");
        }

        Inst_Def inst_def = get_inst_def(bm.program[i].type);

        printf("    %s", inst_def.name);
        if (inst_def.has_operand) {
            if (inst_def.operand_type == TYPE_UNSIGNED_INT || inst_def.operand_type == TYPE_ANY) {
                printf(" %" PRIu64" ;; i64: %"PRIi64", f64: %lf, ptr: %p",
                       bm.program[i].operand.as_u64,
                       bm.program[i].operand.as_i64,
                       bm.program[i].operand.as_f64,
                       bm.program[i].operand.as_ptr);
            } else if (inst_def.operand_type == TYPE_NATIVE_ID) {
                assert(bm.program[i].operand.as_u64 < bm.externals_size);
                printf(" %s", bm.externals[bm.program[i].operand.as_u64].name);
            } else {
                printf(" %s(%" PRIu64") ;; i64: %"PRIi64", f64: %lf, ptr: %p",
                       type_name(inst_def.operand_type),
                       bm.program[i].operand.as_u64,
                       bm.program[i].operand.as_i64,
                       bm.program[i].operand.as_f64,
                       bm.program[i].operand.as_ptr);
            }
        }
        printf("\n");
    }

    return 0;
}
