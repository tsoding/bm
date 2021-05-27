#ifndef BANG_COMPILER_H_
#define BANG_COMPILER_H_

#include "./basm.h"
#include "./bang_lexer.h"
#include "./bang_parser.h"

void compile_bang_expr_into_basm(Basm *basm, Bang_Expr expr, Native_ID write_id);
void compile_stmt_into_basm(Basm *basm, Bang_Stmt stmt, Native_ID write_id);
void compile_block_into_basm(Basm *basm, Bang_Block *block, Native_ID write_id);
void compile_proc_def_into_basm(Basm *basm, Bang_Proc_Def proc_def, Native_ID write_id);
void compile_bang_if_into_basm(Basm *basm, Bang_If eef, Native_ID write_id);

void bang_funcall_expect_arity(Bang_Funcall funcall, size_t expected_arity);

#endif // BANG_COMPILER_H_
