#ifndef BM_H_
#define BM_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>

// NOTE: Stolen from https://stackoverflow.com/a/3312896
#if defined(__GNUC__) || defined(__clang__)
#  define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#elif defined(_MSC_VER)
#  define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#else
#  error "Packed attributes for struct is not implemented for this compiler. This may result in a program working incorrectly. Feel free to fix that and submit a Pull Request to https://github.com/tsoding/bm"
#endif

#define BM_WORD_SIZE 8
#define BM_STACK_CAPACITY 1024
#define BM_PROGRAM_CAPACITY 1024
#define BM_NATIVES_CAPACITY 1024
#define BM_MEMORY_CAPACITY (640 * 1000)

#define BASM_BINDINGS_CAPACITY 1024
#define BASM_DEFERRED_OPERANDS_CAPACITY 1024
#define BASM_COMMENT_SYMBOL ';'
#define BASM_PP_SYMBOL '%'
#define BASM_MAX_INCLUDE_LEVEL 69

typedef struct {
    size_t count;
    const char *data;
} String_View;

#define SV_NULL (String_View) {0}

// printf macros for String_View
#define SV_Fmt "%.*s"
#define SV_Arg(sv) (int) sv.count, sv.data
// USAGE:
//   String_View name = ...;
//   printf("Name: "SV_Fmt"\n", SV_Arg(name));

String_View sv_from_cstr(const char *cstr);
String_View sv_trim_left(String_View sv);
String_View sv_trim_right(String_View sv);
String_View sv_trim(String_View sv);
String_View sv_chop_by_delim(String_View *sv, char delim);
String_View sv_chop_left(String_View *sv, size_t n);
String_View sv_chop_left_while(String_View *sv, bool (*predicate)(char x));
bool sv_index_of(String_View sv, char c, size_t *index);
bool sv_eq(String_View a, String_View b);
bool sv_has_prefix(String_View sv, String_View prefix);
uint64_t sv_to_u64(String_View sv);

typedef enum {
    ERR_OK = 0,
    ERR_STACK_OVERFLOW,
    ERR_STACK_UNDERFLOW,
    ERR_ILLEGAL_INST,
    ERR_ILLEGAL_INST_ACCESS,
    ERR_ILLEGAL_OPERAND,
    ERR_ILLEGAL_MEMORY_ACCESS,
    ERR_DIV_BY_ZERO,
    ERR_NULL_NATIVE
} Err;

const char *err_as_cstr(Err err);

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
    INST_MODI,
    INST_MULTU,
    INST_DIVU,
    INST_MODU,
    INST_PLUSF,
    INST_MINUSF,
    INST_MULTF,
    INST_DIVF,
    INST_JMP,
    INST_JMP_IF,
    INST_RET,
    INST_CALL,
    INST_NATIVE,
    INST_HALT,
    INST_NOT,

    INST_EQI,
    INST_GEI,
    INST_GTI,
    INST_LEI,
    INST_LTI,
    INST_NEI,

    INST_EQU,
    INST_GEU,
    INST_GTU,
    INST_LEU,
    INST_LTU,
    INST_NEU,

    INST_EQF,
    INST_GEF,
    INST_GTF,
    INST_LEF,
    INST_LTF,
    INST_NEF,

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

    INST_I2F,
    INST_U2F,
    INST_F2I,
    INST_F2U,

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

static_assert(sizeof(Word) == BM_WORD_SIZE,
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

#define BM_FILE_MAGIC 0x6D62
#define BM_FILE_VERSION 5

PACK(struct Bm_File_Meta {
    uint16_t magic;
    uint16_t version;
    uint64_t program_size;
    uint64_t entry;
    uint64_t memory_size;
    uint64_t memory_capacity;
});

typedef struct Bm_File_Meta Bm_File_Meta;

// NOTE: https://en.wikipedia.org/wiki/Region-based_memory_management
typedef struct Region Region;

struct Region {
    Region *next;
    size_t capacity;
    size_t size;
    char buffer[];
};

Region *region_create(size_t capacity);

#define ARENA_DEFAULT_CAPACITY (640 * 1000)

typedef struct {
    Region *first;
    Region *last;
} Arena;

void *arena_alloc(Arena *arena, size_t size);
void arena_clean(Arena *arena);
void arena_free(Arena *arena);
void arena_summary(Arena *arena);

int arena_slurp_file(Arena *arena, String_View file_path, String_View *content);
String_View arena_sv_concat(Arena *arena, ...);
const char *arena_cstr_concat(Arena *arena, ...);
const char *arena_sv_to_cstr(Arena *arena, String_View sv);
String_View arena_sv_dup(Arena *arena, String_View sv);

#define SV_CONCAT(arena, ...)                   \
    arena_sv_concat(arena, __VA_ARGS__, SV_NULL)

#define CSTR_CONCAT(arena, ...)                 \
    arena_cstr_concat(arena, __VA_ARGS__, NULL)

typedef struct {
    String_View file_path;
    int line_number;
} File_Location;

#define FL_Fmt SV_Fmt":%d"
#define FL_Arg(location) SV_Arg(location.file_path), location.line_number

typedef enum {
    BINDING_CONST = 0,
    BINDING_LABEL,
    BINDING_NATIVE,
} Binding_Kind;

const char *binding_kind_as_cstr(Binding_Kind kind);

typedef enum {
    TOKEN_KIND_STR,
    TOKEN_KIND_CHAR,
    TOKEN_KIND_PLUS,
    TOKEN_KIND_MINUS,
    TOKEN_KIND_NUMBER,
    TOKEN_KIND_NAME,
} Token_Kind;

const char *token_kind_name(Token_Kind kind);

typedef struct {
    Token_Kind kind;
    String_View text;
} Token;

#define TOKENS_CAPACITY 1024

typedef struct {
    Token elems[TOKENS_CAPACITY];
    size_t count;
} Tokens;

void tokens_push(Tokens *tokens, Token token);
void tokenize(String_View source, Tokens *tokens, File_Location location);

typedef struct {
    const Token *elems;
    size_t count;
} Tokens_View;

Tokens_View tokens_as_view(const Tokens *tokens);
Tokens_View tv_chop_left(Tokens_View *tv, size_t n);

typedef enum {
    EXPR_KIND_BINDING,
    EXPR_KIND_LIT_INT,
    EXPR_KIND_LIT_FLOAT,
    EXPR_KIND_LIT_CHAR,
    EXPR_KIND_LIT_STR,
    EXPR_KIND_BINARY_OP,
} Expr_Kind;

typedef struct Binary_Op Binary_Op;

typedef union {
    String_View as_binding;
    uint64_t as_lit_int;
    double as_lit_float;
    char as_lit_char;
    String_View as_lit_str;
    Binary_Op *as_binary_op;
} Expr_Value;

// TODO(#178): compile time expressions don't support math operations
typedef struct {
    Expr_Kind kind;
    Expr_Value value;
} Expr;

typedef enum {
    BINARY_OP_PLUS
} Binary_Op_Kind;

struct Binary_Op {
    Binary_Op_Kind kind;
    Expr left;
    Expr right;
};

Expr parse_sum_from_tokens(Arena *arena, Tokens_View *tokens, File_Location location);
Expr parse_primary_from_tokens(Arena *arena, Tokens_View *tokens, File_Location location);
Expr parse_expr_from_tokens(Arena *arena, Tokens_View *tokens, File_Location location);
Expr parse_expr_from_sv(Arena *arena, String_View source, File_Location location);

typedef enum {
    BINDING_UNEVALUATED = 0,
    BINDING_EVALUATING,
    BINDING_EVALUATED,
} Binding_Status;

// TODO(#177): bindings don't support expressions
typedef struct {
    Binding_Kind kind;
    String_View name;
    Word value;
    Expr expr;
    Binding_Status status;
    File_Location location;
} Binding;

typedef struct {
    Inst_Addr addr;
    Expr expr;
    File_Location location;
} Deferred_Operand;

typedef struct {
    Binding bindings[BASM_BINDINGS_CAPACITY];
    size_t bindings_size;

    Deferred_Operand deferred_operands[BASM_DEFERRED_OPERANDS_CAPACITY];
    size_t deferred_operands_size;

    Inst program[BM_PROGRAM_CAPACITY];
    uint64_t program_size;
    Inst_Addr entry;
    bool has_entry;
    File_Location entry_location;
    String_View deferred_entry_binding_name;

    uint8_t memory[BM_MEMORY_CAPACITY];
    size_t memory_size;
    size_t memory_capacity;

    Arena arena;

    size_t include_level;
    File_Location include_location;
} Basm;

Binding *basm_resolve_binding(Basm *basm, String_View name);
void basm_bind_expr(Basm *basm, String_View name, Expr expr, Binding_Kind kind, File_Location location);
void basm_bind_value(Basm *basm, String_View name, Word value, Binding_Kind kind, File_Location location);
void basm_push_deferred_operand(Basm *basm, Inst_Addr addr, Expr expr, File_Location location);
void basm_save_to_file(Basm *basm, const char *output_file_path);
Word basm_push_string_to_memory(Basm *basm, String_View sv);
void basm_translate_source(Basm *basm,
                           String_View input_file_path);
Word basm_expr_eval(Basm *basm, Expr expr, File_Location location);
Word basm_binding_eval(Basm *basm, Binding *binding, File_Location location);

void bm_load_standard_natives(Bm *bm);

Err native_alloc(Bm *bm);
Err native_free(Bm *bm);
Err native_print_f64(Bm *bm);
Err native_print_i64(Bm *bm);
Err native_print_u64(Bm *bm);
Err native_print_ptr(Bm *bm);
Err native_dump_memory(Bm *bm);
Err native_write(Bm *bm);

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
    case INST_MODI:    return false;
    case INST_MULTU:   return false;
    case INST_DIVU:    return false;
    case INST_MODU:    return false;
    case INST_PLUSF:   return false;
    case INST_MINUSF:  return false;
    case INST_MULTF:   return false;
    case INST_DIVF:    return false;
    case INST_JMP:     return true;
    case INST_JMP_IF:  return true;
    case INST_HALT:    return false;
    case INST_SWAP:    return true;
    case INST_NOT:     return false;
    case INST_EQF:     return false;
    case INST_GEF:     return false;
    case INST_GTF:     return false;
    case INST_LEF:     return false;
    case INST_LTF:     return false;
    case INST_NEF:     return false;

    case INST_EQI:     return false;
    case INST_GEI:     return false;
    case INST_GTI:     return false;
    case INST_LEI:     return false;
    case INST_LTI:     return false;
    case INST_NEI:     return false;

    case INST_EQU:     return false;
    case INST_GEU:     return false;
    case INST_GTU:     return false;
    case INST_LEU:     return false;
    case INST_LTU:     return false;
    case INST_NEU:     return false;

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
    case INST_I2F:     return false;
    case INST_U2F:     return false;
    case INST_F2I:     return false;
    case INST_F2U:     return false;
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
    case INST_MODI:    return "modi";
    case INST_MULTU:   return "multu";
    case INST_DIVU:    return "divu";
    case INST_MODU:    return "modu";
    case INST_PLUSF:   return "plusf";
    case INST_MINUSF:  return "minusf";
    case INST_MULTF:   return "multf";
    case INST_DIVF:    return "divf";
    case INST_JMP:     return "jmp";
    case INST_JMP_IF:  return "jmp_if";
    case INST_HALT:    return "halt";
    case INST_SWAP:    return "swap";
    case INST_NOT:     return "not";

    case INST_EQI:     return "eqi";
    case INST_GEI:     return "gei";
    case INST_GTI:     return "gti";
    case INST_LEI:     return "lei";
    case INST_LTI:     return "lti";
    case INST_NEI:     return "nei";

    case INST_EQU:     return "equ";
    case INST_GEU:     return "geu";
    case INST_GTU:     return "gtu";
    case INST_LEU:     return "leu";
    case INST_LTU:     return "ltu";
    case INST_NEU:     return "neu";

    case INST_EQF:     return "eqf";
    case INST_GEF:     return "gef";
    case INST_GTF:     return "gtf";
    case INST_LEF:     return "lef";
    case INST_LTF:     return "ltf";
    case INST_NEF:     return "nef";
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
    case INST_I2F:     return "i2f";
    case INST_U2F:     return "u2f";
    case INST_F2I:     return "f2i";
    case INST_F2U:     return "f2u";
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
    case ERR_NULL_NATIVE:
        return "ERR_NULL_NATIVE";
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

#define BINARY_OP(bm, in, out, op)                                      \
    do {                                                                \
        if ((bm)->stack_size < 2) {                                     \
            return ERR_STACK_UNDERFLOW;                                 \
        }                                                               \
                                                                        \
        (bm)->stack[(bm)->stack_size - 2].as_##out = (bm)->stack[(bm)->stack_size - 2].as_##in op (bm)->stack[(bm)->stack_size - 1].as_##in; \
        (bm)->stack_size -= 1;                                          \
        (bm)->ip += 1;                                                  \
    } while (false)

#define CAST_OP(bm, src, dst, cast)             \
    do {                                        \
        if ((bm)->stack_size < 1) {             \
            return ERR_STACK_UNDERFLOW;         \
        }                                                               \
                                                                        \
        (bm)->stack[(bm)->stack_size - 1].as_##dst = cast (bm)->stack[(bm)->stack_size - 1].as_##src; \
                                                                        \
        (bm)->ip += 1;                                                  \
    } while (false)


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
        BINARY_OP(bm, u64, u64, +);
        break;

    case INST_MINUSI:
        BINARY_OP(bm, u64, u64, -);
        break;

    case INST_MULTI:
        BINARY_OP(bm, i64, i64, *);
        break;

    case INST_MULTU:
        BINARY_OP(bm, u64, u64, *);
        break;

    case INST_DIVI: {
        if (bm->stack[bm->stack_size - 1].as_i64 == 0) {
            return ERR_DIV_BY_ZERO;
        }
        BINARY_OP(bm, i64, i64, /);
    } break;

    case INST_DIVU: {
        if (bm->stack[bm->stack_size - 1].as_u64 == 0) {
            return ERR_DIV_BY_ZERO;
        }
        BINARY_OP(bm, u64, u64, /);
    } break;

    case INST_MODI: {
        if (bm->stack[bm->stack_size - 1].as_i64 == 0) {
            return ERR_DIV_BY_ZERO;
        }
        BINARY_OP(bm, i64, i64, %);
    } break;

    case INST_MODU: {
        if (bm->stack[bm->stack_size - 1].as_u64 == 0) {
            return ERR_DIV_BY_ZERO;
        }
        BINARY_OP(bm, u64, u64, %);
    } break;

    case INST_PLUSF:
        BINARY_OP(bm, f64, f64, +);
        break;

    case INST_MINUSF:
        BINARY_OP(bm, f64, f64, -);
        break;

    case INST_MULTF:
        BINARY_OP(bm, f64, f64, *);
        break;

    case INST_DIVF:
        BINARY_OP(bm, f64, f64, /);
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

        if (!bm->natives[inst.operand.as_u64]) {
            return ERR_NULL_NATIVE;
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

    case INST_EQF:
        BINARY_OP(bm, f64, u64, ==);
        break;

    case INST_GEF:
        BINARY_OP(bm, f64, u64, >=);
        break;

    case INST_GTF:
        BINARY_OP(bm, f64, u64, >);
        break;

    case INST_LEF:
        BINARY_OP(bm, f64, u64, <=);
        break;

    case INST_LTF:
        BINARY_OP(bm, f64, u64, <);
        break;

    case INST_NEF:
        BINARY_OP(bm, f64, u64, !=);
        break;

    case INST_EQI:
        BINARY_OP(bm, i64, u64, ==);
        break;

    case INST_GEI:
        BINARY_OP(bm, i64, u64, >=);
        break;

    case INST_GTI:
        BINARY_OP(bm, i64, u64, >);
        break;

    case INST_LEI:
        BINARY_OP(bm, i64, u64, <=);
        break;

    case INST_LTI:
        BINARY_OP(bm, i64, u64, <);
        break;

    case INST_NEI:
        BINARY_OP(bm, i64, u64, !=);
        break;

    case INST_EQU:
        BINARY_OP(bm, u64, u64, ==);
        break;

    case INST_GEU:
        BINARY_OP(bm, u64, u64, >=);
        break;

    case INST_GTU:
        BINARY_OP(bm, u64, u64, >);
        break;

    case INST_LEU:
        BINARY_OP(bm, u64, u64, <=);
        break;

    case INST_LTU:
        BINARY_OP(bm, u64, u64, <);
        break;

    case INST_NEU:
        BINARY_OP(bm, u64, u64, !=);
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
        BINARY_OP(bm, u64, u64, &);
        break;

    case INST_ORB:
        BINARY_OP(bm, u64, u64, |);
        break;

    case INST_XOR:
        BINARY_OP(bm, u64, u64, ^);
        break;

    case INST_SHR:
        BINARY_OP(bm, u64, u64, >>);
        break;

    case INST_SHL:
        BINARY_OP(bm, u64, u64, <<);
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

    case INST_I2F:
        CAST_OP(bm, i64, f64, (double));
        break;

    case INST_U2F:
        CAST_OP(bm, u64, f64, (double));
        break;

    case INST_F2I:
        CAST_OP(bm, f64, i64, (int64_t));
        break;

    case INST_F2U:
        CAST_OP(bm, f64, u64, (uint64_t) (int64_t));
        break;

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
    memset(bm, 0, sizeof(*bm));

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

    bm->ip = meta.entry;

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
        fprintf(stderr, "ERROR: %s: read %"PRIu64" program instructions, but expected %"PRIu64"\n",
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

String_View sv_chop_left(String_View *sv, size_t n)
{
    if (n > sv->count) {
        n = sv->count;
    }

    String_View result = {
        .data = sv->data,
        .count = n,
    };

    sv->data  += n;
    sv->count -= n;

    return result;
}

bool sv_index_of(String_View sv, char c, size_t *index)
{
    size_t i = 0;
    while (i < sv.count && sv.data[i] != c) {
        i += 1;
    }

    if (i < sv.count) {
        *index = i;
        return true;
    } else {
        return false;
    }
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

bool sv_has_prefix(String_View sv, String_View prefix)
{
    if (prefix.count <= sv.count) {
        for (size_t i = 0; i < prefix.count; ++i) {
            if (prefix.data[i] != sv.data[i]) {
                return false;
            }
        }

        return true;
    } else {
        return false;
    }
}

bool sv_eq(String_View a, String_View b)
{
    if (a.count != b.count) {
        return false;
    } else {
        return memcmp(a.data, b.data, a.count) == 0;
    }
}

uint64_t sv_to_u64(String_View sv)
{
    uint64_t result = 0;

    for (size_t i = 0; i < sv.count && isdigit(sv.data[i]); ++i) {
        result = result * 10 + (uint64_t) sv.data[i] - '0';
    }

    return result;
}

const char *arena_sv_to_cstr(Arena *arena, String_View sv)
{
    char *cstr = arena_alloc(arena, sv.count + 1);
    memcpy(cstr, sv.data, sv.count);
    cstr[sv.count] = '\0';
    return cstr;
}

String_View arena_sv_dup(Arena *arena, String_View sv)
{
    char *buffer = arena_alloc(arena, sv.count);
    memcpy(buffer, sv.data, sv.count);
    return (String_View) {
        .count = sv.count,
        .data = buffer,
    };
}

String_View arena_sv_concat(Arena *arena, ...)
{
    size_t len = 0;

    va_list args;
    va_start(args, arena);
    String_View sv = va_arg(args, String_View);
    while (sv.data != NULL) {
        len += sv.count;
        sv = va_arg(args, String_View);
    }
    va_end(args);

    char *buffer = arena_alloc(arena, len);
    len = 0;

    va_start(args, arena);
    sv = va_arg(args, String_View);
    while (sv.data != NULL) {
        memcpy(buffer + len, sv.data, sv.count);
        len += sv.count;
        sv = va_arg(args, String_View);
    }
    va_end(args);

    return (String_View) {
        .count = len,
        .data = buffer
    };
}

const char *arena_cstr_concat(Arena *arena, ...)
{
    size_t len = 0;

    va_list args;
    va_start(args, arena);
    const char *cstr = va_arg(args, const char*);
    while (cstr != NULL) {
        len += strlen(cstr);
        cstr = va_arg(args, const char*);
    }
    va_end(args);

    char *buffer = arena_alloc(arena, len + 1);
    len = 0;

    va_start(args, arena);
    cstr = va_arg(args, const char*);
    while (cstr != NULL) {
        size_t n = strlen(cstr);
        memcpy(buffer + len, cstr, n);
        len += n;
        cstr = va_arg(args, const char*);
    }
    va_end(args);

    buffer[len] = '\0';

    return buffer;
}

Region *region_create(size_t capacity)
{
    const size_t region_size = sizeof(Region) + capacity;
    Region *region = malloc(region_size);
    memset(region, 0, region_size);
    region->capacity = capacity;
    return region;
}

void *arena_alloc(Arena *arena, size_t size)
{
    if (arena->last == NULL) {
        assert(arena->first == NULL);

        Region *region = region_create(
            size > ARENA_DEFAULT_CAPACITY ? size : ARENA_DEFAULT_CAPACITY);

        arena->last = region;
        arena->first = region;
    }

    while (arena->last->size + size > arena->last->capacity &&
           arena->last->next) {
        arena->last = arena->last->next;
    }

    if (arena->last->size + size > arena->last->capacity) {
        Region *region = region_create(
            size > ARENA_DEFAULT_CAPACITY ? size : ARENA_DEFAULT_CAPACITY);

        arena->last->next = region;
        arena->last = region;
    }

    void *result = arena->last->buffer + arena->last->size;
    arena->last->size += size;
    return result;
}

void arena_clean(Arena *arena)
{
    for (Region *iter = arena->first;
         iter != NULL;
         iter = iter->next)
    {
        iter->size = 0;
    }

    arena->last = arena->first;
}

void arena_free(Arena *arena)
{
    Region *iter = arena->first;
    while (iter != NULL) {
        Region *next = iter->next;
        free(iter);
        iter = next;
    }
    arena->first = NULL;
    arena->last = NULL;
}

void arena_summary(Arena *arena)
{
    if (arena->first == NULL) {
        printf("[empty]");
    }

    for (Region *iter = arena->first;
         iter != NULL;
         iter = iter->next)
    {
        printf("[%zu/%zu] -> ", iter->size, iter->capacity);
    }
    printf("\n");
}

Binding *basm_resolve_binding(Basm *basm, String_View name)
{
    for (size_t i = 0; i < basm->bindings_size; ++i) {
        if (sv_eq(basm->bindings[i].name, name)) {
            return &basm->bindings[i];
        }
    }

    return NULL;
}

void basm_bind_value(Basm *basm, String_View name, Word value, Binding_Kind kind, File_Location location)
{
    assert(basm->bindings_size < BASM_BINDINGS_CAPACITY);

    Binding *existing = basm_resolve_binding(basm, name);
    if (existing) {
        fprintf(stderr,
                FL_Fmt": ERROR: name `"SV_Fmt"` is already bound\n",
                FL_Arg(location),
                SV_Arg(name));
        fprintf(stderr,
                FL_Fmt": NOTE: first binding is located here\n",
                FL_Arg(existing->location));
        exit(1);
    }

    basm->bindings[basm->bindings_size++] = (Binding) {
        .name = name,
        .value = value,
        .status = BINDING_EVALUATED,
        .kind = kind,
        .location = location,
    };
}

void basm_bind_expr(Basm *basm, String_View name, Expr expr, Binding_Kind kind, File_Location location)
{
    assert(basm->bindings_size < BASM_BINDINGS_CAPACITY);

    Binding *existing = basm_resolve_binding(basm, name);
    if (existing) {
        fprintf(stderr,
                FL_Fmt": ERROR: name `"SV_Fmt"` is already bound\n",
                FL_Arg(location),
                SV_Arg(name));
        fprintf(stderr,
                FL_Fmt": NOTE: first binding is located here\n",
                FL_Arg(existing->location));
        exit(1);
    }

    basm->bindings[basm->bindings_size++] = (Binding) {
        .name = name,
        .expr = expr,
        .kind = kind,
        .location = location,
    };
}

void basm_push_deferred_operand(Basm *basm, Inst_Addr addr, Expr expr, File_Location location)
{
    assert(basm->deferred_operands_size < BASM_DEFERRED_OPERANDS_CAPACITY);
    basm->deferred_operands[basm->deferred_operands_size++] =
        (Deferred_Operand) {.addr = addr, .expr = expr, .location = location};
}

Word basm_push_string_to_memory(Basm *basm, String_View sv)
{
    assert(basm->memory_size + sv.count <= BM_MEMORY_CAPACITY);

    Word result = word_u64(basm->memory_size);
    memcpy(basm->memory + basm->memory_size, sv.data, sv.count);
    basm->memory_size += sv.count;

    if (basm->memory_size > basm->memory_capacity) {
        basm->memory_capacity = basm->memory_size;
    }

    return result;
}

const char *binding_kind_as_cstr(Binding_Kind kind)
{
    switch (kind) {
        case BINDING_CONST: return "const";
        case BINDING_LABEL: return "label";
        case BINDING_NATIVE: return "native";
        default:
            assert(false && "binding_kind_as_cstr: unreachable");
            exit(0);
    }
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
        .entry = basm->entry,
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

static void basm_translate_bind_directive(Basm *basm, String_View *line, File_Location location, Binding_Kind binding_kind)
{
    *line = sv_trim(*line);
    String_View name = sv_chop_by_delim(line, ' ');
    if (name.count > 0) {
        *line = sv_trim(*line);
        Expr expr = parse_expr_from_sv(&basm->arena, *line, location);

        basm_bind_expr(basm, name, expr, binding_kind, location);

        // TODO(#182): there is not way to get the length of the string again
        // I removed it. Oops.
    } else {
        fprintf(stderr,
                FL_Fmt": ERROR: binding name is not provided\n",
                FL_Arg(location));
        exit(1);
    }
}

void basm_translate_source(Basm *basm, String_View input_file_path)
{
    String_View original_source = {0};
    if (arena_slurp_file(&basm->arena, input_file_path, &original_source) < 0) {
        if (basm->include_level > 0) {
            fprintf(stderr, FL_Fmt": ERROR: could not read file `"SV_Fmt"`: %s\n",
                    FL_Arg(basm->include_location),
                    SV_Arg(input_file_path), strerror(errno));
        } else {
            fprintf(stderr, "ERROR: could not read file `"SV_Fmt"`: %s\n",
                    SV_Arg(input_file_path), strerror(errno));
        }
        exit(1);
    }

    String_View source = original_source;

    File_Location location = {
        .file_path = input_file_path,
    };

    // First pass
    while (source.count > 0) {
        String_View line = sv_trim(sv_chop_by_delim(&source, '\n'));
        line = sv_trim(sv_chop_by_delim(&line, BASM_COMMENT_SYMBOL));
        location.line_number += 1;
        if (line.count > 0) {
            String_View token = sv_trim(sv_chop_by_delim(&line, ' '));

            // Pre-processor
            if (token.count > 0 && *token.data == BASM_PP_SYMBOL) {
                token.count -= 1;
                token.data  += 1;
                if (sv_eq(token, sv_from_cstr("bind"))) {
                    fprintf(stderr, FL_Fmt": ERROR: %%bind directive has been removed! Use %%const directive to define consts. Use %%native directive to define native functions.\n", FL_Arg(location));
                    exit(1);
                } else if (sv_eq(token, sv_from_cstr("const"))) {
                    basm_translate_bind_directive(basm, &line, location, BINDING_CONST);
                } else if (sv_eq(token, sv_from_cstr("native"))) {
                    basm_translate_bind_directive(basm, &line, location, BINDING_NATIVE);
                } else if (sv_eq(token, sv_from_cstr("include"))) {
                    line = sv_trim(line);


                    if (line.count > 0) {
                        if (*line.data == '"' && line.data[line.count - 1] == '"') {
                            line.data  += 1;
                            line.count -= 2;

                            if (basm->include_level + 1 >= BASM_MAX_INCLUDE_LEVEL) {
                                fprintf(stderr,
                                        FL_Fmt": ERROR: exceeded maximum include level\n",
                                        FL_Arg(location));
                                exit(1);
                            }

                            {
                                File_Location prev_include_location = basm->include_location;
                                basm->include_level += 1;
                                basm->include_location = location;
                                basm_translate_source(basm, line);
                                basm->include_location = prev_include_location;
                                basm->include_level -= 1;
                            }
                        } else {
                            fprintf(stderr,
                                    FL_Fmt": ERROR: include file path has to be surrounded with quotation marks\n",
                                    FL_Arg(location));
                            exit(1);
                        }
                    } else {
                        fprintf(stderr,
                                FL_Fmt": ERROR: include file path is not provided\n",
                                FL_Arg(location));
                        exit(1);
                    }
                } else if (sv_eq(token, sv_from_cstr("entry"))) {
                    if (basm->has_entry) {
                        fprintf(stderr,
                                FL_Fmt": ERROR: entry point has been already set!\n",
                                FL_Arg(location));
                        fprintf(stderr, FL_Fmt": NOTE: the first entry point\n", FL_Arg(basm->entry_location));
                        exit(1);
                    }

                    line = sv_trim(line);

                    Expr expr = parse_expr_from_sv(&basm->arena, line, location);

                    if (expr.kind != EXPR_KIND_BINDING) {
                        fprintf(stderr, FL_Fmt": ERROR: only bindings are allowed to be set as entry points for now.\n",
                                FL_Arg(location));
                        exit(1);
                    }

                    basm->deferred_entry_binding_name = expr.value.as_binding;
                    basm->has_entry = true;
                    basm->entry_location = location;
                } else {
                    fprintf(stderr,
                            FL_Fmt": ERROR: unknown pre-processor directive `"SV_Fmt"`\n",
                            FL_Arg(location),
                            SV_Arg(token));
                    exit(1);

                }
            } else {
                // Label binding
                if (token.count > 0 && token.data[token.count - 1] == ':') {
                    String_View label = {
                        .count = token.count - 1,
                        .data = token.data
                    };

                    basm_bind_value(basm, label, word_u64(basm->program_size), BINDING_LABEL, location);
                    token = sv_trim(sv_chop_by_delim(&line, ' '));
                }

                // Instruction
                if (token.count > 0) {
                    String_View operand = line;
                    Inst_Type inst_type = INST_NOP;
                    if (inst_by_name(token, &inst_type)) {
                        assert(basm->program_size < BM_PROGRAM_CAPACITY);
                        basm->program[basm->program_size].type = inst_type;

                        if (inst_has_operand(inst_type)) {
                            if (operand.count == 0) {
                                fprintf(stderr, FL_Fmt": ERROR: instruction `"SV_Fmt"` requires an operand\n",
                                        FL_Arg(location),
                                        SV_Arg(token));
                                exit(1);
                            }

                            Expr expr = parse_expr_from_sv(&basm->arena, operand, location);

                            if (expr.kind == EXPR_KIND_BINDING) {
                                basm_push_deferred_operand(basm, basm->program_size, expr, location);
                            } else {
                                assert(expr.kind != EXPR_KIND_BINDING);
                                basm->program[basm->program_size].operand =
                                    basm_expr_eval(basm, expr, location);
                            }
                        }

                        basm->program_size += 1;
                    } else {
                        fprintf(stderr, FL_Fmt": ERROR: unknown instruction `"SV_Fmt"`\n",
                                FL_Arg(location),
                                SV_Arg(token));
                        exit(1);
                    }
                }
            }
        }
    }

    // Second pass
    for (size_t i = 0; i < basm->deferred_operands_size; ++i) {
        Expr expr = basm->deferred_operands[i].expr;
        assert(expr.kind == EXPR_KIND_BINDING);
        String_View name = expr.value.as_binding;

        Inst_Addr addr = basm->deferred_operands[i].addr;
        Binding *binding = basm_resolve_binding(basm, name);
        if (binding == NULL) {
            fprintf(stderr, FL_Fmt": ERROR: unknown binding `"SV_Fmt"`\n",
                    FL_Arg(basm->deferred_operands[i].location),
                    SV_Arg(name));
            exit(1);
        }

        if (basm->program[addr].type == INST_CALL && binding->kind != BINDING_LABEL) {
            fprintf(stderr, FL_Fmt": ERROR: trying to call not a label. `"SV_Fmt"` is %s, but the call instructions accepts only literals or labels.\n", FL_Arg(basm->deferred_operands[i].location), SV_Arg(name), binding_kind_as_cstr(binding->kind));
            exit(1);
        }

        if (basm->program[addr].type == INST_NATIVE && binding->kind != BINDING_NATIVE) {
            fprintf(stderr, FL_Fmt": ERROR: trying to invoke native function from a binding that is %s. Bindings for native functions have to be defined via `%%native` basm directive.\n", FL_Arg(basm->deferred_operands[i].location), binding_kind_as_cstr(binding->kind));
            exit(1);
        }

        basm->program[addr].operand = basm_binding_eval(basm, binding, basm->deferred_operands[i].location);
    }

    // Resolving deferred entry point
    if (basm->has_entry && basm->deferred_entry_binding_name.count > 0) {
        Binding *binding = basm_resolve_binding(
            basm,
            basm->deferred_entry_binding_name);
        if (binding == NULL) {
            fprintf(stderr, FL_Fmt": ERROR: unknown binding `"SV_Fmt"`\n",
                    FL_Arg(basm->entry_location),
                    SV_Arg(basm->deferred_entry_binding_name));
            exit(1);
        }

        if (binding->kind != BINDING_LABEL) {
            fprintf(stderr, FL_Fmt": ERROR: trying to set a %s as an entry point. Entry point has to be a label.\n", FL_Arg(basm->entry_location), binding_kind_as_cstr(binding->kind));
            exit(1);
        }

        basm->entry = basm_binding_eval(basm, binding, basm->entry_location).as_u64;
    }
}

Word basm_binding_eval(Basm *basm, Binding *binding, File_Location location)
{
    if (binding->status == BINDING_EVALUATING) {
        fprintf(stderr, FL_Fmt": ERROR: cycling binding definition.\n",
        FL_Arg(binding->location));
        exit(1);
    }

    if (binding->status == BINDING_UNEVALUATED) {
        binding->status = BINDING_EVALUATING;
        Word value = basm_expr_eval(basm, binding->expr, location);
        binding->status = BINDING_EVALUATED;
        binding->value = value;
    }

    return binding->value;
}

static Word basm_binary_op_eval(Basm *basm, Binary_Op *binary_op, File_Location location)
{
    switch (binary_op->kind) {
    case BINARY_OP_PLUS: {
        Word left = basm_expr_eval(basm, binary_op->left, location);
        Word right = basm_expr_eval(basm, binary_op->right, location);
        // TODO(#183): compile-time sum can only work with integers
        return word_u64(left.as_u64 + right.as_u64);
    } break;

    default: {
        assert(false && "basm_binary_op_eval: unreachable");
        exit(1);
    }
    }
}

Word basm_expr_eval(Basm *basm, Expr expr, File_Location location)
{
    switch (expr.kind) {
    case EXPR_KIND_LIT_INT:
        return word_u64(expr.value.as_lit_int);

    case EXPR_KIND_LIT_FLOAT:
        return word_f64(expr.value.as_lit_float);

    case EXPR_KIND_LIT_CHAR:
        return word_u64((uint64_t) expr.value.as_lit_char);

    case EXPR_KIND_LIT_STR:
        return basm_push_string_to_memory(basm, expr.value.as_lit_str);

    case EXPR_KIND_BINDING: {
        String_View name = expr.value.as_binding;
        Binding *binding = basm_resolve_binding(basm, name);
        if (binding == NULL) {
            fprintf(stderr, FL_Fmt": ERROR: could find binding `"SV_Fmt"`.\n",
                    FL_Arg(location), SV_Arg(name));
            exit(1);
        }

        return basm_binding_eval(basm, binding, location);
    } break;

    case EXPR_KIND_BINARY_OP: {
        return basm_binary_op_eval(basm, expr.value.as_binary_op, location);
    } break;

    default: {
        assert(false && "basm_expr_eval: unreachable");
        exit(1);
    } break;
    }
}

int arena_slurp_file(Arena *arena, String_View file_path, String_View *content)
{
    const char *file_path_cstr = arena_sv_to_cstr(arena, file_path);

    FILE *f = fopen(file_path_cstr, "r");
    if (f == NULL) {
        return -1;
    }

    if (fseek(f, 0, SEEK_END) < 0) {
        return -1;
    }

    long m = ftell(f);
    if (m < 0) {
        return -1;
    }

    char *buffer = arena_alloc(arena, (size_t) m);
    if (buffer == NULL) {
        return -1;
    }

    if (fseek(f, 0, SEEK_SET) < 0) {
        return -1;
    }

    size_t n = fread(buffer, 1, (size_t) m, f);
    if (ferror(f)) {
        return -1;
    }

    fclose(f);

    if (content) {
        content->count = n;
        content->data = buffer;
    }

    return 0;
}

void bm_load_standard_natives(Bm *bm)
{
    // TODO(#35): some sort of mechanism to load native functions from DLLs
    bm_push_native(bm, native_write); // 0
}

Err native_alloc(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    bm->stack[bm->stack_size - 1].as_ptr = malloc(bm->stack[bm->stack_size - 1].as_u64);

    return ERR_OK;
}

Err native_free(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    free(bm->stack[bm->stack_size - 1].as_ptr);
    bm->stack_size -= 1;

    return ERR_OK;
}

Err native_print_f64(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    printf("%lf\n", bm->stack[bm->stack_size - 1].as_f64);
    bm->stack_size -= 1;
    return ERR_OK;
}

Err native_print_i64(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    printf("%" PRId64 "\n", bm->stack[bm->stack_size - 1].as_i64);
    bm->stack_size -= 1;
    return ERR_OK;
}

Err native_print_u64(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    printf("%" PRIu64 "\n", bm->stack[bm->stack_size - 1].as_u64);
    bm->stack_size -= 1;
    return ERR_OK;
}

Err native_print_ptr(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    printf("%p\n", bm->stack[bm->stack_size - 1].as_ptr);
    bm->stack_size -= 1;
    return ERR_OK;
}

Err native_dump_memory(Bm *bm)
{
    if (bm->stack_size < 2) {
        return ERR_STACK_UNDERFLOW;
    }

    Memory_Addr addr = bm->stack[bm->stack_size - 2].as_u64;
    uint64_t count = bm->stack[bm->stack_size - 1].as_u64;

    if (addr >= BM_MEMORY_CAPACITY) {
        return ERR_ILLEGAL_MEMORY_ACCESS;
    }

    if (addr + count < addr || addr + count >= BM_MEMORY_CAPACITY) {
        return ERR_ILLEGAL_MEMORY_ACCESS;
    }

    for (uint64_t i = 0; i < count; ++i) {
        printf("%02X ", bm->memory[addr + i]);
    }
    printf("\n");

    bm->stack_size -= 2;

    return ERR_OK;
}

Err native_write(Bm *bm)
{
    if (bm->stack_size < 2) {
        return ERR_STACK_UNDERFLOW;
    }

    Memory_Addr addr = bm->stack[bm->stack_size - 2].as_u64;
    uint64_t count = bm->stack[bm->stack_size - 1].as_u64;

    if (addr >= BM_MEMORY_CAPACITY) {
        return ERR_ILLEGAL_MEMORY_ACCESS;
    }

    if (addr + count < addr || addr + count >= BM_MEMORY_CAPACITY) {
        return ERR_ILLEGAL_MEMORY_ACCESS;
    }

    fwrite(&bm->memory[addr], sizeof(bm->memory[0]), count, stdout);

    bm->stack_size -= 2;

    return ERR_OK;
}

const char *token_kind_name(Token_Kind kind)
{
    switch (kind) {
    case TOKEN_KIND_STR: return "string";
    case TOKEN_KIND_CHAR: return "character";
    case TOKEN_KIND_PLUS: return "plus";
    case TOKEN_KIND_MINUS: return "minus";
    case TOKEN_KIND_NUMBER: return "number";
    case TOKEN_KIND_NAME: return "name";
    default: {
        assert(false && "token_kind_name: unreachable");
        exit(1);
    }
    }
}

void tokens_push(Tokens *tokens, Token token)
{
    assert(tokens->count < TOKENS_CAPACITY);
    tokens->elems[tokens->count++] = token;
}

static bool is_name(char x)
{
    return isalnum(x) || x == '_';
}

static bool is_number(char x)
{
    return isalnum(x) || x == '.';
}

String_View sv_chop_left_while(String_View *sv, bool (*predicate)(char x))
{
    size_t i = 0;
    while (i < sv->count && predicate(sv->data[i])) {
        i += 1;
    }
    return sv_chop_left(sv, i);
}

void tokenize(String_View source, Tokens *tokens, File_Location location)
{
    source = sv_trim_left(source);
    while (source.count > 0) {
        switch (*source.data) {
        case '+': {
            tokens_push(tokens, (Token) {
               .kind = TOKEN_KIND_PLUS,
               .text = sv_chop_left(&source, 1)
            });
        } break;

        case '-': {
            tokens_push(tokens, (Token) {
                .kind = TOKEN_KIND_MINUS,
                .text = sv_chop_left(&source, 1)
            });
        } break;

        case '"': {
            sv_chop_left(&source, 1);

            size_t index = 0;

            if (sv_index_of(source, '"', &index)) {
                String_View text = sv_chop_left(&source, index);
                sv_chop_left(&source, 1);
                tokens_push(tokens, (Token) {.kind = TOKEN_KIND_STR, .text = text});
            } else {
                fprintf(stderr, FL_Fmt": ERROR: Could not find closing \"\n",
                        FL_Arg(location));
                exit(1);
            }
        } break;

        case '\'': {
            sv_chop_left(&source, 1);

            size_t index = 0;

            if (sv_index_of(source, '\'', &index)) {
                String_View text = sv_chop_left(&source, index);
                sv_chop_left(&source, 1);
                tokens_push(tokens, (Token) {.kind = TOKEN_KIND_CHAR, .text = text});
            } else {
                fprintf(stderr, FL_Fmt": ERROR: Could not find closing \'\n",
                        FL_Arg(location));
                exit(1);
            }
        } break;

        default: {
            if (isalpha(*source.data)) {
                tokens_push(tokens, (Token) {
                    .kind = TOKEN_KIND_NAME,
                    .text = sv_chop_left_while(&source, is_name)
                });
            } else if (isdigit(*source.data)) {
                tokens_push(tokens, (Token) {
                    .kind = TOKEN_KIND_NUMBER,
                    .text = sv_chop_left_while(&source, is_number)
                });
            } else {
                fprintf(stderr, FL_Fmt": ERROR: Unknown token starts with %c\n",
                        FL_Arg(location), *source.data);
                exit(1);
            }
        }
        }

        source = sv_trim_left(source);
    }
}

Tokens_View tokens_as_view(const Tokens *tokens)
{
    return (Tokens_View) {
        .elems = tokens->elems,
        .count = tokens->count,
    };
}

Tokens_View tv_chop_left(Tokens_View *tv, size_t n)
{
    if (n > tv->count) {
        n = tv->count;
    }

    Tokens_View result = {
        .elems = tv->elems,
        .count = n,
    };

    tv->elems += n;
    tv->count -= n;

    return result;
}

static Expr parse_number_from_tokens(Arena *arena, Tokens_View *tokens, File_Location location)
{
    if (tokens->count == 0) {
        fprintf(stderr, FL_Fmt": ERROR: Cannot parse empty expression\n",
                FL_Arg(location));
        exit(1);
    }

    Expr result = {0};

    if (tokens->elems->kind == TOKEN_KIND_NUMBER) {
        String_View text = tokens->elems->text;

        const char *cstr = arena_sv_to_cstr(arena, text);
        char *endptr = 0;

        if (sv_has_prefix(text, sv_from_cstr("0x"))) {
            result.value.as_lit_int = strtoull(cstr, &endptr, 16);
            if ((size_t) (endptr - cstr) != text.count) {
                fprintf(stderr, FL_Fmt": ERROR: `"SV_Fmt"` is not a hex literal\n",
                        FL_Arg(location), SV_Arg(text));
                exit(1);
            }

            result.kind = EXPR_KIND_LIT_INT;
            tv_chop_left(tokens, 1);
        } else {
            result.value.as_lit_int = strtoull(cstr, &endptr, 10);
            if ((size_t) (endptr - cstr) != text.count) {
                result.value.as_lit_float = strtod(cstr, &endptr);
                if ((size_t) (endptr - cstr) != text.count) {
                    fprintf(stderr, FL_Fmt": ERROR: `"SV_Fmt"` is not a number literal\n",
                            FL_Arg(location), SV_Arg(text));
                } else {
                    result.kind = EXPR_KIND_LIT_FLOAT;
                }
            } else {
                result.kind = EXPR_KIND_LIT_INT;
            }

            tv_chop_left(tokens, 1);
        }
    } else {
        fprintf(stderr, FL_Fmt": ERROR: expected %s but got %s",
                FL_Arg(location),
                token_kind_name(TOKEN_KIND_NUMBER),
                token_kind_name(tokens->elems->kind));
        exit(1);
    }

    return result;
}

Expr parse_primary_from_tokens(Arena *arena, Tokens_View *tokens, File_Location location)
{
    if (tokens->count == 0) {
        fprintf(stderr, FL_Fmt": ERROR: Cannot parse empty expression\n",
                FL_Arg(location));
        exit(1);
    }

    Expr result = {0};

    switch (tokens->elems->kind) {
    case TOKEN_KIND_STR: {
        // TODO(#66): string literals don't support escaped characters
        result.kind = EXPR_KIND_LIT_STR;
        result.value.as_lit_str = tokens->elems->text;
        tv_chop_left(tokens, 1);
    } break;

    case TOKEN_KIND_CHAR: {
        if (tokens->elems->text.count != 1) {
            // TODO(#179): char literals don't support escaped characters
            fprintf(stderr, FL_Fmt": ERROR: the length of char literal has to be exactly one\n",
                    FL_Arg(location));
            exit(1);
        }

        result.kind = EXPR_KIND_LIT_CHAR;
        result.value.as_lit_char = tokens->elems->text.data[0];
        tv_chop_left(tokens, 1);
    } break;

    case TOKEN_KIND_NAME: {
        result.value.as_binding = tokens->elems->text;
        result.kind = EXPR_KIND_BINDING;
        tv_chop_left(tokens, 1);
    } break;

    case TOKEN_KIND_NUMBER: {
        return parse_number_from_tokens(arena, tokens, location);
    } break;

    case TOKEN_KIND_MINUS: {
        tv_chop_left(tokens, 1);
        Expr expr = parse_number_from_tokens(arena, tokens, location);

        if (expr.kind == EXPR_KIND_LIT_INT) {
            // TODO(#184): more cross-platform way to negate integer literals
            // what if somewhere the numbers are not two's complement
            expr.value.as_lit_int = (~expr.value.as_lit_int + 1);
        } else if (expr.kind == EXPR_KIND_LIT_FLOAT) {
            expr.value.as_lit_float = -expr.value.as_lit_float;
        } else {
            assert(false && "unreachable");
        }

        return expr;
    } break;

    case TOKEN_KIND_PLUS: {
        fprintf(stderr, FL_Fmt": ERROR: expected primary expression but found %s\n",
                FL_Arg(location), token_kind_name(tokens->elems->kind));
        exit(1);
    } break;

    default: {
        assert(false && "parse_primary_from_tokens: unreachable");
        exit(1);
    }
    }

    return result;
}

Expr parse_sum_from_tokens(Arena *arena, Tokens_View *tokens, File_Location location)
{
    Expr left = parse_primary_from_tokens(arena, tokens, location);

    if (tokens->count != 0) {
        if (tokens->elems->kind != TOKEN_KIND_PLUS) {
            fprintf(stderr, FL_Fmt": ERROR: expected %s but found %s\n",
                    FL_Arg(location),
                    token_kind_name(TOKEN_KIND_PLUS),
                    token_kind_name(tokens->elems->kind));
            exit(1);
        }

        tv_chop_left(tokens, 1);

        Expr right = parse_expr_from_tokens(arena, tokens, location);

        Binary_Op *binary_op = arena_alloc(arena, sizeof(Binary_Op));
        binary_op->kind = BINARY_OP_PLUS;
        binary_op->left = left;
        binary_op->right = right;

        Expr result = {
            .kind = EXPR_KIND_BINARY_OP,
            .value = {
                .as_binary_op = binary_op,
            }
        };

        return result;
    } else {
        return left;
    }
}

Expr parse_expr_from_tokens(Arena *arena, Tokens_View *tokens, File_Location location)
{
    return parse_sum_from_tokens(arena, tokens, location);
}

Expr parse_expr_from_sv(Arena *arena, String_View source, File_Location location)
{
    Tokens tokens = {0};
    tokenize(source, &tokens, location);

    Tokens_View tv = tokens_as_view(&tokens);
    return parse_expr_from_tokens(arena, &tv, location);
}

#endif // BM_IMPLEMENTATION
