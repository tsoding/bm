#ifndef STATEMENT_H_
#define STATEMENT_H_

#include "./bm.h"
#include "./expr.h"
#include "./linizer.h"

typedef struct Statement Statement;
typedef struct Block_Statement Block_Statement;
typedef struct Fundef_Statement Fundef_Statement;

typedef enum {
    STATEMENT_KIND_EMIT_INST,
    STATEMENT_KIND_BIND_LABEL,
    STATEMENT_KIND_BIND_CONST,
    STATEMENT_KIND_BIND_NATIVE,
    STATEMENT_KIND_INCLUDE,
    STATEMENT_KIND_ASSERT,
    STATEMENT_KIND_ERROR,
    STATEMENT_KIND_ENTRY,
    STATEMENT_KIND_BLOCK,
    STATEMENT_KIND_IF,
    STATEMENT_KIND_SCOPE,
    STATEMENT_KIND_FOR,
    STATEMENT_KIND_FUNCDEF,
    STATEMENT_KIND_MACROCALL,
    STATEMENT_KIND_MACRODEF
} Statement_Kind;

typedef struct {
    Inst_Type type;
    Expr operand;
} Emit_Inst_Statement;

typedef struct {
    String_View name;
} Bind_Label_Statement;

typedef struct {
    String_View name;
    Expr value;
} Bind_Const_Statement;

typedef struct {
    String_View name;
} Bind_Native_Statement;

typedef struct {
    String_View path;
} Include_Statement;

typedef struct {
    Expr condition;
} Assert_Statement;

typedef struct {
    String_View message;
} Error_Statement;

typedef struct {
    Expr value;
} Entry_Statement;

typedef struct {
    Expr condition;
    Block_Statement *then;
    Block_Statement *elze;
} If_Statement;

typedef struct {
    String_View var;
    Expr from;
    Expr to;
    Block_Statement *body;
} For_Statement;

struct Fundef_Statement {
    String_View name;
    Fundef_Arg *args;
    Expr *guard;
    Expr body;
};

typedef struct {
    String_View name;
    Funcall_Arg *args;
} Macrocall;

typedef struct {
    String_View name;
    Fundef_Arg *args;
    Block_Statement *body;
} Macrodef_Statement;

typedef union {
    Emit_Inst_Statement as_emit_inst;
    Bind_Label_Statement as_bind_label;
    Bind_Const_Statement as_bind_const;
    Bind_Native_Statement as_bind_native;
    Include_Statement as_include;
    Assert_Statement as_assert;
    Error_Statement as_error;
    Entry_Statement as_entry;
    Block_Statement *as_block;
    If_Statement as_if;
    Block_Statement *as_scope;
    For_Statement as_for;
    Fundef_Statement as_fundef;
    Macrocall as_macrocall;
    // TODO(#319): all of the Statement kind types should have the `_Statement` suffix
    // Not only Macrodef_Statement.
    Macrodef_Statement as_macrodef;
} Statement_Value;

struct Statement {
    Statement_Kind kind;
    Statement_Value value;
    File_Location location;
};

struct Block_Statement {
    Statement statement;
    Block_Statement *next;
};

typedef struct {
    Block_Statement *begin;
    Block_Statement *end;
} Block_List;

void block_list_push(Arena *arena, Block_List *list, Statement statement);

void dump_block(FILE *stream, Block_Statement *block, int level);
void dump_statement(FILE *stream, Statement statement, int level);
int dump_block_as_dot_edges(FILE *stream, Block_Statement *block, int *counter);
int dump_statement_as_dot_edges(FILE *stream, Statement statement, int *counter);
void dump_statement_as_dot(FILE *stream, Statement statement);

Statement parse_if_else_body_from_lines(Arena *arena, Linizer *linizer,
                                        Expr condition, File_Location location);
void parse_directive_from_line(Arena *arena, Linizer *linizer, Block_List *output);
Block_Statement *parse_block_from_lines(Arena *arena, Linizer *linizer);

#endif // STATEMENT_H_
