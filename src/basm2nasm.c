#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define BM_IMPLEMENTATION
#include "bm.h"

static void usage(FILE *stream)
{
    fprintf(stream, "Usage: ./basm2nasm <input.basm> <output.asm>\n");
}

static char *shift(int *argc, char ***argv)
{
    assert(*argc > 0);
    char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

int main(int argc, char *argv[])
{
    shift(&argc, &argv);        // skip the program

    if (argc == 0) {
        usage(stderr);
        fprintf(stderr, "ERROR: no input provided.\n");
        exit(1);
    }
    const char *input_file_path = shift(&argc, &argv);

    if (argc == 0) {
        usage(stderr);
        fprintf(stderr, "ERROR: no output provided.\n");
        exit(1);
    }
    const char *output_file_path = shift(&argc, &argv);


    // NOTE: The structure might be quite big due its arena. Better allocate it in the static memory.
    static Basm basm = {0};
    basm_translate_source(&basm, sv_from_cstr(input_file_path));

    FILE *output = fopen(output_file_path, "wb");
    if (output == NULL) {
        fprintf(stderr, "ERROR: could not open file %s: %s\n",
                output_file_path, strerror(errno));
        exit(1);
    }

    fprintf(output, "BITS 64\n");
    fprintf(output, "%%define BM_STACK_CAPACITY %d\n", BM_STACK_CAPACITY);
    fprintf(output, "%%define BM_WORD_SIZE %d\n", BM_WORD_SIZE);
    fprintf(output, "%%define SYS_EXIT 60\n");
    fprintf(output, "%%define SYS_WRITE 1\n");
    fprintf(output, "%%define STDOUT 1\n");
    fprintf(output, "segment .text\n");
    fprintf(output, "global _start\n");

    size_t jmp_if_escape_count = 0;
    for (size_t i = 0; i < basm.program_size; ++i) {
        Inst inst = basm.program[i];

        if (i == basm.entry) {
            fprintf(output, "_start:\n");
        }

        fprintf(output, "inst_%zu:\n", i);
        switch (inst.type) {
        case INST_NOP: assert(false && "NOP is not implemented");
        case INST_PUSH: {
            fprintf(output, "    ;; push %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    mov rax, 0x%"PRIX64"\n", inst.operand.as_u64);
            fprintf(output, "    mov QWORD [rsi], rax\n");
            fprintf(output, "    add QWORD [stack_top], BM_WORD_SIZE\n");
        } break;
        case INST_DROP: {
            fprintf(output, "    ;; drop\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        } break;
        case INST_DUP: {
            fprintf(output, "    ;; dup %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    mov rdi, rsi\n");
            fprintf(output, "    sub rdi, BM_WORD_SIZE * (%"PRIu64" + 1)\n", inst.operand.as_u64);
            fprintf(output, "    mov rax, [rdi]\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        } break;
        case INST_SWAP: {
            fprintf(output, "    ;; swap %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rdi, rsi\n");
            fprintf(output, "    sub rdi, BM_WORD_SIZE * %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    mov rbx, [rdi]\n");
            fprintf(output, "    mov [rdi], rax\n");
            fprintf(output, "    mov [rsi], rbx\n");
        } break;
        case INST_PLUSI: {
            fprintf(output, "    ;; plusi\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    add rax, rbx\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        } break;
        case INST_MINUSI: {
            fprintf(output, "    ;; minusi\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    sub rax, rbx\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        } break;
        case INST_MULTI: {
            fprintf(output, "    ;; multi\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], r11\n");
            fprintf(output, "    mov rax, [r11]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [r11]\n");
            fprintf(output, "    imul rax, rbx\n");
            fprintf(output, "    mov [r11], rax\n");
        } break;
        case INST_MULTU: assert(false && "MULTU is not implemented");
        case INST_DIVI: {
            fprintf(output, "    ;; divi\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    xor rdx, rdx\n");
            fprintf(output, "    idiv rbx\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        } break;
        case INST_DIVU: {
            fprintf(output, "    ;; divi\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    xor rdx, rdx\n");
            fprintf(output, "    div rbx\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        } break;
        case INST_MODI: {
            fprintf(output, "    ;; modi\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    xor rdx, rdx\n");
            fprintf(output, "    idiv rbx\n");
            fprintf(output, "    mov [rsi], rdx\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        } break;

        case INST_MODU: {
            fprintf(output, "    ;; modi\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    xor rdx, rdx\n");
            fprintf(output, "    div rbx\n");
            fprintf(output, "    mov [rsi], rdx\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        } break;
        case INST_PLUSF: {
            fprintf(output, "    ;; plusf\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], r11\n");
            fprintf(output, "    movsd xmm0, [r11]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    movsd xmm1, [r11]\n");
            fprintf(output, "    addsd xmm1, xmm0\n");
            fprintf(output, "    movsd [r11], xmm1\n");
        } break;
        case INST_MINUSF: {
            fprintf(output, "    ;; minusf\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], r11\n");
            fprintf(output, "    movsd xmm0, [r11]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    movsd xmm1, [r11]\n");
            fprintf(output, "    subsd xmm1, xmm0\n");
            fprintf(output, "    movsd [r11], xmm1\n");
        } break;
        case INST_MULTF: {
            fprintf(output, "    ;; multf\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], r11\n");
            fprintf(output, "    movsd xmm0, [r11]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    movsd xmm1, [r11]\n");
            fprintf(output, "    mulsd xmm1, xmm0\n");
            fprintf(output, "    movsd [r11], xmm1\n");
        } break;
        case INST_DIVF: {
            fprintf(output, "    ;; divf\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], r11\n");
            fprintf(output, "    movsd xmm0, [r11]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    movsd xmm1, [r11]\n");
            fprintf(output, "    divsd xmm1, xmm0\n");
            fprintf(output, "    movsd [r11], xmm1\n");
        } break;
        case INST_JMP: {
            fprintf(output, "    ;; jmp\n");
            fprintf(output, "    mov rdi, inst_map\n");
            fprintf(output, "    add rdi, BM_WORD_SIZE * %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    jmp [rdi]\n");
        } break;
        case INST_JMP_IF: {
            fprintf(output, "    ;; jmp_if %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    mov [stack_top], rsi\n");
            fprintf(output, "    cmp rax, 0\n");
            fprintf(output, "    je jmp_if_escape_%zu\n", jmp_if_escape_count);
            fprintf(output, "    mov rdi, inst_map\n");
            fprintf(output, "    add rdi, BM_WORD_SIZE * %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    jmp [rdi]\n");
            fprintf(output, "jmp_if_escape_%zu:\n", jmp_if_escape_count);
            jmp_if_escape_count += 1;
        } break;
        case INST_RET: {
            fprintf(output, "    ;; ret\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    mov rbx, BM_WORD_SIZE\n");
            fprintf(output, "    mul rbx\n");
            fprintf(output, "    add rax, inst_map\n");
            fprintf(output, "    mov [stack_top], rsi\n");
            fprintf(output, "    jmp [rax]\n");
        } break;
        case INST_CALL: {
            fprintf(output, "    ;; call\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    mov QWORD [rsi], %zu\n", i + 1);
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
            fprintf(output, "    mov rdi, inst_map\n");
            fprintf(output, "    add rdi, BM_WORD_SIZE * %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    jmp [rdi]\n");
        } break;
        case INST_NATIVE: {
            if (inst.operand.as_u64 == 3) {
                fprintf(output, "    ;; native print_i64\n");
                fprintf(output, "    call print_i64\n");
            } else if (inst.operand.as_u64 == 7) {
                fprintf(output, "    ;; native write\n");
                fprintf(output, "    mov r11, [stack_top]\n");
                fprintf(output, "    sub r11, BM_WORD_SIZE\n");
                fprintf(output, "    mov rdx, [r11]\n");
                fprintf(output, "    sub r11, BM_WORD_SIZE\n");
                fprintf(output, "    mov rsi, [r11]\n");
                fprintf(output, "    add rsi, memory\n");
                fprintf(output, "    mov rdi, STDOUT\n");
                fprintf(output, "    mov rax, SYS_WRITE\n");
                fprintf(output, "    mov [stack_top], r11\n");
                fprintf(output, "    syscall\n");
            } else {
                assert(false && "unsupported native function");
            }
        } break;
        case INST_HALT: {
            fprintf(output, "    ;; halt\n");
            fprintf(output, "    mov rax, SYS_EXIT\n");
            fprintf(output, "    mov rdi, 0\n");
            fprintf(output, "    syscall\n");
        } break;
        case INST_NOT: {
            fprintf(output, "    ;; not\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    cmp rax, 0\n");
            fprintf(output, "    mov rax, 0\n");
            fprintf(output, "    setz al\n");
            fprintf(output, "    mov [rsi], rax\n");
        } break;
        case INST_EQI: {
            fprintf(output, "    ;; eqi\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    cmp rax, rbx\n");
            fprintf(output, "    mov rax, 0\n");
            fprintf(output, "    setz al\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        } break;
        case INST_GEI: {
            fprintf(output, "    ;; FIXME: gei\n");
        } break;
        case INST_GTI: {
            fprintf(output, "    ;; FIXME: gti\n");
        } break;
        case INST_LEI: {
            fprintf(output, "    ;; FIXME: lei\n");
        } break;
        case INST_LTI: assert(false && "LTI is not implemented");
        case INST_NEI: assert(false && "NEI is not implemented");
        case INST_EQU: {
            fprintf(output, "    ;; FIXME: equ\n");
        } break;
        case INST_GEU: assert(false && "GEU is not implemented");
        case INST_GTU: assert(false && "GTU is not implemented");
        case INST_LEU: assert(false && "LEU is not implemented");
        case INST_LTU: assert(false && "LTU is not implemented");
        case INST_NEU: assert(false && "NEU is not implemented");
        case INST_EQF: assert(false && "EQF is not implemented");
        case INST_GEF: {
            fprintf(output, "    ;; FIXME: gef\n");
        } break;
        case INST_GTF: {
            fprintf(output, "    ;; FIXME: gtf\n");
        } break;
        case INST_LEF: {
            fprintf(output, "    ;; FIXME: lef\n");
        } break;
        case INST_LTF: {
            fprintf(output, "    ;; FIXME: ltf\n");
        } break;
        case INST_NEF: assert(false && "NEF is not implemented");
        case INST_ANDB: {
            fprintf(output, "    ;; FIXME: andb\n");
        } break;
        case INST_ORB: assert(false && "ORB is not implemented");
        case INST_XOR: {
            fprintf(output, "    ;; FIXME: xor\n");
        } break;
        case INST_SHR: assert(false && "SHR is not implemented");
        case INST_SHL: assert(false && "SHL is not implemented");
        case INST_NOTB: assert(false && "NOTB is not implemented");
        case INST_READ8: {
            fprintf(output, "    ;; read8\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rsi, [r11]\n");
            fprintf(output, "    add rsi, memory\n");
            fprintf(output, "    xor rax, rax\n");
            fprintf(output, "    mov al, BYTE [rsi]\n");
            fprintf(output, "    mov [r11], rax\n");
        } break;
        case INST_READ16: assert(false && "READ16 is not implemented");
        case INST_READ32: assert(false && "READ32 is not implemented");
        case INST_READ64: assert(false && "READ64 is not implemented");
        case INST_WRITE8: {
            fprintf(output, "    ;; write8\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [r11]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rsi, [r11]\n");
            fprintf(output, "    add rsi, memory\n");
            fprintf(output, "    mov BYTE [rsi], al\n");
            fprintf(output, "    mov [stack_top], r11\n");
        } break;
        case INST_WRITE16: assert(false && "WRITE16 is not implemented");
        case INST_WRITE32: assert(false && "WRITE32 is not implemented");
        case INST_WRITE64: assert(false && "WRITE64 is not implemented");
        case INST_I2F: {
            fprintf(output, "    ;; FIXME: i2f\n");
        } break;
        case INST_U2F: assert(false && "U2F is not implemented");
        case INST_F2I: {
            fprintf(output, "    ;; FIXME: f2i\n");
        } break;
        case INST_F2U: assert(false && "F2U is not implemented");
        case NUMBER_OF_INSTS:
        default: assert(false && "unknown instruction");
        }
    }

    fprintf(output, "    ret\n");
    fprintf(output, "segment .data\n");
    fprintf(output, "stack_top: dq stack\n");
    fprintf(output, "inst_map:\n");
#define ROW_SIZE 5
#define ROW_COUNT(size) ((size + ROW_SIZE - 1) / ROW_SIZE)
#define INDEX(row, col) ((row) * ROW_SIZE + (col))
    for (size_t row = 0; row < ROW_COUNT(basm.program_size); ++row) {
        fprintf(output, "  dq");
        for (size_t col = 0; col < ROW_SIZE && INDEX(row, col) < basm.program_size; ++col) {
            fprintf(output, " inst_%zu,", INDEX(row, col));
        }
        fprintf(output, "\n");
    }
    fprintf(output, "\n");
    fprintf(output, "memory:\n");
    for (size_t row = 0; row < ROW_COUNT(basm.memory_size); ++row) {
        fprintf(output, "  db");
        for (size_t col = 0; col < ROW_SIZE && INDEX(row, col) < basm.memory_size; ++col) {
            fprintf(output, " %u,", basm.memory[INDEX(row, col)]);
        }
        fprintf(output, "\n");
    }
    fprintf(output, "  times %zu db 0", BM_MEMORY_CAPACITY - basm.memory_size);
#undef ROW_SIZE
#undef ROW_COUNT
    fprintf(output, "\n");
    fprintf(output, "segment .bss\n");
    fprintf(output, "stack: resq BM_STACK_CAPACITY\n");

    fclose(output);

    return 0;
}
