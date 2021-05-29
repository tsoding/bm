#include "basm.h"

void basm_save_to_file_as_nasm_sysv_x86_64(Basm *basm, OS_Target os_target, const char *output_file_path)
{
    FILE *output = fopen(output_file_path, "wb");
    if (output == NULL) {
        fprintf(stderr, "ERROR: could not open file %s: %s\n",
                output_file_path, strerror(errno));
        exit(1);
    }

    fprintf(output, "BITS 64\n");
    fprintf(output, "%%define BM_STACK_CAPACITY %d\n", BM_STACK_CAPACITY);
    fprintf(output, "%%define BM_WORD_SIZE %d\n", BM_WORD_SIZE);

    switch (os_target) {
    case OS_TARGET_LINUX: {
        fprintf(output, "%%define SYS_EXIT 60\n");
        fprintf(output, "%%define SYS_WRITE 1\n");
        fprintf(output, "%%define STDOUT 1\n");
        fprintf(output, "%%define ENTRY_POINT _start\n");
    }
    break;
    case OS_TARGET_FREEBSD: {
        fprintf(output, "%%define SYS_EXIT 1\n");
        fprintf(output, "%%define SYS_WRITE 4\n");
        fprintf(output, "%%define STDOUT 1\n");
        fprintf(output, "%%define ENTRY_POINT _start\n");
    }
    break;
    case OS_TARGET_WINDOWS: {
        fprintf(output, "extern GetStdHandle\n");
        fprintf(output, "extern WriteFile\n");
        fprintf(output, "extern ExitProcess\n");
        fprintf(output, "%%define ENTRY_POINT _start\n");
    }
    break;
    case OS_TARGET_MACOS: {
        // the syscall numbers are actually the same as BSD, but
        // they need to be left-shifted 24 bits for some reason.
        fprintf(output, "%%define SYS_EXIT 0x2000001\n");
        fprintf(output, "%%define SYS_WRITE 0x2000004\n");
        fprintf(output, "%%define STDOUT 1\n");
        fprintf(output, "%%define ENTRY_POINT _main\n");
    }
    break;
    }

    fprintf(output, "segment .text\n");
    fprintf(output, "global ENTRY_POINT\n\n");

    fprintf(output, "%%macro STACK_PUSH 1\n");
    fprintf(output, "    add r15, BM_WORD_SIZE\n");
    fprintf(output, "    mov [r15], %%1\n");
    fprintf(output, "%%endmacro\n\n");

    fprintf(output, "%%macro STACK_PUSH_XMM 1\n");
    fprintf(output, "    add r15, BM_WORD_SIZE\n");
    fprintf(output, "    movsd [r15], %%1\n");
    fprintf(output, "%%endmacro\n\n");

    fprintf(output, "%%macro STACK_POP 1\n");
    fprintf(output, "    mov %%1, [r15]\n");
    fprintf(output, "    sub r15, BM_WORD_SIZE\n");
    fprintf(output, "%%endmacro\n\n");

    fprintf(output, "%%macro STACK_POP_XMM 1\n");
    fprintf(output, "    movsd %%1, [r15]\n");
    fprintf(output, "    sub r15, BM_WORD_SIZE\n");
    fprintf(output, "%%endmacro\n\n");

    fprintf(output, "%%macro STACK_POP_TWO 2\n");
    fprintf(output, "    mov %%1, [r15]\n");
    fprintf(output, "    mov %%2, [r15 - BM_WORD_SIZE]\n");
    fprintf(output, "    sub r15, 2 * BM_WORD_SIZE\n");
    fprintf(output, "%%endmacro\n\n");

    fprintf(output, "%%macro STACK_POP_TWO_XMM 2\n");
    fprintf(output, "    movsd %%1, [r15]\n");
    fprintf(output, "    movsd %%2, [r15 - BM_WORD_SIZE]\n");
    fprintf(output, "    sub r15, 2 * BM_WORD_SIZE\n");
    fprintf(output, "%%endmacro\n\n");


    /*
        "calling convention"
        --------------------
        special registers:
        - %r15 always points to the top of the stack, such that (%r15) contains the topmost value
        - %r14 always points to the base address of memory (`memory`)
        - %r13 always points to the base address of the instruction array (`inst_map`)

        other than those 3 special registers, every other register is fair game.
    */

    size_t jmp_count = 0;
    for (size_t i = 0; i < basm->program_size; ++i) {
        Inst inst = basm->program[i];

        if (i == basm->entry) {
            fprintf(output, "ENTRY_POINT:\n");
            fprintf(output, "    lea r15, [REL stack]\n");
            fprintf(output, "    lea r14, [REL memory]\n");
            fprintf(output, "    lea r13, [REL inst_map]\n");
            fprintf(output, "\n");
        }

        fprintf(output, "\ninst_%zu:  ;; "FL_Fmt"\n", i, FL_Arg(basm->program_locations[i]));
        switch (inst.type) {
        case INST_NOP: {
            fprintf(output, "    ;; nop\n");
            fprintf(output, "    nop\n");
        }
        break;
        case INST_PUSH: {
            fprintf(output, "    ;; push %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    mov rax, 0x%"PRIX64"\n", inst.operand.as_u64);
            fprintf(output, "    STACK_PUSH rax\n");
        }
        break;
        case INST_DROP: {
            fprintf(output, "    ;; drop\n");
            fprintf(output, "    STACK_POP rax\n");
        }
        break;
        case INST_DUP: {
            fprintf(output, "    ;; dup %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    mov rax, [r15 - (BM_WORD_SIZE * %"PRIu64")]\n", inst.operand.as_u64);
            fprintf(output, "    STACK_PUSH rax\n");
        }
        break;
        case INST_SWAP: {
            fprintf(output, "    ;; swap %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    mov rax, [r15 - (BM_WORD_SIZE * %"PRIu64")]\n", inst.operand.as_u64);
            fprintf(output, "    xchg [r15], rax\n");
            fprintf(output, "    mov [r15 - (BM_WORD_SIZE * %"PRIu64")], rax\n", inst.operand.as_u64);
        }
        break;
        case INST_PLUSI: {
            fprintf(output, "    ;; plusi\n");
            fprintf(output, "    STACK_POP_TWO rax, rbx\n");
            fprintf(output, "    add rbx, rax\n");
            fprintf(output, "    STACK_PUSH rbx\n");
        }
        break;
        case INST_MINUSI: {
            fprintf(output, "    ;; minusi\n");
            fprintf(output, "    STACK_POP_TWO rax, rbx\n");
            fprintf(output, "    sub rbx, rax\n");
            fprintf(output, "    STACK_PUSH rbx\n");
        }
        break;

#define MULT_DIV_INST(comment_name, inst, ret_reg) do { \
fprintf(output, "    ;; " comment_name "\n");           \
fprintf(output, "    STACK_POP_TWO rbx, rax\n");        \
fprintf(output, "    xor edx, edx\n");                  \
fprintf(output, "    " inst " rbx\n");                  \
fprintf(output, "    STACK_PUSH " ret_reg "\n");        \
} while(0)

        case INST_MULTI:
            MULT_DIV_INST("multi", "imul", "rax");
            break;
        case INST_MULTU:
            MULT_DIV_INST("multu", "mul", "rax");
            break;
        case INST_DIVI:
            MULT_DIV_INST("divi", "idiv", "rax");
            break;
        case INST_DIVU:
            MULT_DIV_INST("divu", "div", "rax");
            break;
        case INST_MODI:
            MULT_DIV_INST("modi", "idiv", "rdx");
            break;
        case INST_MODU:
            MULT_DIV_INST("modu", "div", "rdx");
            break;

#undef MULT_DIV_INST


#define FLOAT_BINOP_INST(comment_name, inst) do {       \
fprintf(output, "    ;; " comment_name "\n");           \
fprintf(output, "    STACK_POP_TWO_XMM xmm0, xmm1\n");  \
fprintf(output, "    " inst " xmm1, xmm0\n");           \
fprintf(output, "    STACK_PUSH_XMM xmm1\n");           \
} while(0)

        case INST_PLUSF:
            FLOAT_BINOP_INST("plusf", "addsd");
            break;
        case INST_MINUSF:
            FLOAT_BINOP_INST("minusf", "subsd");
            break;
        case INST_MULTF:
            FLOAT_BINOP_INST("multf", "mulsd");
            break;
        case INST_DIVF:
            FLOAT_BINOP_INST("divf", "divsd");
            break;

#undef FLOAT_BINOP_INST


        case INST_JMP: {
            fprintf(output, "    ;; jmp\n");
            fprintf(output, "    jmp [r13 + BM_WORD_SIZE * %"PRIu64"]\n", inst.operand.as_u64);
        }
        break;
        case INST_JMP_IF: {
            fprintf(output, "    ;; jmp_if %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    STACK_POP rax\n");
            fprintf(output, "    cmp rax, 0\n");
            fprintf(output, "    je jmp_if_escape_%zu\n", jmp_count);
            fprintf(output, "    jmp [r13 + BM_WORD_SIZE * %"PRIu64"]\n", inst.operand.as_u64);
            fprintf(output, "jmp_if_escape_%zu:\n", jmp_count);
            jmp_count += 1;
        }
        break;
        case INST_RET: {
            fprintf(output, "    ;; ret\n");
            fprintf(output, "    STACK_POP rax\n");
            fprintf(output, "    imul rax, rax, BM_WORD_SIZE\n");
            fprintf(output, "    jmp [rax + r13]\n");
        }
        break;
        case INST_CALL: {
            fprintf(output, "    ;; call\n");
            fprintf(output, "    mov rax, %zu\n", i + 1);
            fprintf(output, "    STACK_PUSH rax\n");
            fprintf(output, "    jmp [r13 + BM_WORD_SIZE * %"PRIu64"]\n", inst.operand.as_u64);
        }
        break;
        case INST_NATIVE: {
            if (inst.operand.as_u64 == 0) {
                fprintf(output, "    ;; native write\n");
                switch (os_target) {
                case OS_TARGET_LINUX:
                case OS_TARGET_MACOS:
                case OS_TARGET_FREEBSD: {
                    fprintf(output, "    mov rdi, STDOUT\n");
                    fprintf(output, "    STACK_POP rdx\n");
                    fprintf(output, "    STACK_POP rsi\n");
                    fprintf(output, "    add rsi, r14\n");
                    fprintf(output, "    mov rax, SYS_WRITE\n");
                    fprintf(output, "    syscall\n");
                }
                break;
                case OS_TARGET_WINDOWS: {
                    // TODO: can we replace `stdout_handler` with a register?
                    fprintf(output, "    sub rsp, 40\n");
                    fprintf(output, "    mov ecx, STD_OUTPUT_HANDLE\n");
                    fprintf(output, "    call GetStdHandle\n");
                    fprintf(output, "    mov DWORD [stdout_handler], eax\n");
                    fprintf(output, "    xor r9, r9\n");
                    fprintf(output, "    STACK_POP r8\n");
                    fprintf(output, "    STACK_POP rdx\n");
                    fprintf(output, "    add rdx, r14\n");
                    fprintf(output, "    mov ecx, DWORD [stdout_handler]\n");
                    fprintf(output, "    call WriteFile\n");
                    fprintf(output, "    add rsp, 40\n");
                }
                break;
                }
            } else {
                assert(false && "unsupported native function");
            }
        }
        break;
        case INST_HALT: {
            fprintf(output, "    ;; halt\n");
            switch (os_target) {
            case OS_TARGET_LINUX:
            case OS_TARGET_MACOS:
            case OS_TARGET_FREEBSD: {
                fprintf(output, "    mov rax, SYS_EXIT\n");
                fprintf(output, "    mov rdi, 0\n");
                fprintf(output, "    syscall\n");
            }
            break;
            case OS_TARGET_WINDOWS: {
                fprintf(output, "    push dword 0\n");
                fprintf(output, "    call ExitProcess\n");
            }
            break;
            }
        }
        break;
        case INST_NOT: {
            fprintf(output, "    ;; not\n");
            fprintf(output, "    STACK_POP rbx\n");
            fprintf(output, "    xor eax, eax\n");
            fprintf(output, "    cmp rbx, 0\n");
            fprintf(output, "    setz al\n");
            fprintf(output, "    STACK_PUSH rax\n");
        }
        break;

#define INT_COMPARISON_INST(comment_name, mnemonic) do {    \
fprintf(output, "    ;; " comment_name "\n");               \
fprintf(output, "    STACK_POP_TWO rax, rbx\n");            \
fprintf(output, "    cmp rbx, rax\n");                      \
fprintf(output, "    mov eax, 0\n");                        \
fprintf(output, "    " mnemonic " al\n");                   \
fprintf(output, "    STACK_PUSH rax\n");                    \
} while(0)

        case INST_EQI:
            INT_COMPARISON_INST("eqi", "setz");
            break;
        case INST_GEI:
            INT_COMPARISON_INST("gei", "setge");
            break;
        case INST_GTI:
            INT_COMPARISON_INST("gti", "setg");
            break;
        case INST_LEI:
            INT_COMPARISON_INST("lei", "setle");
            break;
        case INST_LTI:
            INT_COMPARISON_INST("lti", "setl");
            break;
        case INST_NEI:
            INT_COMPARISON_INST("nei", "setne");
            break;
        case INST_EQU:
            INT_COMPARISON_INST("equ", "sete");
            break;
        case INST_GEU:
            INT_COMPARISON_INST("geu", "setae");
            break;
        case INST_GTU:
            INT_COMPARISON_INST("gtu", "seta");
            break;
        case INST_LEU:
            INT_COMPARISON_INST("leu", "setbe");
            break;
        case INST_LTU:
            INT_COMPARISON_INST("ltu", "setb");
            break;
        case INST_NEU:
            INT_COMPARISON_INST("neu", "setne");
            break;

#undef INT_COMPARISON_INST

#define FLOAT_COMPARISON_INST(comment_name, comisd, mnemonic) do {  \
fprintf(output, "    ;; " comment_name "\n");                       \
fprintf(output, "    STACK_POP_TWO_XMM xmm0, xmm1\n");              \
fprintf(output, "    xor eax, eax\n");                              \
fprintf(output, "    " comisd " xmm1, xmm0\n");                     \
fprintf(output, "    " mnemonic " al\n");                           \
fprintf(output, "    STACK_PUSH rax\n");                            \
} while(0)

        case INST_EQF:
            FLOAT_COMPARISON_INST("eqf", "comisd", "sete");
            break;
        case INST_GEF:
            FLOAT_COMPARISON_INST("gef", "comisd", "setae");
            break;
        case INST_GTF:
            FLOAT_COMPARISON_INST("gtf", "comisd", "seta");
            break;
        case INST_LEF:
            FLOAT_COMPARISON_INST("lef", "comisd", "setbe");
            break;
        case INST_LTF:
            FLOAT_COMPARISON_INST("ltf", "comisd", "setb");
            break;
        case INST_NEF:
            FLOAT_COMPARISON_INST("nef", "comisd", "setne");
            break;

#undef FLOAT_COMPARISON_INST

#define BITWISE_INST(comment_name, inst) do {       \
fprintf(output, "    ;; " comment_name "\n");       \
fprintf(output, "    STACK_POP_TWO rax, rbx\n");    \
fprintf(output, "    " inst " rax, rbx\n");         \
fprintf(output, "    STACK_PUSH rax\n");            \
} while(0)

        case INST_ANDB:
            BITWISE_INST("andb", "and");
            break;
        case INST_ORB:
            BITWISE_INST("orb", "or");
            break;
        case INST_XOR:
            BITWISE_INST("xor", "xor");
            break;

#undef BITWISE_INST

        case INST_NOTB: {
            fprintf(output, "    ;; notb\n");
            fprintf(output, "    STACK_POP rax\n");
            fprintf(output, "    not rax\n");
            fprintf(output, "    STACK_PUSH rax\n");
        }
        break;
        case INST_SHR: {
            fprintf(output, "    ;; shr\n");
            fprintf(output, "    STACK_POP_TWO rcx, rax\n");
            fprintf(output, "    shr rax, cl\n");
            fprintf(output, "    STACK_PUSH rax\n");
        }
        break;
        case INST_SHL: {
            fprintf(output, "    ;; shl\n");
            fprintf(output, "    STACK_POP_TWO rcx, rax\n");
            fprintf(output, "    shl rax, cl\n");
            fprintf(output, "    STACK_PUSH rax\n");
        }
        break;

#define READ_SIGNED_INST(comment_name, reg, size, sign_extend) do { \
fprintf(output, "    ;; " comment_name "\n");                       \
fprintf(output, "    STACK_POP rbx\n");                             \
fprintf(output, "    add rbx, r14\n");                              \
fprintf(output, "    mov " reg ", " size " [rbx]\n");               \
fprintf(output, "    " sign_extend "\n");                           \
fprintf(output, "    STACK_PUSH rax\n");                            \
} while(0)

// note: this changes the semantics slightly; we now zero-extend when
// reading unsigned values, where previously the rest of the memory was
// left undefined
#define READ_UNSIGNED_INST(comment_name, reg, size, zero_extend) do {   \
fprintf(output, "    ;; " comment_name "\n");                           \
fprintf(output, "    STACK_POP rbx\n");                                 \
fprintf(output, "    add rbx, r14\n");                                  \
fprintf(output, "    mov " reg ", " size " [rbx]\n");                   \
fprintf(output, "    " zero_extend "\n");                               \
fprintf(output, "    STACK_PUSH rax\n");                                \
} while(0)

        case INST_READ8I:
            READ_SIGNED_INST("read8i", "al", "BYTE", "movsx rax, al");
            break;
        case INST_READ16I:
            READ_SIGNED_INST("read16i", "ax", "WORD", "movsx rax, ax");
            break;
        case INST_READ32I:
            READ_SIGNED_INST("read32i", "eax", "DWORD", "movsx rax, eax");
            break;
        case INST_READ64I:
            READ_SIGNED_INST("read64i", "rax", "QWORD", "nop");
            break;

        case INST_READ8U:
            READ_UNSIGNED_INST("read8u", "al", "BYTE", "movzx rax, al");
            break;
        case INST_READ16U:
            READ_UNSIGNED_INST("read16u", "ax", "WORD", "movzx rax, ax");
            break;
        case INST_READ32U:
            READ_UNSIGNED_INST("read32u", "eax", "DWORD", "nop");
            break;
        case INST_READ64U:
            READ_UNSIGNED_INST("read64u", "rax", "QWORD", "nop");
            break;

#undef READ_SIGNED_INST
#undef READ_UNSIGNED_INST

#define WRITE_INST(comment_name, reg, size) do {        \
fprintf(output, "    ;; " comment_name "\n");           \
fprintf(output, "    STACK_POP_TWO rax, rbx\n");        \
fprintf(output, "    add rbx, r14\n");                  \
fprintf(output, "    mov " size " [rbx], " reg "\n");   \
} while(0)

        case INST_WRITE8:
            WRITE_INST("write8", "al", "BYTE");
            break;
        case INST_WRITE16:
            WRITE_INST("write16", "ax", "WORD");
            break;
        case INST_WRITE32:
            WRITE_INST("write32", "eax", "DWORD");
            break;
        case INST_WRITE64:
            WRITE_INST("write64", "rax", "QWORD");
            break;

#undef WRITE_INST

        case INST_I2F: {
            fprintf(output, "    ;; i2f\n");
            fprintf(output, "    STACK_POP rax\n");
            fprintf(output, "    pxor xmm0, xmm0\n");
            fprintf(output, "    cvtsi2sd xmm0, rax\n");
            fprintf(output, "    STACK_PUSH_XMM xmm0\n");
        }
        break;
        case INST_F2I: {
            fprintf(output, "    ;; f2i\n");
            fprintf(output, "    STACK_POP_XMM xmm0\n");
            fprintf(output, "    cvttsd2si rax, xmm0\n");
            fprintf(output, "    STACK_PUSH rax\n");
        }
        break;

        case INST_U2F: {
            fprintf(output, "    ;; u2f\n");
            fprintf(output, "    STACK_POP rdi\n");
            fprintf(output, "    test rdi, rdi\n");
            fprintf(output, "    js .u2f_%zu\n", jmp_count);
            fprintf(output, "    pxor xmm0, xmm0\n");
            fprintf(output, "    cvtsi2sd xmm0, rdi\n");
            fprintf(output, "    jmp .u2f_end_%zu\n", jmp_count);
            fprintf(output, "    .u2f_%zu:\n", jmp_count);
            fprintf(output, "    mov rax, rdi\n");
            fprintf(output, "    and edi, 1\n");
            fprintf(output, "    pxor xmm0, xmm0\n");
            fprintf(output, "    shr rax, 1\n");
            fprintf(output, "    or rax, rdi\n");
            fprintf(output, "    cvtsi2sd xmm0, rax\n");
            fprintf(output, "    addsd xmm0, xmm0\n");
            fprintf(output, "    .u2f_end_%zu:\n", jmp_count);
            fprintf(output, "    STACK_PUSH_XMM xmm0\n");
            jmp_count += 1;
        }
        break;

        // TODO(#349): try to get rid of branching in f2u implementation of basm_save_to_file_as_nasm()
        case INST_F2U: {
            fprintf(output, "    ;; f2i\n");
            fprintf(output, "    STACK_POP_XMM xmm0\n");
            fprintf(output, "    movsd xmm1, [REL magic_number_for_f2u]\n");
            fprintf(output, "    comisd xmm0, xmm1\n");
            fprintf(output, "    jnb .f2u_%zu\n", jmp_count);
            fprintf(output, "    cvttsd2si rax, xmm0\n");
            fprintf(output, "    jmp .f2u_end_%zu\n", jmp_count);
            fprintf(output, "    .f2u_%zu:\n", jmp_count);
            fprintf(output, "    subsd xmm0, xmm1\n");
            fprintf(output, "    cvttsd2si rax, xmm0\n");
            fprintf(output, "    btc rax, 63\n");
            fprintf(output, "    .f2u_end_%zu:\n", jmp_count);
            fprintf(output, "    STACK_PUSH rax\n");
        }
        break;
        case NUMBER_OF_INSTS:
        default:
            assert(false && "unknown instruction");
        }
    }

    fprintf(output, "    ret\n");
    fprintf(output, "segment .data\n");
    switch (os_target) {
    case OS_TARGET_WINDOWS:
        fprintf(output, "stdout_handler: dd 0\n");
        break;
    case OS_TARGET_LINUX:
    case OS_TARGET_MACOS:
    case OS_TARGET_FREEBSD:
        // Nothing to do
        break;
    default:
        assert(false && "unreachable");
        exit(1);
    }
    fprintf(output, "magic_number_for_f2u: dq 0x43E0000000000000\n");
    fprintf(output, "inst_map:\n");
#define ROW_SIZE 5
#define ROW_COUNT(size) ((size + ROW_SIZE - 1) / ROW_SIZE)
#define INDEX(row, col) ((row) * ROW_SIZE + (col))
    for (size_t row = 0; row < ROW_COUNT(basm->program_size); ++row) {
        fprintf(output, "  dq");
        for (size_t col = 0; col < ROW_SIZE && INDEX(row, col) < basm->program_size; ++col) {
            fprintf(output, " inst_%zu,", INDEX(row, col));
        }
        fprintf(output, "\n");
    }
    fprintf(output, "\n");
    fprintf(output, "memory:\n");
    for (size_t row = 0; row < ROW_COUNT(basm->memory_size); ++row) {
        fprintf(output, "  db");
        for (size_t col = 0; col < ROW_SIZE && INDEX(row, col) < basm->memory_size; ++col) {
            fprintf(output, " %u,", basm->memory[INDEX(row, col)]);
        }
        fprintf(output, "\n");
    }
    fprintf(output, "  times %zu db 0", BM_MEMORY_CAPACITY - basm->memory_size);
#undef ROW_SIZE
#undef ROW_COUNT
    fprintf(output, "\n");
    fprintf(output, "segment .bss\n");
    switch (os_target) {
    case OS_TARGET_WINDOWS:
        fprintf(output, "STD_OUTPUT_HANDLE equ -11\n");
        break;
    case OS_TARGET_LINUX:
    case OS_TARGET_MACOS:
    case OS_TARGET_FREEBSD:
        // Nothing to do
        break;
    default:
        assert(false && "unreachable");
        exit(1);
    }
    fprintf(output, "stack: resq BM_STACK_CAPACITY\n");

    fclose(output);
}
