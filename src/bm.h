#ifndef BM_H_
#define BM_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#define ARRAY_SIZE(xs) (sizeof(xs) / sizeof((xs)[0]))
#define BM_STACK_CAPACITY 1024
#define BM_PROGRAM_CAPACITY 1024
#define LABEL_CAPACITY 1024
#define DEFERRED_OPERANDS_CAPACITY 1024

typedef enum {
    ERR_OK = 0,
    ERR_STACK_OVERFLOW,
    ERR_STACK_UNDERFLOW,
    ERR_ILLEGAL_INST,
    ERR_ILLEGAL_INST_ACCESS,
    ERR_ILLEGAL_OPERAND,
    ERR_DIV_BY_ZERO,
} Err;

const char *err_as_cstr(Err err);

typedef enum {
    INST_NOP = 0,
    INST_PUSH,
    INST_DUP,
    INST_PLUS,
    INST_MINUS,
    INST_MULT,
    INST_DIV,
    INST_JMP,
    INST_JMP_IF,
    INST_EQ,
    INST_HALT,
    INST_PRINT_DEBUG,
} Inst_Type;

const char *inst_type_as_cstr(Inst_Type type);

typedef uint64_t Inst_Addr;

typedef union {
    uint64_t as_u64;
    int64_t as_i64;
    double as_f64;
    void *as_ptr;
} Word;

static_assert(sizeof(Word) == 8,
              "The BM's Word is expected to be 64 bits");

typedef struct {
    Inst_Type type;
    Word operand;
} Inst;

typedef struct {
    Word stack[BM_STACK_CAPACITY];
    uint64_t stack_size;

    Inst program[BM_PROGRAM_CAPACITY];
    uint64_t program_size;
    Inst_Addr ip;

    int halt;
} Bm;

Err bm_execute_inst(Bm *bm);
Err bm_execute_program(Bm *bm, int limit);
void bm_dump_stack(FILE *stream, const Bm *bm);
void bm_load_program_from_memory(Bm *bm, Inst *program, size_t program_size);
void bm_load_program_from_file(Bm *bm, const char *file_path);
void bm_save_program_to_file(const Bm *bm, const char *file_path);

typedef struct {
    size_t count;
    const char *data;
} String_View;

String_View cstr_as_sv(const char *cstr);
String_View sv_trim_left(String_View sv);
String_View sv_trim_right(String_View sv);
String_View sv_trim(String_View sv);
String_View sv_chop_by_delim(String_View *sv, char delim);
int sv_eq(String_View a, String_View b);
int sv_to_int(String_View sv);
String_View sv_slurp_file(const char *file_path);

typedef struct {
    String_View name;
    Inst_Addr addr;
} Label;

typedef struct {
    Inst_Addr addr;
    String_View label;
} Deferred_Operand;

typedef struct {
    Label labels[LABEL_CAPACITY];
    size_t labels_size;
    Deferred_Operand deferred_operands[DEFERRED_OPERANDS_CAPACITY];
    size_t deferred_operands_size;
} Basm;

Inst_Addr basm_find_label_addr(const Basm *basm, String_View name);
void basm_push_label(Basm *basm, String_View name, Inst_Addr addr);
void basm_push_deferred_operand(Basm *basm, Inst_Addr addr, String_View label);

void bm_translate_source(String_View source, Bm *bm, Basm *basm);

#endif  // BM_H_

#ifdef BM_IMPLEMENTATION

const char *err_as_cstr(Err err)
{
    switch (err) {
    case ERR_OK:
        return "ERR_OK";
    case ERR_STACK_OVERFLOW:
        return "ERR_STACK_OVERFLOW";
    case ERR_STACK_UNDERFLOW:
        return "ERR_STACK_UNDERFLOW";
    case ERR_ILLEGAL_INST:
        return "ERR_ILLEGAL_INST";
    case ERR_ILLEGAL_OPERAND:
        return "ERR_ILLEGAL_OPERAND";
    case ERR_ILLEGAL_INST_ACCESS:
        return "ERR_ILLEGAL_INST_ACCESS";
    case ERR_DIV_BY_ZERO:
        return "ERR_DIV_BY_ZERO";
    default:
        assert(0 && "err_as_cstr: Unreachable");
    }
}

const char *inst_type_as_cstr(Inst_Type type)
{
    switch (type) {
    case INST_NOP:         return "INST_NOP";
    case INST_PUSH:        return "INST_PUSH";
    case INST_PLUS:        return "INST_PLUS";
    case INST_MINUS:       return "INST_MINUS";
    case INST_MULT:        return "INST_MULT";
    case INST_DIV:         return "INST_DIV";
    case INST_JMP:         return "INST_JMP";
    case INST_HALT:        return "INST_HALT";
    case INST_JMP_IF:      return "INST_JMP_IF";
    case INST_EQ:          return "INST_EQ";
    case INST_PRINT_DEBUG: return "INST_PRINT_DEBUG";
    case INST_DUP:         return "INST_DUP";
    default: assert(0 && "inst_type_as_cstr: unreachable");
    }
}

Err bm_execute_program(Bm *bm, int limit)
{
    while (limit != 0 && !bm->halt) {
        Err err = bm_execute_inst(bm);
        if (err != ERR_OK) {
            return err;
        }
        if (limit > 0) {
            --limit;
        }
    }

    return ERR_OK;
}

Err bm_execute_inst(Bm *bm)
{
    if (bm->ip >= bm->program_size) {
        return ERR_ILLEGAL_INST_ACCESS;
    }

    Inst inst = bm->program[bm->ip];

    switch (inst.type) {
    case INST_NOP:
        bm->ip += 1;
        break;

    case INST_PUSH:
        if (bm->stack_size >= BM_STACK_CAPACITY) {
            return ERR_STACK_OVERFLOW;
        }
        bm->stack[bm->stack_size++] = inst.operand;
        bm->ip += 1;
        break;

    case INST_PLUS:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }
        bm->stack[bm->stack_size - 2].as_u64 += bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_MINUS:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }
        bm->stack[bm->stack_size - 2].as_u64 -= bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_MULT:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }
        bm->stack[bm->stack_size - 2].as_u64 *= bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_DIV:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }

        if (bm->stack[bm->stack_size - 1].as_u64 == 0) {
            return ERR_DIV_BY_ZERO;
        }

        bm->stack[bm->stack_size - 2].as_u64 /= bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_JMP:
        bm->ip = inst.operand.as_u64;
        break;

    case INST_HALT:
        bm->halt = 1;
        break;

    case INST_EQ:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->stack[bm->stack_size - 2].as_u64 = bm->stack[bm->stack_size - 1].as_u64 == bm->stack[bm->stack_size - 2].as_u64;
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_JMP_IF:
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }

        if (bm->stack[bm->stack_size - 1].as_u64) {
            bm->stack_size -= 1;
            bm->ip = inst.operand.as_u64;
        } else {
            bm->ip += 1;
        }
        break;

    case INST_PRINT_DEBUG:
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }
        printf("%lu\n", bm->stack[bm->stack_size - 1].as_u64);
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_DUP:
        if (bm->stack_size >= BM_STACK_CAPACITY) {
            return ERR_STACK_OVERFLOW;
        }

        if (bm->stack_size - inst.operand.as_u64 <= 0) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->stack[bm->stack_size] = bm->stack[bm->stack_size - 1 - inst.operand.as_u64];
        bm->stack_size += 1;
        bm->ip += 1;
        break;

    default:
        return ERR_ILLEGAL_INST;
    }

    return ERR_OK;
}


void bm_dump_stack(FILE *stream, const Bm *bm)
{
    fprintf(stream, "Stack:\n");
    if (bm->stack_size > 0) {
        for (Inst_Addr i = 0; i < bm->stack_size; ++i) {
            fprintf(stream, "  u64: %lu, i64: %ld, f64: %lf, ptr: %p\n",
                    bm->stack[i].as_u64,
                    bm->stack[i].as_i64,
                    bm->stack[i].as_f64,
                    bm->stack[i].as_ptr);
        }
    } else {
        fprintf(stream, "  [empty]\n");
    }
}

void bm_load_program_from_memory(Bm *bm, Inst *program, size_t program_size)
{
    assert(program_size < BM_PROGRAM_CAPACITY);
    memcpy(bm->program, program, sizeof(program[0]) * program_size);
    bm->program_size = program_size;
}


void bm_load_program_from_file(Bm *bm, const char *file_path)
{
    FILE *f = fopen(file_path, "rb");
    if (f == NULL) {
        fprintf(stderr, "ERROR: Could not open file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    if (fseek(f, 0, SEEK_END) < 0) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    long m = ftell(f);
    if (m < 0) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    assert(m % sizeof(bm->program[0]) == 0);
    assert((size_t) m <= BM_PROGRAM_CAPACITY * sizeof(bm->program[0]));

    if (fseek(f, 0, SEEK_SET) < 0) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    bm->program_size = fread(bm->program, sizeof(bm->program[0]), m / sizeof(bm->program[0]), f);

    if (ferror(f)) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    fclose(f);
}


void bm_save_program_to_file(const Bm *bm, const char *file_path)
{
    FILE *f = fopen(file_path, "wb");
    if (f == NULL) {
        fprintf(stderr, "ERROR: Could not open file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    fwrite(bm->program, sizeof(bm->program[0]), bm->program_size, f);

    if (ferror(f)) {
        fprintf(stderr, "ERROR: Could not write to file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    fclose(f);
}

String_View cstr_as_sv(const char *cstr)
{
    return (String_View) {
        .count = strlen(cstr),
        .data = cstr,
    };
}


String_View sv_trim_left(String_View sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[i])) {
        i += 1;
    }

    return (String_View) {
        .count = sv.count - i,
        .data = sv.data + i,
    };
}

String_View sv_trim_right(String_View sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[sv.count - 1 - i])) {
        i += 1;
    }

    return (String_View) {
        .count = sv.count - i,
        .data = sv.data
    };
}

String_View sv_trim(String_View sv)
{
    return sv_trim_right(sv_trim_left(sv));
}

String_View sv_chop_by_delim(String_View *sv, char delim)
{
    size_t i = 0;
    while (i < sv->count && sv->data[i] != delim) {
        i += 1;
    }

    String_View result = {
        .count = i,
        .data = sv->data,
    };

    if (i < sv->count) {
        sv->count -= i + 1;
        sv->data  += i + 1;
    } else {
        sv->count -= i;
        sv->data  += i;
    }

    return result;
}

int sv_eq(String_View a, String_View b)
{
    if (a.count != b.count) {
        return 0;
    } else {
        return memcmp(a.data, b.data, a.count) == 0;
    }
}

int sv_to_int(String_View sv)
{
    int result = 0;

    for (size_t i = 0; i < sv.count && isdigit(sv.data[i]); ++i) {
        result = result * 10 + sv.data[i] - '0';
    }

    return result;
}

Inst_Addr basm_find_label_addr(const Basm *basm, String_View name)
{
    for (size_t i = 0; i < basm->labels_size; ++i) {
        if (sv_eq(basm->labels[i].name, name)) {
            return basm->labels[i].addr;
        }
    }

    fprintf(stderr, "ERROR: label `%.*s` does not exist\n",
            (int) name.count, name.data);
    exit(1);
}

void basm_push_label(Basm *basm, String_View name, Inst_Addr addr)
{
    assert(basm->labels_size < LABEL_CAPACITY);
    basm->labels[basm->labels_size++] = (Label) {.name = name, .addr = addr};
}

void basm_push_deferred_operand(Basm *basm, Inst_Addr addr, String_View label)
{
    assert(basm->deferred_operands_size < DEFERRED_OPERANDS_CAPACITY);
    basm->deferred_operands[basm->deferred_operands_size++] =
        (Deferred_Operand) {.addr = addr, .label = label};
}

void bm_translate_source(String_View source, Bm *bm, Basm *basm)
{
    bm->program_size = 0;

    // First pass
    while (source.count > 0) {
        assert(bm->program_size < BM_PROGRAM_CAPACITY);
        String_View line = sv_trim(sv_chop_by_delim(&source, '\n'));
        if (line.count > 0 && *line.data != '#') {
            String_View inst_name = sv_chop_by_delim(&line, ' ');

            if (inst_name.count > 0 && inst_name.data[inst_name.count - 1] == ':') {
                String_View label = {
                    .count = inst_name.count - 1,
                    .data = inst_name.data
                };

                basm_push_label(basm, label, bm->program_size);

                inst_name = sv_trim(sv_chop_by_delim(&line, ' '));
            }

            if (inst_name.count > 0) {
                String_View operand = sv_trim(sv_chop_by_delim(&line, '#'));

                if (sv_eq(inst_name, cstr_as_sv("nop"))) {
                    bm->program[bm->program_size++] = (Inst) {
                        .type = INST_NOP,
                    };
                } else if (sv_eq(inst_name, cstr_as_sv("push"))) {
                    bm->program[bm->program_size++] = (Inst) {
                        .type = INST_PUSH,
                        .operand = { .as_i64 = sv_to_int(operand) }
                    };
                } else if (sv_eq(inst_name, cstr_as_sv("dup"))) {
                    bm->program[bm->program_size++] = (Inst) {
                        .type = INST_DUP,
                        .operand = { .as_i64 = sv_to_int(operand) }
                    };
                } else if (sv_eq(inst_name, cstr_as_sv("plus"))) {
                    bm->program[bm->program_size++] = (Inst) {
                        .type = INST_PLUS
                    };
                } else if (sv_eq(inst_name, cstr_as_sv("jmp"))) {
                    if (operand.count > 0 && isdigit(*operand.data)) {
                        bm->program[bm->program_size++] = (Inst) {
                            .type = INST_JMP,
                            .operand = { .as_i64 = sv_to_int(operand) },
                        };
                    } else {
                        basm_push_deferred_operand(
                            basm, bm->program_size, operand);

                        bm->program[bm->program_size++] = (Inst) {
                            .type = INST_JMP
                        };
                    }
                } else if (sv_eq(inst_name, cstr_as_sv("halt"))) {
                    bm->program[bm->program_size++] = (Inst) {
                        .type = INST_HALT
                    };
                } else {
                    fprintf(stderr, "ERROR: unknown instruction `%.*s`\n",
                            (int) inst_name.count, inst_name.data);
                    exit(1);
                }
            }
        }
    }

    // Second pass
    for (size_t i = 0; i < basm->deferred_operands_size; ++i) {
        Inst_Addr addr = basm_find_label_addr(basm, basm->deferred_operands[i].label);
        bm->program[basm->deferred_operands[i].addr].operand.as_u64 = addr;
    }
}

String_View sv_slurp_file(const char *file_path)
{
    FILE *f = fopen(file_path, "r");
    if (f == NULL) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    if (fseek(f, 0, SEEK_END) < 0) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    long m = ftell(f);
    if (m < 0) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    char *buffer = malloc(m);
    if (buffer == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for file: %s\n",
                strerror(errno));
        exit(1);
    }

    if (fseek(f, 0, SEEK_SET) < 0) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    size_t n = fread(buffer, 1, m, f);
    if (ferror(f)) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    fclose(f);

    return (String_View) {
        .count = n,
        .data = buffer,
    };
}

#endif // BM_IMPLEMENTATION
