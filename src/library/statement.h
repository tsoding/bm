#ifndef STATEMENT_H_
#define STATEMENT_H_

#include "./bm.h"
#include "./expr.h"
#include "./linizer.h"

typedef struct Statement Statement;
typedef struct Block Block;

typedef enum {
    STATEMENT_KIND_EMIT_INST,
    STATEMENT_KIND_BIND_LABEL,
    STATEMENT_KIND_BLOCK,
} Statement_Kind;

typedef struct {
    Inst_Type type;
    Expr operand;
} Emit_Inst;

typedef struct {
    String_View name;
} Bind_Label;

typedef union {
    Emit_Inst as_emit_inst;
    Bind_Label as_bind_label;
    Block *as_block;
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
int dump_statement_as_dot_edges(FILE *stream, Statement statement, int *counter);
void dump_statement_as_dot(FILE *stream, Statement statement);

Block *parse_block_from_lines(Arena *arena, Linizer *linizer);

#endif // STATEMENT_H_
