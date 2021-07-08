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
typedef struct Bang_Proc_Param Bang_Proc_Param;
typedef struct Bang_Var_Def Bang_Var_Def;
typedef union Bang_Top_As Bang_Top_As;
typedef struct Bang_Top Bang_Top;
typedef struct Bang_Module Bang_Module;
typedef struct Bang_Var_Assign Bang_Var_Assign;
typedef struct Bang_Var_Read Bang_Var_Read;
typedef struct Bang_While Bang_While;
typedef struct Bang_For Bang_For;
typedef struct Bang_Binary_Op Bang_Binary_Op;
typedef struct Bang_Binary_Op_Def Bang_Binary_Op_Def;
typedef struct Dynarray_Of_Bang_Proc_Param Dynarray_Of_Bang_Proc_Param;
typedef struct Dynarray_Of_Bang_Funcall_Arg Dynarray_Of_Bang_Funcall_Arg;
typedef struct Bang_Range Bang_Range;

typedef enum {
    BANG_BINARY_OP_KIND_PLUS = 0,
    BANG_BINARY_OP_KIND_MINUS,
    BANG_BINARY_OP_KIND_MULT,
    BANG_BINARY_OP_KIND_LT,
    BANG_BINARY_OP_KIND_GE,
    BANG_BINARY_OP_KIND_NE,
    BANG_BINARY_OP_KIND_AND,
    BANG_BINARY_OP_KIND_OR,
    BANG_BINARY_OP_KIND_EQ,
    COUNT_BANG_BINARY_OP_KINDS,
} Bang_Binary_Op_Kind;

typedef enum {
    BINARY_OP_PREC0 = 0,
    BINARY_OP_PREC1 = 1,
    BINARY_OP_PREC2 = 2,
    BINARY_OP_PREC3 = 3,
    COUNT_BINARY_OP_PRECS,
} Binary_Op_Prec;

struct Bang_Binary_Op_Def {
    Bang_Binary_Op_Kind kind;
    Bang_Token_Kind token_kind;
    Binary_Op_Prec prec;
};

// TODO(#440): there are no unary operators in Bang

struct Dynarray_Of_Bang_Funcall_Arg {
    size_t size;
    size_t capacity;
    Bang_Funcall_Arg *items;
};

struct Bang_Funcall {
    Bang_Loc loc;
    String_View name;
    Dynarray_Of_Bang_Funcall_Arg args;
};

typedef enum {
    BANG_EXPR_KIND_LIT_STR,
    BANG_EXPR_KIND_LIT_BOOL,
    BANG_EXPR_KIND_LIT_INT,
    BANG_EXPR_KIND_FUNCALL,
    BANG_EXPR_KIND_VAR_READ,
    BANG_EXPR_KIND_BINARY_OP,
    COUNT_BANG_EXPR_KINDS,
} Bang_Expr_Kind;

struct Bang_Var_Read {
    Bang_Loc loc;
    String_View name;
};

union Bang_Expr_As {
    String_View lit_str;
    int64_t lit_int;
    Bang_Funcall funcall;
    bool boolean;
    Bang_Var_Read var_read;
    Bang_Binary_Op *binary_op;
};
static_assert(
    COUNT_BANG_EXPR_KINDS == 6,
    "The amount of expression kinds has changed. "
    "Please update the union of those expressions accordingly. "
    "Thanks!");

struct Bang_Expr {
    Bang_Loc loc;
    Bang_Expr_Kind kind;
    Bang_Expr_As as;
};

struct Bang_Binary_Op {
    Bang_Loc loc;
    Bang_Binary_Op_Kind kind;
    Bang_Expr lhs;
    Bang_Expr rhs;
};

struct Bang_Funcall_Arg {
    Bang_Expr value;
    Bang_Funcall_Arg *next;
};

typedef enum {
    BANG_STMT_KIND_EXPR = 0,
    BANG_STMT_KIND_IF,
    BANG_STMT_KIND_VAR_ASSIGN,
    BANG_STMT_KIND_VAR_DEF,
    BANG_STMT_KIND_WHILE,
    BANG_STMT_KIND_FOR,
    COUNT_BANG_STMT_KINDS,
} Bang_Stmt_Kind;

struct Bang_Range {
    Bang_Expr low;
    Bang_Expr high;
};

struct Bang_Block {
    size_t size;
    size_t capacity;
    Bang_Stmt *items;
};

struct Bang_For {
    Bang_Loc loc;
    String_View iter_name;
    String_View iter_type_name;
    Bang_Range range;
    Bang_Block body;
};

struct Bang_While {
    Bang_Loc loc;
    Bang_Expr condition;
    Bang_Block body;
};

struct Bang_If {
    Bang_Loc loc;
    Bang_Expr condition;
    Bang_Block then;
    Bang_Block elze;
};

struct Bang_Var_Assign {
    Bang_Loc loc;
    String_View name;
    Bang_Expr value;
};

struct Bang_Var_Def {
    Bang_Loc loc;
    String_View name;
    String_View type_name;

    Bang_Expr init;
    bool has_init;
};

union Bang_Stmt_As {
    Bang_Expr expr;
    Bang_If eef;
    Bang_Var_Assign var_assign;
    Bang_While hwile;
    Bang_Var_Def var_def;
    Bang_For forr;
};
static_assert(
    COUNT_BANG_STMT_KINDS == 6,
    "The amount of statement kinds has changed. "
    "Please update the union of those statements accordingly. "
    "Thanks!");

struct Bang_Stmt {
    Bang_Stmt_Kind kind;
    Bang_Stmt_As as;
};

struct Bang_Proc_Param {
    Bang_Loc loc;
    String_View name;
    String_View type_name;
};

struct Dynarray_Of_Bang_Proc_Param {
    size_t size;
    size_t capacity;
    Bang_Proc_Param *items;
};

struct Bang_Proc_Def {
    Bang_Loc loc;
    String_View name;
    Dynarray_Of_Bang_Proc_Param params;
    Bang_Block body;
};

typedef enum {
    BANG_TOP_KIND_PROC = 0,
    BANG_TOP_KIND_VAR,
    COUNT_BANG_TOP_KINDS,
} Bang_Top_Kind;

union Bang_Top_As {
    Bang_Var_Def var;
    Bang_Proc_Def proc;
};

struct Bang_Top {
    Bang_Top_Kind kind;
    Bang_Top_As as;
    Bang_Top *next;
};

struct Bang_Module {
    Bang_Top *tops_begin;
    Bang_Top *tops_end;
};

void bang_module_push_top(Bang_Module *module, Bang_Top *top);

Bang_Binary_Op_Def bang_binary_op_def(Bang_Binary_Op_Kind kind);
String_View parse_bang_lit_str(Arena *arena, Bang_Lexer *lexer);
Dynarray_Of_Bang_Funcall_Arg parse_bang_funcall_args(Arena *arena, Bang_Lexer *lexer);
Bang_Funcall parse_bang_funcall(Arena *arena, Bang_Lexer *lexer);
Bang_Expr parse_bang_expr(Arena *arena, Bang_Lexer *lexer);
Bang_Block parse_curly_bang_block(Arena *arena, Bang_Lexer *lexer);
Bang_If parse_bang_if(Arena *arena, Bang_Lexer *lexer);
Bang_Stmt parse_bang_stmt(Arena *arena, Bang_Lexer *lexer);
Dynarray_Of_Bang_Proc_Param parse_bang_proc_params(Arena *arena, Bang_Lexer *lexer);
Bang_Proc_Def parse_bang_proc_def(Arena *arena, Bang_Lexer *lexer);
Bang_Top parse_bang_top(Arena *arena, Bang_Lexer *lexer);
Bang_Var_Def parse_bang_var_def(Arena *arena, Bang_Lexer *lexer);
Bang_Module parse_bang_module(Arena *arena, Bang_Lexer *lexer);
Bang_Var_Assign parse_bang_var_assign(Arena *arena, Bang_Lexer *lexer);
Bang_While parse_bang_while(Arena *arena, Bang_Lexer *lexer);
Bang_For parse_bang_for(Arena *arena, Bang_Lexer *lexer);
Bang_Range parse_bang_range(Arena *arena, Bang_Lexer *lexer);
Bang_Var_Read parse_var_read(Bang_Lexer *lexer);


#endif // BANG_PARSER_H_
