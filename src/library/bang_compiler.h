#ifndef BANG_COMPILER_H_
#define BANG_COMPILER_H_

#include "./basm.h"
#include "./bang_lexer.h"
#include "./bang_parser.h"

#define BANG_GLOBAL_VARS_CAPACITY 1024

typedef struct {
    Bang_Loc loc;
    Memory_Addr addr;
    String_View name;
} Bang_Global_Var;

typedef struct {
    Native_ID write_id;
    Bang_Global_Var global_vars[BANG_GLOBAL_VARS_CAPACITY];
    size_t global_vars_count;
} Bang;

void compile_begin_begin(Bang *bang);

Bang_Global_Var *bang_get_global_var_by_name(Bang *bang, String_View name);

Inst_Addr compile_bang_expr_into_basm(Bang *bang, Basm *basm, Bang_Expr expr);
void compile_stmt_into_basm(Bang *bang, Basm *basm, Bang_Stmt stmt);
void compile_block_into_basm(Bang *bang, Basm *basm, Bang_Block *block);
void compile_proc_def_into_basm(Bang *bang, Basm *basm, Bang_Proc_Def proc_def);
void compile_bang_if_into_basm(Bang *bang, Basm *basm, Bang_If eef);
void compile_bang_while_into_basm(Bang *bang, Basm *basm, Bang_While hwile);
void compile_bang_var_assign_into_basm(Bang *bang, Basm *basm, Bang_Var_Assign var_assign);
void compile_bang_module_into_basm(Bang *bang, Basm *basm, Bang_Module module);
void compile_var_def_into_basm(Bang *bang, Basm *basm, Bang_Var_Def var_def);

void bang_funcall_expect_arity(Bang_Funcall funcall, size_t expected_arity);

#endif // BANG_COMPILER_H_
