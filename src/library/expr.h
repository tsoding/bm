#ifndef EXPR_H_
#define EXPR_H_

#include <stdio.h>
#include "sv.h"
#include "arena.h"

typedef struct {
    String_View file_path;
    int line_number;
} File_Location;

#define FL_Fmt SV_Fmt":%d"
#define FL_Arg(location) SV_Arg(location.file_path), location.line_number

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

#define Token_Fmt "%s"
#define Token_Arg(token) token_kind_name((token).kind)

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

#endif // EXPR_H_
