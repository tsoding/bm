#ifndef BM_H_
#define BM_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>

#if defined(__GNUC__) || defined(__clang__)
#  define PACKED __attribute__((packed))
#else
#  warning "Packed attributes for struct is not implemented for this compiler. This may result in a program working incorrectly. Feel free to fix that and submit a Pull Request to https://github.com/tsoding/bm"
#  define PACKED
#endif

#define BM_STACK_CAPACITY 1024
#define BM_PROGRAM_CAPACITY 1024
#define BM_NATIVES_CAPACITY 1024
#define BM_MEMORY_CAPACITY (640 * 1000)

#define BASM_BINDINGS_CAPACITY 1024
#define BASM_DEFERRED_OPERANDS_CAPACITY 1024
#define BASM_COMMENT_SYMBOL ';'
#define BASM_PP_SYMBOL '%'
#define BASM_MAX_INCLUDE_LEVEL 69
#define BASM_ARENA_CAPACITY (1000 * 1000 * 1000)

typedef struct {
    size_t count;
    const char *data;
} String_View;

#define SV_FORMAT(sv) (int) sv.count, sv.data

String_View sv_from_cstr(const char *cstr);
String_View sv_trim_left(String_View sv);
String_View sv_trim_right(String_View sv);
String_View sv_trim(String_View sv);
String_View sv_chop_by_delim(String_View *sv, char delim);
bool sv_eq(String_View a, String_View b);

typedef enum {
    ERR_OK = 0,
    ERR_STACK_OVERFLOW,
    ERR_STACK_UNDERFLOW,
    ERR_ILLEGAL_INST,
    ERR_ILLEGAL_INST_ACCESS,
    ERR_ILLEGAL_OPERAND,
    ERR_ILLEGAL_MEMORY_ACCESS,
    ERR_DIV_BY_ZERO,
} Err;

const char *err_as_cstr(Err err);

// TODO(#38): comparison instruction set is not complete
// TODO(#39): there is no operations for converting integer->float/float->interger
typedef enum {
    INST_NOP = 0,
    INST_PUSH,
    INST_DROP,
    INST_DUP,
    INST_SWAP,
    INST_PLUSI,
    INST_MINUSI,
    INST_MULTI,
    INST_DIVI,
    INST_PLUSF,
    INST_MINUSF,
    INST_MULTF,
    INST_DIVF,
    INST_JMP,
    INST_JMP_IF,
    INST_RET,
    INST_CALL,
    INST_NATIVE,
    INST_EQ,
    INST_HALT,
    INST_NOT,
    INST_GEF,
    INST_ANDB,
    INST_ORB,
    INST_XOR,
    INST_SHR,
    INST_SHL,
    INST_NOTB,
    INST_READ8,
    INST_READ16,
    INST_READ32,
    INST_READ64,
    INST_WRITE8,
    INST_WRITE16,
    INST_WRITE32,
    INST_WRITE64,
    NUMBER_OF_INSTS,
} Inst_Type;

const char *inst_name(Inst_Type type);
bool inst_has_operand(Inst_Type type);
bool inst_by_name(String_View name, Inst_Type *output);

typedef uint64_t Inst_Addr;
typedef uint64_t Memory_Addr;

typedef union {
    uint64_t as_u64;
    int64_t as_i64;
    double as_f64;
    void *as_ptr;
} Word;

Word word_u64(uint64_t u64);
Word word_i64(int64_t i64);
Word word_f64(double f64);
Word word_ptr(void *ptr);

static_assert(sizeof(Word) == 8,
              "The BM's Word is expected to be 64 bits");

typedef struct {
    Inst_Type type;
    Word operand;
} Inst;

typedef struct Bm Bm;

typedef Err (*Bm_Native)(Bm*);

struct Bm {
    Word stack[BM_STACK_CAPACITY];
    uint64_t stack_size;

    Inst program[BM_PROGRAM_CAPACITY];
    uint64_t program_size;
    Inst_Addr ip;

    Bm_Native natives[BM_NATIVES_CAPACITY];
    size_t natives_size;

    uint8_t memory[BM_MEMORY_CAPACITY];

    bool halt;
};

Err bm_execute_inst(Bm *bm);
Err bm_execute_program(Bm *bm, int limit);
void bm_push_native(Bm *bm, Bm_Native native);
void bm_dump_stack(FILE *stream, const Bm *bm);
void bm_load_program_from_file(Bm *bm, const char *file_path);

#define BM_FILE_MAGIC 0x4D42
#define BM_FILE_VERSION 1

typedef struct {
    uint16_t magic;
    uint16_t version;
    uint64_t program_size;
    uint64_t memory_size;
    uint64_t memory_capacity;
} PACKED Bm_File_Meta;

typedef struct {
    String_View name;
    Word value;
} Binding;

typedef struct {
    Inst_Addr addr;
    String_View name;
} Deferred_Operand;

typedef struct {
    Binding bindings[BASM_BINDINGS_CAPACITY];
    size_t bindings_size;

    Deferred_Operand deferred_operands[BASM_DEFERRED_OPERANDS_CAPACITY];
    size_t deferred_operands_size;

    Inst program[BM_PROGRAM_CAPACITY];
    uint64_t program_size;

    uint8_t memory[BM_MEMORY_CAPACITY];
    size_t memory_size;
    size_t memory_capacity;

    // NOTE: https://en.wikipedia.org/wiki/Region-based_memory_management
    char arena[BASM_ARENA_CAPACITY];
    size_t arena_size;
} Basm;

void *basm_alloc(Basm *basm, size_t size);
String_View basm_slurp_file(Basm *basm, String_View file_path);
bool basm_resolve_binding(const Basm *basm, String_View name, Word *output);
bool basm_bind_value(Basm *basm, String_View name, Word word);
void basm_push_deferred_operand(Basm *basm, Inst_Addr addr, String_View name);
bool basm_translate_literal(Basm *basm, String_View sv, Word *output);
void basm_save_to_file(Basm *basm, const char *output_file_path);
void basm_translate_source(Basm *basm,
                           String_View input_file_path,
                           size_t level);

#endif  // BM_H_

#ifdef BM_IMPLEMENTATION

Word word_u64(uint64_t u64)
{
    return (Word) { .as_u64 = u64 };
}

Word word_i64(int64_t i64)
{
    return (Word) { .as_i64 = i64 };
}

Word word_f64(double f64)
{
    return (Word) { .as_f64 = f64 };
}

Word word_ptr(void *ptr)
{
    return (Word) { .as_ptr = ptr };
}


bool inst_has_operand(Inst_Type type)
{
    switch (type) {
    case INST_NOP:     return false;
    case INST_PUSH:    return true;
    case INST_DROP:    return false;
    case INST_DUP:     return true;
    case INST_PLUSI:   return false;
    case INST_MINUSI:  return false;
    case INST_MULTI:   return false;
    case INST_DIVI:    return false;
    case INST_PLUSF:   return false;
    case INST_MINUSF:  return false;
    case INST_MULTF:   return false;
    case INST_DIVF:    return false;
    case INST_JMP:     return true;
    case INST_JMP_IF:  return true;
    case INST_EQ:      return false;
    case INST_HALT:    return false;
    case INST_SWAP:    return true;
    case INST_NOT:     return false;
    case INST_GEF:     return false;
    case INST_RET:     return false;
    case INST_CALL:    return true;
    case INST_NATIVE:  return true;
    case INST_ANDB:    return false;
    case INST_ORB:     return false;
    case INST_XOR:     return false;
    case INST_SHR:     return false;
    case INST_SHL:     return false;
    case INST_NOTB:    return false;
    case INST_READ8:   return false;
    case INST_READ16:  return false;
    case INST_READ32:  return false;
    case INST_READ64:  return false;
    case INST_WRITE8:  return false;
    case INST_WRITE16: return false;
    case INST_WRITE32: return false;
    case INST_WRITE64: return false;
    case NUMBER_OF_INSTS:
    default: assert(false && "inst_has_operand: unreachable");
        exit(1);
    }
}

bool inst_by_name(String_View name, Inst_Type *output)
{
    for (Inst_Type type = (Inst_Type) 0; type < NUMBER_OF_INSTS; type += 1) {
        if (sv_eq(sv_from_cstr(inst_name(type)), name)) {
            *output = type;
            return true;
        }
    }

    return false;
}

const char *inst_name(Inst_Type type)
{
    switch (type) {
    case INST_NOP:     return "nop";
    case INST_PUSH:    return "push";
    case INST_DROP:    return "drop";
    case INST_DUP:     return "dup";
    case INST_PLUSI:   return "plusi";
    case INST_MINUSI:  return "minusi";
    case INST_MULTI:   return "multi";
    case INST_DIVI:    return "divi";
    case INST_PLUSF:   return "plusf";
    case INST_MINUSF:  return "minusf";
    case INST_MULTF:   return "multf";
    case INST_DIVF:    return "divf";
    case INST_JMP:     return "jmp";
    case INST_JMP_IF:  return "jmp_if";
    case INST_EQ:      return "eq";
    case INST_HALT:    return "halt";
    case INST_SWAP:    return "swap";
    case INST_NOT:     return "not";
    case INST_GEF:     return "gef";
    case INST_RET:     return "ret";
    case INST_CALL:    return "call";
    case INST_NATIVE:  return "native";
    case INST_ANDB:    return "andb";
    case INST_ORB:     return "orb";
    case INST_XOR:     return "xor";
    case INST_SHR:     return "shr";
    case INST_SHL:     return "shl";
    case INST_NOTB:    return "notb";
    case INST_READ8:   return "read8";
    case INST_READ16:  return "read16";
    case INST_READ32:  return "read32";
    case INST_READ64:  return "read64";
    case INST_WRITE8:  return "write8";
    case INST_WRITE16: return "write16";
    case INST_WRITE32: return "write32";
    case INST_WRITE64: return "write64";
    case NUMBER_OF_INSTS:
    default: assert(false && "inst_name: unreachable");
        exit(1);
    }
}

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
    case ERR_ILLEGAL_MEMORY_ACCESS:
        return "ERR_ILLEGAL_MEMORY_ACCESS";
    default:
        assert(false && "err_as_cstr: Unreachable");
        exit(1);
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

    case INST_DROP:
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_PLUSI:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }
        bm->stack[bm->stack_size - 2].as_u64 += bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_MINUSI:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }
        bm->stack[bm->stack_size - 2].as_u64 -= bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_MULTI:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }
        bm->stack[bm->stack_size - 2].as_u64 *= bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_DIVI:
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

    case INST_PLUSF:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->stack[bm->stack_size - 2].as_f64 += bm->stack[bm->stack_size - 1].as_f64;
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_MINUSF:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->stack[bm->stack_size - 2].as_f64 -= bm->stack[bm->stack_size - 1].as_f64;
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_MULTF:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->stack[bm->stack_size - 2].as_f64 *= bm->stack[bm->stack_size - 1].as_f64;
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_DIVF:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->stack[bm->stack_size - 2].as_f64 /= bm->stack[bm->stack_size - 1].as_f64;
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_JMP:
        bm->ip = inst.operand.as_u64;
        break;

    case INST_RET:
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->ip = bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 1;
        break;

    case INST_CALL:
        if (bm->stack_size >= BM_STACK_CAPACITY) {
            return ERR_STACK_OVERFLOW;
        }

        bm->stack[bm->stack_size++].as_u64 = bm->ip + 1;
        bm->ip = inst.operand.as_u64;
        break;

    case INST_NATIVE:
        if (inst.operand.as_u64 > bm->natives_size) {
            return ERR_ILLEGAL_OPERAND;
        }
        const Err err = bm->natives[inst.operand.as_u64](bm);
        if (err != ERR_OK) {
            return err;
        }
        bm->ip += 1;
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

    // TODO(#40): Inconsistency between gef and minus* instructions operand ordering
    case INST_GEF:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->stack[bm->stack_size - 2].as_u64 = bm->stack[bm->stack_size - 1].as_f64 >= bm->stack[bm->stack_size - 2].as_f64;
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_JMP_IF:
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }

        if (bm->stack[bm->stack_size - 1].as_u64) {
            bm->ip = inst.operand.as_u64;
        } else {
            bm->ip += 1;
        }

        bm->stack_size -= 1;
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

    case INST_SWAP:
        if (inst.operand.as_u64 >= bm->stack_size) {
            return ERR_STACK_UNDERFLOW;
        }

        const uint64_t a = bm->stack_size - 1;
        const uint64_t b = bm->stack_size - 1 - inst.operand.as_u64;

        Word t = bm->stack[a];
        bm->stack[a] = bm->stack[b];
        bm->stack[b] = t;
        bm->ip += 1;
        break;

    case INST_NOT:
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->stack[bm->stack_size - 1].as_u64 = !bm->stack[bm->stack_size - 1].as_u64;
        bm->ip += 1;
        break;

    case INST_ANDB:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->stack[bm->stack_size - 2].as_u64 = bm->stack[bm->stack_size - 2].as_u64 & bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_ORB:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->stack[bm->stack_size - 2].as_u64 = bm->stack[bm->stack_size - 2].as_u64 | bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_XOR:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->stack[bm->stack_size - 2].as_u64 = bm->stack[bm->stack_size - 2].as_u64 ^ bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_SHR:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->stack[bm->stack_size - 2].as_u64 = bm->stack[bm->stack_size - 2].as_u64 >> bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_SHL:
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->stack[bm->stack_size - 2].as_u64 = bm->stack[bm->stack_size - 2].as_u64 << bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_NOTB:
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }

        bm->stack[bm->stack_size - 1].as_u64 = ~bm->stack[bm->stack_size - 1].as_u64;
        bm->ip += 1;
        break;

    case INST_READ8: {
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }
        const Memory_Addr addr = bm->stack[bm->stack_size - 1].as_u64;
        if (addr >= BM_MEMORY_CAPACITY) {
            return ERR_ILLEGAL_MEMORY_ACCESS;
        }
        bm->stack[bm->stack_size - 1].as_u64 = bm->memory[addr];
        bm->ip += 1;
    } break;

    case INST_READ16: {
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }
        const Memory_Addr addr = bm->stack[bm->stack_size - 1].as_u64;
        if (addr >= BM_MEMORY_CAPACITY - 1) {
            return ERR_ILLEGAL_MEMORY_ACCESS;
        }
        bm->stack[bm->stack_size - 1].as_u64 = *(uint16_t*)&bm->memory[addr];
        bm->ip += 1;
    } break;

    case INST_READ32: {
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }
        const Memory_Addr addr = bm->stack[bm->stack_size - 1].as_u64;
        if (addr >= BM_MEMORY_CAPACITY - 3) {
            return ERR_ILLEGAL_MEMORY_ACCESS;
        }
        bm->stack[bm->stack_size - 1].as_u64 = *(uint32_t*)&bm->memory[addr];
        bm->ip += 1;
    } break;

    case INST_READ64: {
        if (bm->stack_size < 1) {
            return ERR_STACK_UNDERFLOW;
        }
        const Memory_Addr addr = bm->stack[bm->stack_size - 1].as_u64;
        if (addr >= BM_MEMORY_CAPACITY - 7) {
            return ERR_ILLEGAL_MEMORY_ACCESS;
        }
        bm->stack[bm->stack_size - 1].as_u64 = *(uint64_t*)&bm->memory[addr];
        bm->ip += 1;
    } break;

    case INST_WRITE8: {
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }
        const Memory_Addr addr = bm->stack[bm->stack_size - 2].as_u64;
        if (addr >= BM_MEMORY_CAPACITY) {
            return ERR_ILLEGAL_MEMORY_ACCESS;
        }
        bm->memory[addr] = (uint8_t) bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 2;
        bm->ip += 1;
    } break;

    case INST_WRITE16: {
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }
        const Memory_Addr addr = bm->stack[bm->stack_size - 2].as_u64;
        if (addr >= BM_MEMORY_CAPACITY - 1) {
            return ERR_ILLEGAL_MEMORY_ACCESS;
        }
        *(uint16_t*)&bm->memory[addr] = (uint16_t) bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 2;
        bm->ip += 1;
    } break;

    case INST_WRITE32: {
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }
        const Memory_Addr addr = bm->stack[bm->stack_size - 2].as_u64;
        if (addr >= BM_MEMORY_CAPACITY - 3) {
            return ERR_ILLEGAL_MEMORY_ACCESS;
        }
        *(uint32_t*)&bm->memory[addr] = (uint32_t) bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 2;
        bm->ip += 1;
    } break;

    case INST_WRITE64: {
        if (bm->stack_size < 2) {
            return ERR_STACK_UNDERFLOW;
        }
        const Memory_Addr addr = bm->stack[bm->stack_size - 2].as_u64;
        if (addr >= BM_MEMORY_CAPACITY - 7) {
            return ERR_ILLEGAL_MEMORY_ACCESS;
        }
        *(uint64_t*)&bm->memory[addr] = bm->stack[bm->stack_size - 1].as_u64;
        bm->stack_size -= 2;
        bm->ip += 1;
    } break;

    case NUMBER_OF_INSTS:
    default:
        return ERR_ILLEGAL_INST;
    }

    return ERR_OK;
}

void bm_push_native(Bm *bm, Bm_Native native)
{
    assert(bm->natives_size < BM_NATIVES_CAPACITY);
    bm->natives[bm->natives_size++] = native;
}

void bm_dump_stack(FILE *stream, const Bm *bm)
{
    fprintf(stream, "Stack:\n");
    if (bm->stack_size > 0) {
        for (Inst_Addr i = 0; i < bm->stack_size; ++i) {
            fprintf(stream, "  u64: %" PRIu64 ", i64: %" PRId64 ", f64: %lf, ptr: %p\n",
                    bm->stack[i].as_u64,
                    bm->stack[i].as_i64,
                    bm->stack[i].as_f64,
                    bm->stack[i].as_ptr);
        }
    } else {
        fprintf(stream, "  [empty]\n");
    }
}

void bm_load_program_from_file(Bm *bm, const char *file_path)
{
    FILE *f = fopen(file_path, "rb");
    if (f == NULL) {
        fprintf(stderr, "ERROR: Could not open file `%s`: %s\n",
            file_path, strerror(errno));
        exit(1);
    }

    Bm_File_Meta meta = {0};

    size_t n = fread(&meta, sizeof(meta), 1, f);
    if (n < 1) {
        fprintf(stderr, "ERROR: Could not read meta data from file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    if (meta.magic != BM_FILE_MAGIC) {
        fprintf(stderr,
                "ERROR: %s does not appear to be a valid BM file. "
                "Unexpected magic %04X. Expected %04X.\n",
                file_path,
                meta.magic, BM_FILE_MAGIC);
        exit(1);
    }

    if (meta.version != BM_FILE_VERSION) {
        fprintf(stderr,
                "ERROR: %s: unsupported version of BM file %d. Expected version %d.\n",
                file_path,
                meta.version, BM_FILE_VERSION);
        exit(1);
    }

    if (meta.program_size > BM_PROGRAM_CAPACITY) {
        fprintf(stderr,
                "ERROR: %s: program section is too big. The file contains %" PRIu64 " program instruction. But the capacity is %"  PRIu64 "\n",
                file_path,
                meta.program_size,
                (uint64_t) BM_PROGRAM_CAPACITY);
        exit(1);
    }

    if (meta.memory_capacity > BM_MEMORY_CAPACITY) {
        fprintf(stderr,
                "ERROR: %s: memory section is too big. The file wants %" PRIu64 " bytes. But the capacity is %"  PRIu64 " bytes\n",
                file_path,
                meta.memory_capacity,
                (uint64_t) BM_MEMORY_CAPACITY);
        exit(1);
    }

    if (meta.memory_size > meta.memory_capacity) {
        fprintf(stderr,
                "ERROR: %s: memory size %"PRIu64" is greater than declared memory capacity %"PRIu64"\n",
                file_path,
                meta.memory_size,
                meta.memory_capacity);
        exit(1);
    }

    bm->program_size = fread(bm->program, sizeof(bm->program[0]), meta.program_size, f);

    if (bm->program_size != meta.program_size) {
        fprintf(stderr, "ERROR: %s: read %zd program instructions, but expected %"PRIu64"\n",
                file_path,
                bm->program_size,
                meta.program_size);
        exit(1);
    }

    n = fread(bm->memory, sizeof(bm->memory[0]), meta.memory_size, f);

    if (n != meta.memory_size) {
        fprintf(stderr, "ERROR: %s: read %zd bytes of memory section, but expected %"PRIu64" bytes.\n",
                file_path,
                n,
                meta.memory_size);
        exit(1);
    }

    fclose(f);
}

String_View sv_from_cstr(const char *cstr)
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

bool sv_eq(String_View a, String_View b)
{
    if (a.count != b.count) {
        return false;
    } else {
        return memcmp(a.data, b.data, a.count) == 0;
    }
}

void *basm_alloc(Basm *basm, size_t size)
{
    assert(basm->arena_size + size <= BASM_ARENA_CAPACITY);

    void *result = basm->arena + basm->arena_size;
    basm->arena_size += size;
    return result;
}

bool basm_resolve_binding(const Basm *basm, String_View name, Word *output)
{
    for (size_t i = 0; i < basm->bindings_size; ++i) {
        if (sv_eq(basm->bindings[i].name, name)) {
            *output = basm->bindings[i].value;
            return true;
        }
    }

    return false;
}

bool basm_bind_value(Basm *basm, String_View name, Word value)
{
    assert(basm->bindings_size < BASM_BINDINGS_CAPACITY);

    Word ignore = {0};
    if (basm_resolve_binding(basm, name, &ignore)) {
        return false;
    }

    basm->bindings[basm->bindings_size++] = (Binding) {.name = name, .value = value};
    return true;
}

void basm_push_deferred_operand(Basm *basm, Inst_Addr addr, String_View name)
{
    assert(basm->deferred_operands_size < BASM_DEFERRED_OPERANDS_CAPACITY);
    basm->deferred_operands[basm->deferred_operands_size++] =
        (Deferred_Operand) {.addr = addr, .name = name};
}

bool basm_translate_literal(Basm *basm, String_View sv, Word *output)
{
    char *cstr = basm_alloc(basm, sv.count + 1);
    memcpy(cstr, sv.data, sv.count);
    cstr[sv.count] = '\0';

    char *endptr = 0;
    Word result = {0};

    result.as_u64 = strtoull(cstr, &endptr, 10);
    if ((size_t) (endptr - cstr) != sv.count) {
        result.as_f64 = strtod(cstr, &endptr);
        if ((size_t) (endptr - cstr) != sv.count) {
            return false;
        }
    }

    *output = result;
    return true;
}

void basm_save_to_file(Basm *basm, const char *file_path)
{
    FILE *f = fopen(file_path, "wb");
    if (f == NULL) {
        fprintf(stderr, "ERROR: Could not open file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    Bm_File_Meta meta = {
        .magic = BM_FILE_MAGIC,
        .version = BM_FILE_VERSION,
        .program_size = basm->program_size,
        .memory_size = basm->memory_size,
        .memory_capacity = basm->memory_capacity,
    };

    fwrite(&meta, sizeof(meta), 1, f);
    if (ferror(f)) {
        fprintf(stderr, "ERROR: Could not write to file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    fwrite(basm->program, sizeof(basm->program[0]), basm->program_size, f);
    if (ferror(f)) {
        fprintf(stderr, "ERROR: Could not write to file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    fwrite(basm->memory, sizeof(basm->memory[0]), basm->memory_size, f);
    if (ferror(f)) {
        fprintf(stderr, "ERROR: Could not write to file `%s`: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    fclose(f);
}

void basm_translate_source(Basm *basm, String_View input_file_path, size_t level)
{
    String_View original_source = basm_slurp_file(basm, input_file_path);
    String_View source = original_source;

    int line_number = 0;

    // First pass
    while (source.count > 0) {
        String_View line = sv_trim(sv_chop_by_delim(&source, '\n'));
        line_number += 1;
        if (line.count > 0 && *line.data != BASM_COMMENT_SYMBOL) {
            String_View token = sv_trim(sv_chop_by_delim(&line, ' '));

            // Pre-processor
            if (token.count > 0 && *token.data == BASM_PP_SYMBOL) {
                token.count -= 1;
                token.data  += 1;
                if (sv_eq(token, sv_from_cstr("bind"))) {
                    line = sv_trim(line);
                    String_View name = sv_chop_by_delim(&line, ' ');
                    if (name.count > 0) {
                        line = sv_trim(line);
                        String_View value = sv_chop_by_delim(&line, ' ');
                        Word word = {0};
                        if (!basm_translate_literal(basm, value, &word)) {
                            fprintf(stderr,
                                    "%.*s:%d: ERROR: `%.*s` is not a number",
                                    SV_FORMAT(input_file_path),
                                    line_number,
                                    SV_FORMAT(value));
                            exit(1);
                        }

                        if (!basm_bind_value(basm, name, word)) {
                            // TODO(#51): label redefinition error does not tell where the first label was already defined
                            fprintf(stderr,
                                    "%.*s:%d: ERROR: name `%.*s` is already bound\n",
                                    SV_FORMAT(input_file_path),
                                    line_number,
                                    SV_FORMAT(name));
                            exit(1);
                        }
                    } else {
                        fprintf(stderr,
                                "%.*s:%d: ERROR: binding name is not provided\n",
                                SV_FORMAT(input_file_path), line_number);
                        exit(1);
                    }
                } else if (sv_eq(token, sv_from_cstr("include"))) {
                    line = sv_trim(line);

                    if (line.count > 0) {
                        if (*line.data == '"' && line.data[line.count - 1] == '"') {
                            line.data  += 1;
                            line.count -= 2;

                            if (level + 1 >= BASM_MAX_INCLUDE_LEVEL) {
                                fprintf(stderr,
                                        "%.*s:%d: ERROR: exceeded maximum include level\n",
                                        SV_FORMAT(input_file_path), line_number);
                                exit(1);
                            }

                            basm_translate_source(basm, line, level + 1);
                        } else {
                            fprintf(stderr,
                                    "%.*s:%d: ERROR: include file path has to be surrounded with quotation marks\n",
                                    SV_FORMAT(input_file_path), line_number);
                            exit(1);
                        }
                    } else {
                        fprintf(stderr,
                                "%.*s:%d: ERROR: include file path is not provided\n",
                                SV_FORMAT(input_file_path), line_number);
                        exit(1);
                    }
                } else {
                    fprintf(stderr,
                            "%.*s:%d: ERROR: unknown pre-processor directive `%.*s`\n",
                            SV_FORMAT(input_file_path),
                            line_number,
                            SV_FORMAT(token));
                    exit(1);

                }
            } else {
                // Label binding
                if (token.count > 0 && token.data[token.count - 1] == ':') {
                    String_View label = {
                        .count = token.count - 1,
                        .data = token.data
                    };

                    if (!basm_bind_value(basm, label, word_u64(basm->program_size))) {
                        fprintf(stderr,
                                "%.*s:%d: ERROR: name `%.*s` is already bound to something\n",
                                SV_FORMAT(input_file_path),
                                line_number,
                                SV_FORMAT(label));
                        exit(1);
                    }

                    token = sv_trim(sv_chop_by_delim(&line, ' '));
                }

                // Instruction
                if (token.count > 0) {
                    String_View operand = sv_trim(sv_chop_by_delim(&line, BASM_COMMENT_SYMBOL));

                    Inst_Type inst_type = INST_NOP;
                    if (inst_by_name(token, &inst_type)) {
                        assert(basm->program_size < BM_PROGRAM_CAPACITY);
                        basm->program[basm->program_size].type = inst_type;

                        if (inst_has_operand(inst_type)) {
                            if (operand.count == 0) {
                                fprintf(stderr, "%.*s:%d: ERROR: instruction `%.*s` requires an operand\n",
                                        SV_FORMAT(input_file_path),
                                        line_number,
                                        SV_FORMAT(token));
                                exit(1);
                            }

                            if (!basm_translate_literal(
                                    basm,
                                    operand,
                                    &basm->program[basm->program_size].operand)) {
                                basm_push_deferred_operand(
                                    basm, basm->program_size, operand);
                            }
                        }

                        basm->program_size += 1;
                    } else {
                        fprintf(stderr, "%.*s:%d: ERROR: unknown instruction `%.*s`\n",
                                SV_FORMAT(input_file_path),
                                line_number,
                                SV_FORMAT(token));
                        exit(1);
                    }
                }
            }
        }
    }

    // Second pass
    for (size_t i = 0; i < basm->deferred_operands_size; ++i) {
        String_View name = basm->deferred_operands[i].name;
        if (!basm_resolve_binding(
                basm,
                name,
                &basm->program[basm->deferred_operands[i].addr].operand)) {
            // TODO(#52): second pass label resolution errors don't report the location in the source code
            fprintf(stderr, "%.*s: ERROR: unknown binding `%.*s`\n",
                    SV_FORMAT(input_file_path), SV_FORMAT(name));
            exit(1);
        }
    }
}

String_View basm_slurp_file(Basm *basm, String_View file_path)
{
    char *file_path_cstr = basm_alloc(basm, file_path.count + 1);
    if (file_path_cstr == NULL) {
        fprintf(stderr,
                "ERROR: Could not allocate memory for the file path `%.*s`: %s\n",
                SV_FORMAT(file_path), strerror(errno));
        exit(1);
    }

    memcpy(file_path_cstr, file_path.data, file_path.count);
    file_path_cstr[file_path.count] = '\0';

    FILE *f = fopen(file_path_cstr, "r");
    if (f == NULL) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
                file_path_cstr, strerror(errno));
        exit(1);
    }

    if (fseek(f, 0, SEEK_END) < 0) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
                file_path_cstr, strerror(errno));
        exit(1);
    }

    long m = ftell(f);
    if (m < 0) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
                file_path_cstr, strerror(errno));
        exit(1);
    }

    char *buffer = basm_alloc(basm, (size_t) m);
    if (buffer == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for file: %s\n",
                strerror(errno));
        exit(1);
    }

    if (fseek(f, 0, SEEK_SET) < 0) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
                file_path_cstr, strerror(errno));
        exit(1);
    }

    size_t n = fread(buffer, 1, (size_t) m, f);
    if (ferror(f)) {
        fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
                file_path_cstr, strerror(errno));
        exit(1);
    }

    fclose(f);

    return (String_View) {
        .count = n,
        .data = buffer,
    };
}

#endif // BM_IMPLEMENTATION
