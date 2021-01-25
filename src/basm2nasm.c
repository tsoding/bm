#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define BM_IMPLEMENTATION
#include "bm.h"

typedef enum {
    ELF64 = 0,
    WIN64,
} Output_Format;

static void usage(FILE *stream)
{
    fprintf(stream, "Usage: ./basm2nasm <input.basm>\n");
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        usage(stderr);
        if (argc < 2) {
            fprintf(stderr, "ERROR: no input provided.\n");
        } else {
            fprintf(stderr, "ERROR: invalid input.\n");
        }
        exit(1);
    }

    const char *flag = argv[1];
    Output_Format output_format = {0};
    if (strcmp(flag, "-felf64") == 0) {
        output_format = ELF64;
    } else if(strcmp(flag, "-fwin64") == 0) {
        output_format = WIN64;
    } else {
        usage(stderr);
        fprintf(stderr, "ERROR: unknown format flag.\n");
        exit(1);
    }

    // NOTE: The structure might be quite big due its arena. Better allocate it in the static memory.
    static Basm basm = {0};
    basm_translate_source(&basm, sv_from_cstr(argv[2]));

    printf("BITS 64\n");
    printf("%%define BM_STACK_CAPACITY %d\n", BM_STACK_CAPACITY);
    printf("%%define BM_WORD_SIZE %d\n", BM_WORD_SIZE);
    if (output_format == ELF64) {
        printf("%%define SYS_EXIT 60\n");
        printf("%%define SYS_WRITE 1\n");
        printf("%%define STDOUT 1\n");
    } else if (output_format == WIN64) {
        printf("extern GetStdHandle\n");
        printf("extern WriteFile\n");
        printf("extern ExitProcess\n");
    }
    printf("segment .text\n");
    printf("global _start\n");

    size_t jmp_if_escape_count = 0;
    for (size_t i = 0; i < basm.program_size; ++i) {
        Inst inst = basm.program[i];

        if (i == basm.entry) {
            printf("_start:\n");
        }

        for (size_t j = 0; j < BASM_BINDINGS_CAPACITY; ++j) {
            if (basm.bindings[j].kind != BINDING_LABEL) continue;

            if (basm.bindings[j].value.as_u64 == i) {
                printf("\n;; "SV_Fmt":\n", SV_Arg(basm.bindings[j].name));
                break;
            }
        }

        printf("inst_%zu:\n", i);
        switch (inst.type) {
        case INST_NOP: assert(false && "NOP is not implemented");
        case INST_PUSH: {
            printf("    ;; push %"PRIu64"\n", inst.operand.as_u64);
            printf("    mov rsi, [stack_top]\n");
            printf("    mov rax, 0x%"PRIX64"\n", inst.operand.as_u64);
            printf("    mov QWORD [rsi], rax\n");
            printf("    add QWORD [stack_top], BM_WORD_SIZE\n");
        } break;
        case INST_DROP: {
            printf("    ;; drop\n");
            printf("    mov rsi, [stack_top]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov [stack_top], rsi\n");
        } break;
        case INST_DUP: {
            printf("    ;; dup %"PRIu64"\n", inst.operand.as_u64);
            printf("    mov rsi, [stack_top]\n");
            printf("    mov rdi, rsi\n");
            printf("    sub rdi, BM_WORD_SIZE * (%"PRIu64" + 1)\n", inst.operand.as_u64);
            printf("    mov rax, [rdi]\n");
            printf("    mov [rsi], rax\n");
            printf("    add rsi, BM_WORD_SIZE\n");
            printf("    mov [stack_top], rsi\n");
        } break;
        case INST_SWAP: {
            printf("    ;; swap %"PRIu64"\n", inst.operand.as_u64);
            printf("    mov rsi, [stack_top]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov rdi, rsi\n");
            printf("    sub rdi, BM_WORD_SIZE * %"PRIu64"\n", inst.operand.as_u64);
            printf("    mov rax, [rsi]\n");
            printf("    mov rbx, [rdi]\n");
            printf("    mov [rdi], rax\n");
            printf("    mov [rsi], rbx\n");
        } break;
        case INST_PLUSI: {
            printf("    ;; plusi\n");
            printf("    mov rsi, [stack_top]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov rbx, [rsi]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov rax, [rsi]\n");
            printf("    add rax, rbx\n");
            printf("    mov [rsi], rax\n");
            printf("    add rsi, BM_WORD_SIZE\n");
            printf("    mov [stack_top], rsi\n");
        } break;
        case INST_MINUSI: {
            printf("    ;; minusi\n");
            printf("    mov rsi, [stack_top]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov rbx, [rsi]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov rax, [rsi]\n");
            printf("    sub rax, rbx\n");
            printf("    mov [rsi], rax\n");
            printf("    add rsi, BM_WORD_SIZE\n");
            printf("    mov [stack_top], rsi\n");
        } break;
        case INST_MULTI: {
            printf("    ;; multi\n");
            printf("    mov r11, [stack_top]\n");
            printf("    sub r11, BM_WORD_SIZE\n");
            printf("    mov [stack_top], r11\n");
            printf("    mov rax, [r11]\n");
            printf("    sub r11, BM_WORD_SIZE\n");
            printf("    mov rbx, [r11]\n");
            printf("    imul rax, rbx\n");
            printf("    mov [r11], rax\n");
        } break;
        case INST_MULTU: assert(false && "MULTU is not implemented");
        case INST_DIVI: {
            printf("    ;; divi\n");
            printf("    mov rsi, [stack_top]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov rbx, [rsi]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov rax, [rsi]\n");
            printf("    xor rdx, rdx\n");
            printf("    idiv rbx\n");
            printf("    mov [rsi], rax\n");
            printf("    add rsi, BM_WORD_SIZE\n");
            printf("    mov [stack_top], rsi\n");
        } break;
        case INST_DIVU: {
            printf("    ;; divu\n");
            printf("    mov rsi, [stack_top]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov rbx, [rsi]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov rax, [rsi]\n");
            printf("    xor rdx, rdx\n");
            printf("    div rbx\n");
            printf("    mov [rsi], rax\n");
            printf("    add rsi, BM_WORD_SIZE\n");
            printf("    mov [stack_top], rsi\n");
        } break;
        case INST_MODI: {
            printf("    ;; modi\n");
            printf("    mov rsi, [stack_top]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov rbx, [rsi]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov rax, [rsi]\n");
            printf("    xor rdx, rdx\n");
            printf("    idiv rbx\n");
            printf("    mov [rsi], rdx\n");
            printf("    add rsi, BM_WORD_SIZE\n");
            printf("    mov [stack_top], rsi\n");
        } break;

        case INST_MODU: {
            printf("    ;; modu\n");
            printf("    mov rsi, [stack_top]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov rbx, [rsi]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov rax, [rsi]\n");
            printf("    xor rdx, rdx\n");
            printf("    div rbx\n");
            printf("    mov [rsi], rdx\n");
            printf("    add rsi, BM_WORD_SIZE\n");
            printf("    mov [stack_top], rsi\n");
        } break;
        case INST_PLUSF: {
            printf("    ;; plusf\n");
            printf("    mov r11, [stack_top]\n");
            printf("    sub r11, BM_WORD_SIZE\n");
            printf("    mov [stack_top], r11\n");
            printf("    movsd xmm0, [r11]\n");
            printf("    sub r11, BM_WORD_SIZE\n");
            printf("    movsd xmm1, [r11]\n");
            printf("    addsd xmm1, xmm0\n");
            printf("    movsd [r11], xmm1\n");
        } break;
        case INST_MINUSF: {
            printf("    ;; minusf\n");
            printf("    mov r11, [stack_top]\n");
            printf("    sub r11, BM_WORD_SIZE\n");
            printf("    mov [stack_top], r11\n");
            printf("    movsd xmm0, [r11]\n");
            printf("    sub r11, BM_WORD_SIZE\n");
            printf("    movsd xmm1, [r11]\n");
            printf("    subsd xmm1, xmm0\n");
            printf("    movsd [r11], xmm1\n");
        } break;
        case INST_MULTF: {
            printf("    ;; multf\n");
            printf("    mov r11, [stack_top]\n");
            printf("    sub r11, BM_WORD_SIZE\n");
            printf("    mov [stack_top], r11\n");
            printf("    movsd xmm0, [r11]\n");
            printf("    sub r11, BM_WORD_SIZE\n");
            printf("    movsd xmm1, [r11]\n");
            printf("    mulsd xmm1, xmm0\n");
            printf("    movsd [r11], xmm1\n");
        } break;
        case INST_DIVF: {
            printf("    ;; divf\n");
            printf("    mov r11, [stack_top]\n");
            printf("    sub r11, BM_WORD_SIZE\n");
            printf("    mov [stack_top], r11\n");
            printf("    movsd xmm0, [r11]\n");
            printf("    sub r11, BM_WORD_SIZE\n");
            printf("    movsd xmm1, [r11]\n");
            printf("    divsd xmm1, xmm0\n");
            printf("    movsd [r11], xmm1\n");
        } break;
        case INST_JMP: {
            printf("    ;; jmp\n");
            printf("    mov rdi, inst_map\n");
            printf("    add rdi, BM_WORD_SIZE * %"PRIu64"\n", inst.operand.as_u64);
            printf("    jmp [rdi]\n");
        } break;
        case INST_JMP_IF: {
            printf("    ;; jmp_if %"PRIu64"\n", inst.operand.as_u64);
            printf("    mov rsi, [stack_top]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov rax, [rsi]\n");
            printf("    mov [stack_top], rsi\n");
            printf("    cmp rax, 0\n");
            printf("    je jmp_if_escape_%zu\n", jmp_if_escape_count);
            printf("    mov rdi, inst_map\n");
            printf("    add rdi, BM_WORD_SIZE * %"PRIu64"\n", inst.operand.as_u64);
            printf("    jmp [rdi]\n");
            printf("jmp_if_escape_%zu:\n", jmp_if_escape_count);
            jmp_if_escape_count += 1;
        } break;
        case INST_RET: {
            printf("    ;; ret\n");
            printf("    mov rsi, [stack_top]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov rax, [rsi]\n");
            printf("    mov rbx, BM_WORD_SIZE\n");
            printf("    mul rbx\n");
            printf("    add rax, inst_map\n");
            printf("    mov [stack_top], rsi\n");
            printf("    jmp [rax]\n");
        } break;
        case INST_CALL: {
            printf("    ;; call\n");
            printf("    mov rsi, [stack_top]\n");
            printf("    mov QWORD [rsi], %zu\n", i + 1);
            printf("    add rsi, BM_WORD_SIZE\n");
            printf("    mov [stack_top], rsi\n");
            printf("    mov rdi, inst_map\n");
            printf("    add rdi, BM_WORD_SIZE * %"PRIu64"\n", inst.operand.as_u64);
            printf("    jmp [rdi]\n");
        } break;
        case INST_NATIVE: {
            if (inst.operand.as_u64 == 3) {
                printf("    ;; native print_i64\n");
                printf("    call print_i64\n");
            } else if (inst.operand.as_u64 == 7) {
                printf("    ;; native write\n");
                if (output_format == ELF64) {
                    printf("    mov r11, [stack_top]\n");
                    printf("    sub r11, BM_WORD_SIZE\n");
                    printf("    mov rdx, [r11]\n");
                    printf("    sub r11, BM_WORD_SIZE\n");
                    printf("    mov rsi, [r11]\n");
                    printf("    add rsi, memory\n");
                    printf("    mov rdi, STDOUT\n");
                    printf("    mov rax, SYS_WRITE\n");
                    printf("    mov [stack_top], r11\n");
                    printf("    syscall\n");
                } else if (output_format == WIN64) {
                    printf("    sub rsp, 40\n");
                    printf("    mov ecx, STD_OUTPUT_HANDLE\n");
                    printf("    call GetStdHandle\n");
                    printf("    mov DWORD [stdout_handler], eax\n");
                    printf("    xor r9, r9\n");
                    printf("    mov r11, [stack_top]\n");
                    printf("    sub r11, BM_WORD_SIZE\n");
                    printf("    mov r8, [r11]\n");
                    printf("    sub r11, BM_WORD_SIZE\n");
                    printf("    mov rdx, [r11]\n");
                    printf("    add rdx, memory\n");
                    printf("    xor ecx, ecx\n");
                    printf("    mov ecx, dword [stdout_handler]\n");
                    printf("    mov [stack_top], r11\n");
                    printf("    call WriteFile\n");
                    printf("    add rsp, 40\n");
                }
            } else {
                assert(false && "unsupported native function");
            }
        } break;
        case INST_HALT: {
            printf("    ;; halt\n");
            if (output_format == ELF64) {
                printf("    mov rax, SYS_EXIT\n");
                printf("    mov rdi, 0\n");
                printf("    syscall\n");
            } else if (output_format == WIN64) {
                printf("    push dword 0\n");
                printf("    call ExitProcess\n");
            }
        } break;
        case INST_NOT: {
            printf("    ;; not\n");
            printf("    mov rsi, [stack_top]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov rax, [rsi]\n");
            printf("    cmp rax, 0\n");
            printf("    mov rax, 0\n");
            printf("    setz al\n");
            printf("    mov [rsi], rax\n");
        } break;
        case INST_EQI: {
            printf("    ;; eqi\n");
            printf("    mov rsi, [stack_top]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov rbx, [rsi]\n");
            printf("    sub rsi, BM_WORD_SIZE\n");
            printf("    mov rax, [rsi]\n");
            printf("    cmp rax, rbx\n");
            printf("    mov rax, 0\n");
            printf("    setz al\n");
            printf("    mov [rsi], rax\n");
            printf("    add rsi, BM_WORD_SIZE\n");
            printf("    mov [stack_top], rsi\n");
        } break;
        case INST_GEI: {
            printf("    ;; FIXME: gei\n");
        } break;
        case INST_GTI: {
            printf("    ;; FIXME: gti\n");
        } break;
        case INST_LEI: {
            printf("    ;; FIXME: lei\n");
        } break;
        case INST_LTI: assert(false && "LTI is not implemented");
        case INST_NEI: assert(false && "NEI is not implemented");
        case INST_EQU: {
            printf("    ;; FIXME: equ\n");
        } break;
        case INST_GEU: assert(false && "GEU is not implemented");
        case INST_GTU: assert(false && "GTU is not implemented");
        case INST_LEU: assert(false && "LEU is not implemented");
        case INST_LTU: assert(false && "LTU is not implemented");
        case INST_NEU: assert(false && "NEU is not implemented");
        case INST_EQF: assert(false && "EQF is not implemented");
        case INST_GEF: {
            printf("    ;; FIXME: gef\n");
        } break;
        case INST_GTF: {
            printf("    ;; FIXME: gtf\n");
        } break;
        case INST_LEF: {
            printf("    ;; FIXME: lef\n");
        } break;
        case INST_LTF: {
            printf("    ;; FIXME: ltf\n");
        } break;
        case INST_NEF: assert(false && "NEF is not implemented");
        case INST_ANDB: {
            printf("    ;; FIXME: andb\n");
        } break;
        case INST_ORB: assert(false && "ORB is not implemented");
        case INST_XOR: {
            printf("    ;; FIXME: xor\n");
        } break;
        case INST_SHR: assert(false && "SHR is not implemented");
        case INST_SHL: assert(false && "SHL is not implemented");
        case INST_NOTB: assert(false && "NOTB is not implemented");
        case INST_READ8: {
            printf("    ;; read8\n");
            printf("    mov r11, [stack_top]\n");
            printf("    sub r11, BM_WORD_SIZE\n");
            printf("    mov rsi, [r11]\n");
            printf("    add rsi, memory\n");
            printf("    xor rax, rax\n");
            printf("    mov al, BYTE [rsi]\n");
            printf("    mov [r11], rax\n");
        } break;
        case INST_READ16: assert(false && "READ16 is not implemented");
        case INST_READ32: assert(false && "READ32 is not implemented");
        case INST_READ64: assert(false && "READ64 is not implemented");
        case INST_WRITE8: {
            printf("    ;; write8\n");
            printf("    mov r11, [stack_top]\n");
            printf("    sub r11, BM_WORD_SIZE\n");
            printf("    mov rax, [r11]\n");
            printf("    sub r11, BM_WORD_SIZE\n");
            printf("    mov rsi, [r11]\n");
            printf("    add rsi, memory\n");
            printf("    mov BYTE [rsi], al\n");
            printf("    mov [stack_top], r11\n");
        } break;
        case INST_WRITE16: assert(false && "WRITE16 is not implemented");
        case INST_WRITE32: assert(false && "WRITE32 is not implemented");
        case INST_WRITE64: assert(false && "WRITE64 is not implemented");
        case INST_I2F: {
            printf("    ;; FIXME: i2f\n");
        } break;
        case INST_U2F: assert(false && "U2F is not implemented");
        case INST_F2I: {
            printf("    ;; FIXME: f2i\n");
        } break;
        case INST_F2U: assert(false && "F2U is not implemented");
        case NUMBER_OF_INSTS:
        default: assert(false && "unknown instruction");
        }
    }

    printf("    ret\n");
    printf("segment .data\n");
    printf("stack_top: dq stack\n");
    printf("inst_map:\n");
#define ROW_SIZE 5
#define ROW_COUNT(size) ((size + ROW_SIZE - 1) / ROW_SIZE)
#define INDEX(row, col) ((row) * ROW_SIZE + (col))
    for (size_t row = 0; row < ROW_COUNT(basm.program_size); ++row) {
        printf("  dq");
        for (size_t col = 0; col < ROW_SIZE && INDEX(row, col) < basm.program_size; ++col) {
            printf(" inst_%zu,", INDEX(row, col));
        }
        printf("\n");
    }
    printf("\n");
    if (output_format == WIN64) {
        printf("stdout_handler: dd 0\n");
    }
    printf("memory:\n");
    for (size_t row = 0; row < ROW_COUNT(basm.memory_size); ++row) {
        printf("  db");
        for (size_t col = 0; col < ROW_SIZE && INDEX(row, col) < basm.memory_size; ++col) {
            printf(" %u,", basm.memory[INDEX(row, col)]);
        }
        printf("\n");
    }
    printf("  times %zu db 0", BM_MEMORY_CAPACITY - basm.memory_size);
#undef ROW_SIZE
#undef ROW_COUNT
    printf("\n");
    printf("segment .bss\n");
    if (output_format == WIN64) {
        printf("STD_OUTPUT_HANDLE equ -11\n");
    }
    printf("stack: resq BM_STACK_CAPACITY\n");

    return 0;
}
