#include "basm.h"

#define LOAD_SP(output)                                 \
    do {                                                \
        fprintf(output, "    // LOAD SP\n");            \
        fprintf(output, "    ldr x9, =stack_top\n");    \
        fprintf(output, "    ldr w0, [x9]\n");          \
    } while(0)

#define STORE_SP(output)                                        \
    do {                                                        \
        fprintf(output, "    // STORE_SP\n");                   \
        fprintf(output, "    ldr x9, =stack_top\n");            \
        fprintf(output, "    str w0, [x9]\n");                  \
    } while(0)


void basm_save_to_file_as_gas_arm64(Basm *basm, Syscall_Target target, const char *output_file_path)
{
    FILE *output = fopen(output_file_path, "wb");
    if (output == NULL) {
        fprintf(stderr, "ERROR: could not open file %s: %s\n",
                output_file_path, strerror(errno));
        exit(1);
    }

    fprintf(output, "#define BM_STACK_CAPACITY %d\n", BM_STACK_CAPACITY);
    fprintf(output, "#define BM_WORD_SIZE %d\n", BM_WORD_SIZE);

    switch (target) {
    case SYSCALLTARGET_LINUX: {
        fprintf(stderr, "ERROR: Linux is not supported on arm64/aarch64\n");
        exit(1);
    } break;
    case SYSCALLTARGET_FREEBSD: {
        fprintf(output, "#define SYS_EXIT #1\n");
        fprintf(output, "#define SYS_WRITE #4\n");
    } break;
    }

    fprintf(output, "#define STDOUT #1\n");
    fprintf(output, "    .text\n");
    fprintf(output, "    .globl _start\n");

    // size_t jmp_count = 0;
    for (size_t i = 0; i < basm->program_size; ++i) {
        Inst inst = basm->program[i];

        if (i == basm->entry) {
            fprintf(output, "_start:\n");
            LOAD_SP(output);
        }

        fprintf(output, "inst_%zu:\n", i);

        switch (inst.type) {
        case INST_NOP: {
            fprintf(output, "    // nop\n");
            fprintf(output, "    nop\n");
        }
        break;
        case INST_PUSH: {
            fprintf(output, "    // push %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    mov x2, #%"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    str x2, [x0]\n");
            fprintf(output, "    add x0, x0, BM_WORD_SIZE\n");
        }
        break;
        case INST_DROP: {
            fprintf(output, "    // drop\n");
            fprintf(output, "    sub x0, x0, BM_WORD_SIZE\n");
        }
        break;
        case INST_DUP: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_SWAP: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_PLUSI: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_MINUSI: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_MULTI: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_MULTU: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_DIVI: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_DIVU: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_MODI: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_MODU: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_PLUSF: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_MINUSF: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_MULTF: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_DIVF: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_JMP: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_JMP_IF: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_RET: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_CALL: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_NATIVE: {
            if (inst.operand.as_u64 == 0) {
                fprintf(output, "    // native write\n");
                fprintf(output, "    sub x0, x0, BM_WORD_SIZE\n");
                fprintf(output, "    ldr x2, [x0]\n");
                fprintf(output, "    sub x0, x0, BM_WORD_SIZE\n");
                fprintf(output, "    ldr x1, [x0]\n");
                fprintf(output, "    ldr x4, =memory\n");
                fprintf(output, "    add x1, x1, x4\n");
                STORE_SP(output);
                fprintf(output, "    mov x8, SYS_WRITE\n");
                fprintf(output, "    mov x0, STDOUT\n");
                fprintf(output, "    svc #0\n");
                LOAD_SP(output);
            }
        }
        break;
        case INST_HALT: {
            // Clobber like hell ... we're gonna exit here anyways
            fprintf(output, "    // halt\n");
            fprintf(output, "    mov x0, #0\n");
            fprintf(output, "    mov x8, SYS_EXIT\n");
            fprintf(output, "    svc #0\n");
        }
        break;
        case INST_NOT: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_EQI: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_GEI: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_GTI: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_LEI: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_LTI: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_NEI: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_EQU: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_GEU: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_GTU: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_LEU: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_LTU: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_NEU: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_EQF: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_GEF: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_GTF: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_LEF: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_LTF: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_NEF: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_ANDB: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_ORB: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_XOR: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_SHR: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_SHL: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_NOTB: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_READ8I: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_READ8U: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_READ16I: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_READ16U: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_READ32I: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_READ32U: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_READ64I:
        case INST_READ64U: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_WRITE8: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_WRITE16: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_WRITE32: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_WRITE64: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_I2F: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_U2F: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_F2I: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_F2U: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case NUMBER_OF_INSTS:
        default: {
            assert(false && "unknown instruction");
        }
        }
    }

    fprintf(output, "    .data\n");
    fprintf(output, "stack_top: .word stack\n");

    fprintf(output, "memory:\n");
    // XXX: Neat formatting
    for (size_t i = 0; i < basm->memory_size; ++i) {
        fprintf(output, "  .byte %u\n", basm->memory[i]);
    }
    fprintf(output, "  .fill %zu, 1, 0\n", BM_MEMORY_CAPACITY - basm->memory_size);

    fprintf(output, "    .bss\n");
    fprintf(output, "stack: .fill BM_STACK_CAPACITY, 1, 0\n");


    fclose(output);
}
