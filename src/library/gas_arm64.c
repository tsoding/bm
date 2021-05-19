#include "basm.h"

#define LOAD_SP                                         \
    do {                                                \
        fprintf(output, "    // LOAD_SP\n");            \
        fprintf(output, "    ldr x9, =stack_top\n");    \
        fprintf(output, "    ldr w0, [x9]\n");          \
    } while(0)

#define STORE_SP                                                \
    do {                                                        \
        fprintf(output, "    // STORE_SP\n");                   \
        fprintf(output, "    ldr x9, =stack_top\n");            \
        fprintf(output, "    str w0, [x9]\n");                  \
    } while(0)

#define BINARY_OP(op)                                                   \
    do {                                                                \
        fprintf(output, "    ldr x9, [x0, #-BM_WORD_SIZE]!\n");         \
        fprintf(output, "    ldr x10, [x0, #-BM_WORD_SIZE]!\n");        \
        fprintf(output, "    "op" x11, x10, x9\n");                     \
        fprintf(output, "    str x11, [x0], #BM_WORD_SIZE\n");          \
    } while (0)

#define CMP_FLT(cc)                                                 \
    do {                                                            \
        fprintf(output, "    ldr d1, [x0, #-BM_WORD_SIZE]!\n");     \
        fprintf(output, "    ldr d2, [x0, #-BM_WORD_SIZE]!\n");     \
        fprintf(output, "    fcmp d2, d1\n");                       \
        fprintf(output, "    cset x1, "cc"\n");                     \
        fprintf(output, "    str x1, [x0], #BM_WORD_SIZE\n");       \
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

    size_t jmp_count = 0;
    for (size_t i = 0; i < basm->program_size; ++i) {
        Inst inst = basm->program[i];

        if (i == basm->entry) {
            fprintf(output, "_start:\n");
            LOAD_SP;
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
            fprintf(output, "    ldr x9, =0x%"PRIx64"\n", inst.operand.as_u64);
            fprintf(output, "    str x9, [x0], #BM_WORD_SIZE\n");
        }
        break;
        case INST_DROP: {
            fprintf(output, "    // drop\n");
            fprintf(output, "    sub x0, x0, BM_WORD_SIZE\n");
        }
        break;
        case INST_DUP: {
            fprintf(output, "    // dup %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    mov x9, #%"PRIu64"\n", inst.operand.as_u64 + 1); // Offset
            fprintf(output, "    mov x10, #BM_WORD_SIZE\n");                      // Wordsize
            fprintf(output, "    msub x1, x9, x10, x0\n");                        // x1 = x0 - x9 * x10 = SP - n * BM_WORDSIZE
            fprintf(output, "    ldr x1, [x1]\n");                                // deref
            fprintf(output, "    str x1, [x0], #BM_WORD_SIZE\n");                 // store at sp
        }
        break;
        case INST_SWAP: {
            fprintf(output, "    // swap %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    ldr x9, [x0, #-BM_WORD_SIZE]!\n");
            fprintf(output, "    ldr x10, [x0, #-%zu]!         // = %zu * BM_WORD_SIZE\n", inst.operand.as_u64 * BM_WORD_SIZE, inst.operand.as_u64);
            fprintf(output, "    str x9, [x0], #%zu            // as previous\n", inst.operand.as_u64 * BM_WORD_SIZE);
            fprintf(output, "    str x10, [x0], #BM_WORD_SIZE\n");
        }
        break;
        case INST_PLUSI: {
            fprintf(output, "    // plusi\n");
            BINARY_OP("add");
        }
        break;
        case INST_MINUSI: {
            fprintf(output, "    // plusi\n");
            BINARY_OP("sub");
        }
        break;
        case INST_MULTI: {
            fprintf(output, "    // multi\n");
            BINARY_OP("mul");
        }
        break;
        case INST_MULTU: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_DIVI: {
            fprintf(output, "    // divi\n");
            BINARY_OP("sdiv");
        }
        break;
        case INST_DIVU: {
            fprintf(output, "    // divu\n");
            BINARY_OP("udiv");
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
            fprintf(output, "    // jmp %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    b inst_%"PRIu64"\n", inst.operand.as_u64);
        }
        break;
        case INST_JMP_IF: {
            fprintf(output, "    // jmp_if\n");
            fprintf(output, "    ldr x9, [x0, #-BM_WORD_SIZE]!\n");
            fprintf(output, "    cmp x9, 0\n");
            fprintf(output, "    beq jmp_if_escape_%zu\n", jmp_count);
            fprintf(output, "    b inst_%"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "jmp_if_escape_%zu:\n", jmp_count);
            jmp_count += 1;
        }
        break;
        case INST_RET: {
            fprintf(output, "    // ret\n");
            fprintf(output, "    ldr x9, [x0, #-BM_WORD_SIZE]!\n");
            // XXX: Hack to make ret work
            fprintf(output, "    mov x10, #4\n");
            fprintf(output, "    ldr x11, =inst_map\n");
            fprintf(output, "    madd x12, x9, x10, x11\n");
            fprintf(output, "    ldr w10, [x12]\n");
            fprintf(output, "    br x10\n");
        }
        break;
        case INST_CALL: {
            fprintf(output, "    // call\n");
            fprintf(output, "    mov x9, #%zu\n", i + 1);
            fprintf(output, "    str x9, [x0], #BM_WORD_SIZE\n");
            fprintf(output, "    b inst_%"PRIu64"\n", inst.operand.as_u64);
        }
        break;
        case INST_NATIVE: {
            if (inst.operand.as_u64 == 0) {
                fprintf(output, "    // native write\n");
                fprintf(output, "    ldr x2, [x0, #-BM_WORD_SIZE]!\n");
                fprintf(output, "    ldr x1, [x0, #-BM_WORD_SIZE]!\n");
                fprintf(output, "    ldr x4, =memory\n");
                fprintf(output, "    add x1, x1, x4\n");
                STORE_SP;
                fprintf(output, "    mov x8, SYS_WRITE\n");
                fprintf(output, "    mov x0, STDOUT\n");
                fprintf(output, "    svc #0\n");
                LOAD_SP;
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
            fprintf(output, "    // eqi\n");
            fprintf(output, "    ldr x9, [x0, #-BM_WORD_SIZE]!\n");
            fprintf(output, "    ldr x10, [x0, #-BM_WORD_SIZE]!\n");
            fprintf(output, "    cmp x9, x10\n");
            fprintf(output, "    cset x10, EQ\n");
            fprintf(output, "    str x10, [x0], #BM_WORD_SIZE\n");
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
            fprintf(output, "    // lti\n");
            fprintf(output, "    ldr x9, [x0, #-BM_WORD_SIZE]!\n");
            fprintf(output, "    ldr x10, [x0, #-BM_WORD_SIZE]!\n");
            fprintf(output, "    cmp x10, x9\n");
            fprintf(output, "    cset x10, LT\n");
            fprintf(output, "    str x10, [x0], #BM_WORD_SIZE\n");
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
            fprintf(output, "    // eqf\n");
            CMP_FLT("EQ");
        }
        break;
        case INST_GEF: {
            fprintf(output, "    // gef\n");
            CMP_FLT("GE");
        }
        break;
        case INST_GTF: {
            fprintf(output, "    // gtf\n");
            CMP_FLT("GT");
        }
        break;
        case INST_LEF: {
            fprintf(output, "    // lef\n");
            CMP_FLT("LE");
        }
        break;
        case INST_LTF: {
            fprintf(output, "    // ltf\n");
            CMP_FLT("LT");
        }
        break;
        case INST_NEF: {
            fprintf(output, "    // nef\n");
            CMP_FLT("NE");
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
            fprintf(output, "    // xor\n");
            BINARY_OP("EOR");
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
            fprintf(output, "    // read8i\n");
            fprintf(output, "    ldr x9, [x0, #-BM_WORD_SIZE]!\n"); // Load address from stack
            fprintf(output, "    ldr x10, =memory\n");              // Load memory offset
            fprintf(output, "    add x9, x9, x10\n");               // Calculate address to load
            fprintf(output, "    ldrsb x10, [x9]\n");               // Read signed
            fprintf(output, "    str x10, [x0], #BM_WORD_SIZE\n");  // Store on stack
        }
        break;
        case INST_READ8U: {
            fprintf(output, "    // read8u\n");
            fprintf(output, "    ldr x9, [x0, #-BM_WORD_SIZE]!\n"); // Load address from stack
            fprintf(output, "    ldr x10, =memory\n");              // Load memory offset
            fprintf(output, "    add x9, x9, x10\n");               // Calculate address to load
            fprintf(output, "    ldursb x10, [x9]\n");              // Read unsigned
            fprintf(output, "    str x10, [x0], #BM_WORD_SIZE\n");  // Store on stack
        }
        break;
        case INST_READ16I: {
            fprintf(stderr, "Instruction is not yet implemented\n"); abort();
        }
        break;
        case INST_READ16U: {
            fprintf(output, "    // read8u\n");
            fprintf(output, "    ldr x9, [x0, #-BM_WORD_SIZE]!\n"); // Load address from stack
            fprintf(output, "    ldr x10, =memory\n");              // Load memory offset
            fprintf(output, "    add x9, x9, x10\n");               // Calculate address to load
            fprintf(output, "    mov x10, #0\n");                   // zero out x10
            fprintf(output, "    ldurh w10, [x9]\n");               // Load half a word of the lower part of x10 signed
            fprintf(output, "    str x10, [x0], #BM_WORD_SIZE\n");  // Store on stack
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
            fprintf(output, "    // write8\n");
            fprintf(output, "    ldr x9, [x0, #-BM_WORD_SIZE]!\n");  // Value
            fprintf(output, "    ldr x10, [x0, #-BM_WORD_SIZE]!\n"); // Offset
            fprintf(output, "    ldr x11, =memory\n");
            fprintf(output, "    add x10, x10, x11\n");             // Address
            fprintf(output, "    strb w9, [x10]\n");                // Store byte
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
            fprintf(output, "    // i2f\n");
            fprintf(output, "    ldr x9, [x0, #-BM_WORD_SIZE]!\n");
            fprintf(output, "    scvtf d0, x9\n");
            fprintf(output, "    fmov x9, d0\n");
            fprintf(output, "    str x9, [x0], #BM_WORD_SIZE\n");
        }
        break;
        case INST_U2F: {
            fprintf(output, "    // u2f\n");
            fprintf(output, "    ldr x9, [x0, #-BM_WORD_SIZE]!\n");
            fprintf(output, "    ucvtf d0, x9\n");
            fprintf(output, "    fmov x9, d0\n");
            fprintf(output, "    str x9, [x0], #BM_WORD_SIZE\n");
        }
        break;
        case INST_F2I: {
            fprintf(output, "    // f2i\n");
            fprintf(output, "    ldr d0, [x0, #-BM_WORD_SIZE]!\n");
            fprintf(output, "    fcvtzs x9, d0\n");
            fprintf(output, "    str x9, [x0], #BM_WORD_SIZE\n");
        }
        break;
        case INST_F2U: {
            fprintf(output, "    // f2u\n");
            fprintf(output, "    ldr d0, [x0, #-BM_WORD_SIZE]!\n");
            fprintf(output, "    fcvtzu x9, d0\n");
            fprintf(output, "    str x9, [x0], #BM_WORD_SIZE\n");
        }
        break;
        case NUMBER_OF_INSTS:
        default: {
            assert(false && "unknown instruction");
        }
        }
    }

    fprintf(output, "    .data\n");
    fprintf(output, "inst_map:\n");
    for (size_t i = 0; i < basm->program_size; ++i) {
        fprintf(output, "    .word inst_%zu\n", i);
    }

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
