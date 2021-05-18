#include "basm.h"

void basm_save_to_file_as_nasm_sysv_x86_64(Basm *basm, Syscall_Target target, const char *output_file_path)
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

    switch (target) {
    case SYSCALLTARGET_LINUX: {
        fprintf(output, "%%define SYS_EXIT 60\n");
        fprintf(output, "%%define SYS_WRITE 1\n");
    }
    break;
    case SYSCALLTARGET_FREEBSD: {
        fprintf(output, "%%define SYS_EXIT 1\n");
        fprintf(output, "%%define SYS_WRITE 4\n");
    }
    break;
    }

    fprintf(output, "%%define STDOUT 1\n");
    fprintf(output, "segment .text\n");
    fprintf(output, "global _start\n");

    size_t jmp_count = 0;
    for (size_t i = 0; i < basm->program_size; ++i) {
        Inst inst = basm->program[i];

        if (i == basm->entry) {
            fprintf(output, "_start:\n");
        }

        fprintf(output, "inst_%zu:\n", i);
        switch (inst.type) {
        case INST_NOP: {
            fprintf(output, "    ;; nop\n");
            fprintf(output, "    nop\n");
        }
        break;
        case INST_PUSH: {
            fprintf(output, "    ;; push %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    mov rax, 0x%"PRIX64"\n", inst.operand.as_u64);
            fprintf(output, "    mov QWORD [rsi], rax\n");
            fprintf(output, "    add QWORD [stack_top], BM_WORD_SIZE\n");
        }
        break;
        case INST_DROP: {
            fprintf(output, "    ;; drop\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_DUP: {
            fprintf(output, "    ;; dup %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    mov rdi, rsi\n");
            fprintf(output, "    sub rdi, BM_WORD_SIZE * (%"PRIu64" + 1)\n", inst.operand.as_u64);
            fprintf(output, "    mov rax, [rdi]\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
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
        }
        break;
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
        }
        break;
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
        }
        break;
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
        }
        break;
        case INST_MULTU: {
            fprintf(output, "    ;; multu\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], r11\n");
            fprintf(output, "    mov rax, [r11]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [r11]\n");
            fprintf(output, "    mul rbx\n");
            fprintf(output, "    mov [r11], rax\n");
        }
        break;
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
        }
        break;
        case INST_DIVU: {
            fprintf(output, "    ;; divu\n");
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
        }
        break;
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
        }
        break;
        case INST_MODU: {
            fprintf(output, "    ;; modu\n");
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
        }
        break;
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
        }
        break;
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
        }
        break;
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
        }
        break;
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
        }
        break;
        case INST_JMP: {
            fprintf(output, "    ;; jmp\n");
            fprintf(output, "    mov rdi, inst_map\n");
            fprintf(output, "    add rdi, BM_WORD_SIZE * %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    jmp [rdi]\n");
        }
        break;
        case INST_JMP_IF: {
            fprintf(output, "    ;; jmp_if %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    mov [stack_top], rsi\n");
            fprintf(output, "    cmp rax, 0\n");
            fprintf(output, "    je jmp_if_escape_%zu\n", jmp_count);
            fprintf(output, "    mov rdi, inst_map\n");
            fprintf(output, "    add rdi, BM_WORD_SIZE * %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    jmp [rdi]\n");
            fprintf(output, "jmp_if_escape_%zu:\n", jmp_count);
            jmp_count += 1;
        }
        break;
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
        }
        break;
        case INST_CALL: {
            fprintf(output, "    ;; call\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    mov QWORD [rsi], %zu\n", i + 1);
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
            fprintf(output, "    mov rdi, inst_map\n");
            fprintf(output, "    add rdi, BM_WORD_SIZE * %"PRIu64"\n", inst.operand.as_u64);
            fprintf(output, "    jmp [rdi]\n");
        }
        break;
        case INST_NATIVE: {
            if (inst.operand.as_u64 == 0) {
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
        }
        break;
        case INST_HALT: {
            fprintf(output, "    ;; halt\n");
            fprintf(output, "    mov rax, SYS_EXIT\n");
            fprintf(output, "    mov rdi, 0\n");
            fprintf(output, "    syscall\n");
        }
        break;
        case INST_NOT: {
            fprintf(output, "    ;; not\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    cmp rax, 0\n");
            fprintf(output, "    mov rax, 0\n");
            fprintf(output, "    setz al\n");
            fprintf(output, "    mov [rsi], rax\n");
        }
        break;
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
        }
        break;
        case INST_GEI: {
            fprintf(output, "    ;; gei\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    cmp rbx, rax\n");
            fprintf(output, "    setge al\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_GTI: {
            fprintf(output, "    ;; gti\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    cmp rbx, rax\n");
            fprintf(output, "    setg al\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_LEI: {
            fprintf(output, "    ;; lei\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    cmp rbx, rax\n");
            fprintf(output, "    setle al\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_LTI: {
            fprintf(output, "    ;; lti\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    cmp rbx, rax\n");
            fprintf(output, "    setl al\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_NEI: {
            fprintf(output, "    ;; equ\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    cmp rbx, rax\n");
            fprintf(output, "    setne al\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_EQU: {
            fprintf(output, "    ;; equ\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    cmp rbx, rax\n");
            fprintf(output, "    sete al\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_GEU: {
            fprintf(output, "    ;; geu\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    cmp rbx, rax\n");
            fprintf(output, "    setnb al\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_GTU: {
            fprintf(output, "    ;; gtu\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    cmp rbx, rax\n");
            fprintf(output, "    seta al\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_LEU: {
            fprintf(output, "    ;; leu\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    cmp rbx, rax\n");
            fprintf(output, "    setbe al\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_LTU: {
            fprintf(output, "    ;; ltu\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    cmp rbx, rax\n");
            fprintf(output, "    setb al\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_NEU: {
            fprintf(output, "    ;; neu\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    cmp rax, rbx\n");
            fprintf(output, "    setne al\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_EQF: {
            fprintf(output, "    ;; eqf\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    movsd xmm0, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    movsd xmm1, [rsi]\n");
            fprintf(output, "    ucomisd xmm0, xmm1\n");
            fprintf(output, "    mov edx, 0\n");
            fprintf(output, "    setnp al\n");
            fprintf(output, "    cmovne eax, edx\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_GEF: {
            fprintf(output, "    ;; gef\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    movsd xmm0, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    movsd xmm1, [rsi]\n");
            fprintf(output, "    comisd xmm1, xmm0\n");
            fprintf(output, "    setae al\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_GTF: {
            fprintf(output, "    ;; gtf\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    movsd xmm0, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    movsd xmm1, [rsi]\n");
            fprintf(output, "    comisd xmm1, xmm0\n");
            fprintf(output, "    seta al\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_LEF: {
            fprintf(output, "    ;; lef\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    movsd xmm0, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    movsd xmm1, [rsi]\n");
            fprintf(output, "    comisd xmm0, xmm1\n");
            fprintf(output, "    setnb al\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_LTF: {
            fprintf(output, "    ;; ltf\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    movsd xmm0, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    movsd xmm1, [rsi]\n");
            fprintf(output, "    comisd xmm0, xmm1\n");
            fprintf(output, "    seta al\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_NEF: {
            fprintf(output, "    ;; ltf\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    movsd xmm0, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    movsd xmm1, [rsi]\n");
            fprintf(output, "    ucomisd xmm1, xmm0\n");
            fprintf(output, "    mov edx, 1\n");
            fprintf(output, "    setp al\n");
            fprintf(output, "    cmovne eax, edx\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_ANDB: {
            fprintf(output, "    ;; andb\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    and rax, rbx\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_ORB: {
            fprintf(output, "    ;; orb\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    or rax, rbx\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_XOR: {
            fprintf(output, "    ;; xor\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rbx, [rsi]\n");
            fprintf(output, "    xor rax, rbx\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_SHR: {
            fprintf(output, "    ;; shr\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rcx, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    shr rax, cl\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_SHL: {
            fprintf(output, "    ;; shl\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rcx, [rsi]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    shl rax, cl\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_NOTB: {
            fprintf(output, "    ;; notb\n");
            fprintf(output, "    mov rsi, [stack_top]\n");
            fprintf(output, "    sub rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [rsi]\n");
            fprintf(output, "    not rax\n");
            fprintf(output, "    mov [rsi], rax\n");
            fprintf(output, "    add rsi, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], rsi\n");
        }
        break;
        case INST_READ8I: {
            fprintf(output, "    ;; read8i\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rsi, [r11]\n");
            fprintf(output, "    add rsi, memory\n");
            fprintf(output, "    xor rax, rax\n");
            fprintf(output, "    mov al, BYTE [rsi]\n");
            fprintf(output, "    movsx rax, al\n");
            fprintf(output, "    mov [r11], rax\n");
        }
        break;
        case INST_READ8U: {
            fprintf(output, "    ;; read8\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rsi, [r11]\n");
            fprintf(output, "    add rsi, memory\n");
            fprintf(output, "    xor rax, rax\n");
            fprintf(output, "    mov al, BYTE [rsi]\n");
            fprintf(output, "    mov [r11], rax\n");
        }
        break;
        case INST_READ16I: {
            fprintf(output, "    ;; read16i\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rsi, [r11]\n");
            fprintf(output, "    add rsi, memory\n");
            fprintf(output, "    xor rax, rax\n");
            fprintf(output, "    mov ax, WORD [rsi]\n");
            fprintf(output, "    movsx rax, ax\n");
            fprintf(output, "    mov [r11], rax\n");
        }
        break;
        case INST_READ16U: {
            fprintf(output, "    ;; read16u\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rsi, [r11]\n");
            fprintf(output, "    add rsi, memory\n");
            fprintf(output, "    xor rax, rax\n");
            fprintf(output, "    mov ax, WORD [rsi]\n");
            fprintf(output, "    mov [r11], rax\n");
        }
        break;
        case INST_READ32I: {
            fprintf(output, "    ;; read32i\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rsi, [r11]\n");
            fprintf(output, "    add rsi, memory\n");
            fprintf(output, "    xor rax, rax\n");
            fprintf(output, "    mov eax, DWORD [rsi]\n");
            fprintf(output, "    movsx rax, eax\n");
            fprintf(output, "    mov [r11], rax\n");
        }
        break;
        case INST_READ32U: {
            fprintf(output, "    ;; read32u\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rsi, [r11]\n");
            fprintf(output, "    add rsi, memory\n");
            fprintf(output, "    xor rax, rax\n");
            fprintf(output, "    mov eax, DWORD [rsi]\n");
            fprintf(output, "    mov [r11], rax\n");
        }
        break;
        case INST_READ64I:
        case INST_READ64U: {
            fprintf(output, "    ;; read64\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rsi, [r11]\n");
            fprintf(output, "    add rsi, memory\n");
            fprintf(output, "    xor rax, rax\n");
            fprintf(output, "    mov rax, QWORD [rsi]\n");
            fprintf(output, "    mov [r11], rax\n");
        }
        break;
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
        }
        break;
        case INST_WRITE16: {
            fprintf(output, "    ;; write16\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [r11]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rsi, [r11]\n");
            fprintf(output, "    add rsi, memory\n");
            fprintf(output, "    mov WORD [rsi], ax\n");
            fprintf(output, "    mov [stack_top], r11\n");
        }
        break;
        case INST_WRITE32: {
            fprintf(output, "    ;; write32\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [r11]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rsi, [r11]\n");
            fprintf(output, "    add rsi, memory\n");
            fprintf(output, "    mov DWORD [rsi], eax\n");
            fprintf(output, "    mov [stack_top], r11\n");
        }
        break;
        case INST_WRITE64: {
            fprintf(output, "    ;; write64\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rax, [r11]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rsi, [r11]\n");
            fprintf(output, "    add rsi, memory\n");
            fprintf(output, "    mov QWORD [rsi], rax\n");
            fprintf(output, "    mov [stack_top], r11\n");
        }
        break;
        case INST_I2F: {
            fprintf(output, "    ;; i2f\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rdi, [r11]\n");
            fprintf(output, "    pxor xmm0, xmm1\n");
            fprintf(output, "    cvtsi2sd xmm0, rdi\n");
            fprintf(output, "    movsd [r11], xmm0\n");
            fprintf(output, "    add r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], r11\n");
        }
        break;
        case INST_U2F: {
            fprintf(output, "    ;; u2f\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov rdi, [r11]\n");
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
            fprintf(output, "    movsd [r11], xmm0\n");
            fprintf(output, "    add r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], r11\n");
            jmp_count += 1;
        }
        break;
        case INST_F2I: {
            fprintf(output, "    ;; f2i\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    movsd xmm0, [r11]\n");
            fprintf(output, "    cvttsd2si rax, xmm0\n");
            fprintf(output, "    mov [r11], rax\n");
            fprintf(output, "    add r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], r11\n");
        }
        break;
        // TODO(#349): try to get rid of branching in f2u implementation of basm_save_to_file_as_nasm()
        case INST_F2U: {
            fprintf(output, "    ;; f2i\n");
            fprintf(output, "    mov r11, [stack_top]\n");
            fprintf(output, "    sub r11, BM_WORD_SIZE\n");
            fprintf(output, "    movsd xmm0, [r11]\n");
            fprintf(output, "    movsd xmm1, [magic_number_for_f2u]\n");
            fprintf(output, "    comisd xmm0, xmm1\n");
            fprintf(output, "    jnb .f2u_%zu\n", jmp_count);
            fprintf(output, "    cvttsd2si rax, xmm0\n");
            fprintf(output, "    jmp .f2u_end_%zu\n", jmp_count);
            fprintf(output, "    .f2u_%zu:\n", jmp_count);
            fprintf(output, "    subsd xmm0, xmm1\n");
            fprintf(output, "    cvttsd2si rax, xmm0\n");
            fprintf(output, "    btc rax, 63\n");
            fprintf(output, "    .f2u_end_%zu:\n", jmp_count);
            fprintf(output, "    mov [r11], rax\n");
            fprintf(output, "    add r11, BM_WORD_SIZE\n");
            fprintf(output, "    mov [stack_top], r11\n");
        }
        break;
        case NUMBER_OF_INSTS:
        default:
            assert(false && "unknown instruction");
        }
    }

    fprintf(output, "    ret\n");
    fprintf(output, "segment .data\n");
    fprintf(output, "magic_number_for_f2u: dq 0x43E0000000000000\n");
    fprintf(output, "stack_top: dq stack\n");
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
    fprintf(output, "stack: resq BM_STACK_CAPACITY\n");

    fclose(output);
}
