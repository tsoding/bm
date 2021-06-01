#ifndef BANG_COMPILER_H_
#define BANG_COMPILER_H_

#include "./basm.h"
#include "./bang_lexer.h"
#include "./bang_parser.h"

typedef struct {
    Native_ID write_id;
} Bang;

void compile_begin_begin(Bang *bang);

void compile_bang_expr_into_basm(Bang *bang, Basm *basm, Bang_Expr expr);
void compile_stmt_into_basm(Bang *bang, Basm *basm, Bang_Stmt stmt);
void compile_block_into_basm(Bang *bang, Basm *basm, Bang_Block *block);
void compile_proc_def_into_basm(Bang *bang, Basm *basm, Bang_Proc_Def proc_def);
void compile_bang_if_into_basm(Bang *bang, Basm *basm, Bang_If eef);
void compile_bang_module_into_basm(Bang *bang, Basm *basm, Bang_Module module);

void bang_funcall_expect_arity(Bang_Funcall funcall, size_t expected_arity);

#endif // BANG_COMPILER_H_
