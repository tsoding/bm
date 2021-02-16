#ifndef BASM_H_
#define BASM_H_

#include "./sv.h"
#include "./arena.h"
#include "./bm.h"

#define BASM_BINDINGS_CAPACITY 1024
#define BASM_DEFERRED_OPERANDS_CAPACITY 1024
#define BASM_DEFERRED_ASSERTS_CAPACITY 1024
#define BASM_STRING_LENGTHS_CAPACITY 1024
#define BASM_COMMENT_SYMBOL ';'
#define BASM_PP_SYMBOL '%'
#define BASM_MAX_INCLUDE_LEVEL 69

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
    TOKEN_KIND_OPEN_PAREN,
    TOKEN_KIND_CLOSING_PAREN,
    TOKEN_KIND_COMMA,
    TOKEN_KIND_GT,
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
    EXPR_KIND_FUNCALL,
} Expr_Kind;

typedef struct Binary_Op Binary_Op;
typedef struct Funcall Funcall;
typedef struct Funcall_Arg Funcall_Arg;

size_t funcall_args_len(Funcall_Arg *args);

typedef union {
    String_View as_binding;
    uint64_t as_lit_int;
    double as_lit_float;
    char as_lit_char;
    String_View as_lit_str;
    Binary_Op *as_binary_op;
    Funcall *as_funcall;
} Expr_Value;

typedef struct {
    Expr_Kind kind;
    Expr_Value value;
} Expr;

void dump_expr(FILE *stream, Expr expr, int level);
void dump_expr_as_dot(FILE *stream, Expr expr);

typedef enum {
    BINARY_OP_PLUS,
    BINARY_OP_GT
} Binary_Op_Kind;

const char *binary_op_kind_name(Binary_Op_Kind kind);
void dump_binary_op(FILE *stream, Binary_Op binary_op, int level);

struct Binary_Op {
    Binary_Op_Kind kind;
    Expr left;
    Expr right;
};

struct Funcall_Arg {
    Funcall_Arg *next;
    Expr value;
};

struct Funcall {
    String_View name;
    Funcall_Arg *args;
};

void dump_funcall_args(FILE *stream, Funcall_Arg *args, int level);
Funcall_Arg *parse_funcall_args(Arena *arena, Tokens_View *tokens, File_Location location);
Expr parse_sum_from_tokens(Arena *arena, Tokens_View *tokens, File_Location location);
Expr parse_gt_from_tokens(Arena *arena, Tokens_View *tokens, File_Location location);
Expr parse_primary_from_tokens(Arena *arena, Tokens_View *tokens, File_Location location);
Expr parse_expr_from_tokens(Arena *arena, Tokens_View *tokens, File_Location location);
Expr parse_expr_from_sv(Arena *arena, String_View source, File_Location location);

typedef enum {
    BINDING_UNEVALUATED = 0,
    BINDING_EVALUATING,
    BINDING_EVALUATED,
} Binding_Status;

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
    Inst_Addr addr;
    uint64_t length;
} String_Length;

typedef struct {
    Expr expr;
    File_Location location;
} Deferred_Assert;

typedef struct {
    Binding bindings[BASM_BINDINGS_CAPACITY];
    size_t bindings_size;

    Deferred_Operand deferred_operands[BASM_DEFERRED_OPERANDS_CAPACITY];
    size_t deferred_operands_size;

    Deferred_Assert deferred_asserts[BASM_DEFERRED_ASSERTS_CAPACITY];
    size_t deferred_asserts_size;

    Inst program[BM_PROGRAM_CAPACITY];
    uint64_t program_size;
    Inst_Addr entry;
    bool has_entry;
    File_Location entry_location;
    String_View deferred_entry_binding_name;

    String_Length string_lengths[BASM_STRING_LENGTHS_CAPACITY];
    size_t string_lengths_size;

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
bool basm_string_length_by_addr(Basm *basm, Inst_Addr addr, Word *length);
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

#endif // BASM_H_
