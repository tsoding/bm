#ifndef BANG_PARSER_H_
#define BANG_PARSER_H_

#include "./arena.h"
#include "./bang_lexer.h"

typedef struct Bang_Funcall_Arg Bang_Funcall_Arg;
typedef struct Bang_Funcall Bang_Funcall;
typedef union Bang_Expr_As Bang_Expr_As;
typedef struct Bang_Expr Bang_Expr;
typedef struct Bang_Funcall_Arg Bang_Funcall_Arg;
typedef struct Bang_Stmt Bang_Stmt;
typedef struct Bang_If Bang_If;
typedef union Bang_Stmt_As Bang_Stmt_As;
typedef struct Bang_Block Bang_Block;
typedef struct Bang_Proc_Def Bang_Proc_Def;

struct Bang_Funcall {
    Bang_Loc loc;
    String_View name;
    Bang_Funcall_Arg *args;
};

union Bang_Expr_As {
    String_View lit_str;
    Bang_Funcall funcall;
    bool boolean;
};

typedef enum {
    BANG_EXPR_KIND_LIT_STR,
    BANG_EXPR_KIND_LIT_BOOL,
    BANG_EXPR_KIND_FUNCALL,
} Bang_Expr_Kind;

struct Bang_Expr {
    Bang_Expr_Kind kind;
    Bang_Expr_As as;
};

struct Bang_Funcall_Arg {
    Bang_Expr value;
    Bang_Funcall_Arg *next;
};

typedef enum {
    BANG_STMT_KIND_EXPR,
    BANG_STMT_KIND_IF,
} Bang_Stmt_Kind;

struct Bang_If {
    Bang_Loc loc;
    Bang_Expr condition;
    Bang_Block *then;
    Bang_Block *elze;
};

union Bang_Stmt_As {
    Bang_Expr expr;
    Bang_If eef;
};

struct Bang_Stmt {
    Bang_Stmt_Kind kind;
    Bang_Stmt_As as;
};

struct Bang_Block {
    Bang_Stmt stmt;
    Bang_Block *next;
};

struct Bang_Proc_Def {
    String_View name;
    Bang_Block *body;
};

String_View parse_bang_lit_str(Arena *arena, Bang_Lexer *lexer);
Bang_Funcall_Arg *parse_bang_funcall_args(Arena *arena, Bang_Lexer *lexer);
Bang_Funcall parse_bang_funcall(Arena *arena, Bang_Lexer *lexer);
Bang_Expr parse_bang_expr(Arena *arena, Bang_Lexer *lexer);
Bang_Block *parse_curly_bang_block(Arena *arena, Bang_Lexer *lexer);
Bang_Proc_Def parse_bang_proc_def(Arena *arena, Bang_Lexer *lexer);
Bang_If parse_bang_if(Arena *arena, Bang_Lexer *lexer);
Bang_Stmt parse_bang_stmt(Arena *arena, Bang_Lexer *lexer);

#endif // BANG_PARSER_H_
