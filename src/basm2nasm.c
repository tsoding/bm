#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define BM_IMPLEMENTATION
#include "bm.h"

static void usage(FILE *stream)
{
    fprintf(stream, "Usage: ./basm2nasm <input.basm>\n");
}

Basm basm = {0};

static void gen_print_i64(FILE *stream)
{
    fprintf(stream, "print_i64:\n");
    fprintf(stream, "    ;; extracting input from the BM's stack\n");
    fprintf(stream, "    mov rsi, [stack_top]\n");
    fprintf(stream, "    sub rsi, BM_WORD_SIZE\n");
    fprintf(stream, "    mov rax, [rsi]\n");
    fprintf(stream, "    mov [stack_top], rsi\n");
    fprintf(stream, "    ;; rax contains the value we need to print\n");
    fprintf(stream, "    ;; rdi is the counter of chars\n");
    fprintf(stream, "    mov rdi, 0\n");
    fprintf(stream, "    ;; adding the new line\n");
    fprintf(stream, "    dec rsp\n");
    fprintf(stream, "    inc rdi\n");
    fprintf(stream, "    mov BYTE [rsp], 10\n");
    fprintf(stream, ".loop:\n");
    fprintf(stream, "    xor rdx, rdx\n");
    fprintf(stream, "    mov rbx, 10\n");
    fprintf(stream, "    div rbx\n");
    fprintf(stream, "    add rdx, '0'\n");
    fprintf(stream, "    dec rsp\n");
    fprintf(stream, "    inc rdi\n");
    fprintf(stream, "    mov [rsp], dl\n");
    fprintf(stream, "    cmp rax, 0\n");
    fprintf(stream, "    jne .loop\n");
    fprintf(stream, "    ;; rsp - points at the beginning of the buf\n");
    fprintf(stream, "    ;; rdi - contains the size of the buf\n");
    fprintf(stream, "    ;; printing the buffer\n");
    fprintf(stream, "    mov rbx, rdi\n");
    fprintf(stream, "    ;; write(STDOUT, buf, buf_size)\n");
    fprintf(stream, "    mov rax, SYS_WRITE\n");
    fprintf(stream, "    mov rdi, STDOUT\n");
    fprintf(stream, "    mov rsi, rsp\n");
    fprintf(stream, "    mov rdx, rbx\n");
    fprintf(stream, "    syscall\n");
    fprintf(stream, "    add rsp, rbx\n");
    fprintf(stream, "    ret\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        usage(stderr);
        fprintf(stderr, "ERROR: no input provided.\n");
        exit(1);
    }

    basm_translate_source(&basm, sv_from_cstr(argv[1]));

    printf("BITS 64\n");
    printf("%%define BM_STACK_CAPACITY %d\n", BM_STACK_CAPACITY);
    printf("%%define BM_WORD_SIZE %d\n", BM_WORD_SIZE);
    printf("%%define SYS_EXIT 60\n");
    printf("%%define SYS_WRITE 1\n");
    printf("%%define STDOUT 1\n");
    printf("segment .text\n");
    printf("global _start\n");
    gen_print_i64(stdout);
    printf("_start:\n");

    size_t jmp_if_escape_count = 0;
    for (size_t i = 0; i < basm.program_size; ++i) {
        Inst inst = basm.program[i];

        printf("inst_%zu:\n", i);
        switch (inst.type) {
        case INST_NOP: assert(false && "NOP is not implemented");
        case INST_PUSH: {
            printf("    ;; push %"PRIu64"\n", inst.operand.as_u64);
            printf("    mov rsi, [stack_top]\n");
            printf("    mov QWORD [rsi], %"PRIu64"\n", inst.operand.as_u64);
            printf("    add QWORD [stack_top], BM_WORD_SIZE\n");
        } break;
        case INST_DROP: assert(false && "DROP is not implemented");
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
        case INST_MULTI: assert(false && "MULTI is not implemented");
        case INST_DIVI: assert(false && "DIVI is not implemented");
        case INST_MODI: assert(false && "MODI is not implemented");
        case INST_PLUSF: assert(false && "PLUSF is not implemented");
        case INST_MINUSF: assert(false && "MINUSF is not implemented");
        case INST_MULTF: assert(false && "MULTF is not implemented");
        case INST_DIVF: assert(false && "DIVF is not implemented");
        case INST_JMP: assert(false && "JMP is not implemented");
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
        case INST_RET: assert(false && "RET is not implemented");
        case INST_CALL: assert(false && "CALL is not implemented");
        case INST_NATIVE: {
            if (inst.operand.as_u64 == 3) {
                printf("    ;; native print_i64\n");
                printf("    call print_i64\n");
            } else {
                assert(false && "unsupported native function");
            }
        } break;
        case INST_HALT: {
            printf("    ;; halt\n");
            printf("    mov rax, SYS_EXIT\n");
            printf("    mov rdi, 0\n");
            printf("    syscall\n");
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
        case INST_GEI: assert(false && "GEI is not implemented");
        case INST_GTI: assert(false && "GTI is not implemented");
        case INST_LEI: assert(false && "LEI is not implemented");
        case INST_LTI: assert(false && "LTI is not implemented");
        case INST_NEI: assert(false && "NEI is not implemented");
        case INST_EQF: assert(false && "EQF is not implemented");
        case INST_GEF: assert(false && "GEF is not implemented");
        case INST_GTF: assert(false && "GTF is not implemented");
        case INST_LEF: assert(false && "LEF is not implemented");
        case INST_LTF: assert(false && "LTF is not implemented");
        case INST_NEF: assert(false && "NEF is not implemented");
        case INST_ANDB: assert(false && "ANDB is not implemented");
        case INST_ORB: assert(false && "ORB is not implemented");
        case INST_XOR: assert(false && "XOR is not implemented");
        case INST_SHR: assert(false && "SHR is not implemented");
        case INST_SHL: assert(false && "SHL is not implemented");
        case INST_NOTB: assert(false && "NOTB is not implemented");
        case INST_READ8: assert(false && "READ8 is not implemented");
        case INST_READ16: assert(false && "READ16 is not implemented");
        case INST_READ32: assert(false && "READ32 is not implemented");
        case INST_READ64: assert(false && "READ64 is not implemented");
        case INST_WRITE8: assert(false && "WRITE8 is not implemented");
        case INST_WRITE16: assert(false && "WRITE16 is not implemented");
        case INST_WRITE32: assert(false && "WRITE32 is not implemented");
        case INST_WRITE64: assert(false && "WRITE64 is not implemented");
        case INST_I2F: assert(false && "I2F is not implemented");
        case INST_U2F: assert(false && "U2F is not implemented");
        case INST_F2I: assert(false && "F2I is not implemented");
        case INST_F2U: assert(false && "F2U is not implemented");
        case NUMBER_OF_INSTS:
        default: assert(false && "unknown instruction");
        }
    }

    printf("    ret\n");
    printf("segment .data\n");
    printf("stack_top: dq stack\n");
    printf("inst_map: dq");
    for (size_t i = 0; i < basm.program_size; ++i) {
        printf(" inst_%zu,", i);
    }
    printf("\n");
    printf("segment .bss\n");
    printf("stack: resq BM_STACK_CAPACITY\n");

    return 0;
}
