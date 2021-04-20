#ifndef STATEMENT_H_
#define STATEMENT_H_

#include "./bm.h"
#include "./expr.h"
#include "./linizer.h"

typedef struct Statement Statement;
typedef struct Block Block;
typedef struct Fundef Fundef;

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
} Bind_Const;

typedef struct {
    String_View name;
} Bind_Native;

typedef struct {
    String_View path;
} Include;

typedef struct {
    Expr condition;
} Assert;

typedef struct {
    String_View message;
} Error;

typedef struct {
    Expr value;
} Entry;

typedef struct {
    Expr condition;
    Block *then;
    Block *elze;
} If;

typedef struct {
    String_View var;
    Expr from;
    Expr to;
    Block *body;
} For;

struct Fundef {
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
    Block *body;
} Macrodef_Statement;

typedef union {
    Emit_Inst_Statement as_emit_inst;
    Bind_Label_Statement as_bind_label;
    Bind_Const as_bind_const;
    Bind_Native as_bind_native;
    Include as_include;
    Assert as_assert;
    Error as_error;
    Entry as_entry;
    Block *as_block;
    If as_if;
    Block *as_scope;
    For as_for;
    Fundef as_fundef;
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

struct Block {
    Statement statement;
    Block *next;
};

typedef struct {
    Block *begin;
    Block *end;
} Block_List;

void block_list_push(Arena *arena, Block_List *list, Statement statement);

void dump_block(FILE *stream, Block *block, int level);
void dump_statement(FILE *stream, Statement statement, int level);
int dump_block_as_dot_edges(FILE *stream, Block *block, int *counter);
int dump_statement_as_dot_edges(FILE *stream, Statement statement, int *counter);
void dump_statement_as_dot(FILE *stream, Statement statement);

Statement parse_if_else_body_from_lines(Arena *arena, Linizer *linizer,
                                        Expr condition, File_Location location);
void parse_directive_from_line(Arena *arena, Linizer *linizer, Block_List *output);
Block *parse_block_from_lines(Arena *arena, Linizer *linizer);

#endif // STATEMENT_H_
