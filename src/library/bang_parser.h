#ifndef BANG_PARSER_H_
#define BANG_PARSER_H_

#include "./arena.h"
#include "./bang_lexer.h"

typedef enum {
    BANG_EXPR_KIND_LIT_STR,
    BANG_EXPR_KIND_FUNCALL,
} Bang_Expr_Kind;

typedef struct Bang_Funcall_Arg Bang_Funcall_Arg;

typedef struct {
    Bang_Loc loc;
    String_View name;
    Bang_Funcall_Arg *args;
} Bang_Funcall;

typedef union {
    String_View as_lit_str;
    Bang_Funcall as_funcall;
} Bang_Expr_Value;

typedef struct {
    Bang_Expr_Kind kind;
    Bang_Expr_Value value;
} Bang_Expr;

typedef struct Bang_Funcall_Arg Bang_Funcall_Arg;

struct Bang_Funcall_Arg {
    Bang_Expr value;
    Bang_Funcall_Arg *next;
};

typedef enum {
    BANG_STMT_KIND_EXPR,
    BANG_STMT_KIND_IF,
} Bang_Stmt_Kind;

typedef struct Bang_Stmt Bang_Stmt;

typedef struct {
    Bang_Expr condition;
    Bang_Stmt *then;
    Bang_Stmt *elze;
} Bang_If;

typedef union {
    Bang_Expr expr;
    Bang_If eef;
} Bang_Stmt_As;

struct Bang_Stmt {
    Bang_Stmt_Kind kind;
    Bang_Stmt_As as;
};

typedef struct Bang_Block Bang_Block;

struct Bang_Block {
    Bang_Stmt stmt;
    Bang_Block *next;
};

typedef struct {
    String_View name;
    Bang_Block *body;
} Bang_Proc_Def;

String_View parse_bang_lit_str(Arena *arena, Bang_Lexer *lexer);
Bang_Funcall_Arg *parse_bang_funcall_args(Arena *arena, Bang_Lexer *lexer);
Bang_Funcall parse_bang_funcall(Arena *arena, Bang_Lexer *lexer);
Bang_Expr parse_bang_expr(Arena *arena, Bang_Lexer *lexer);
Bang_Block *parse_curly_bang_block(Arena *arena, Bang_Lexer *lexer);
Bang_Proc_Def parse_bang_proc_def(Arena *arena, Bang_Lexer *lexer);
Bang_Stmt parse_bang_stmt(Arena *arena, Bang_Lexer *lexer);

#endif // BANG_PARSER_H_
