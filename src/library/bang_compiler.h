#ifndef BANG_COMPILER_H_
#define BANG_COMPILER_H_

#include "./basm.h"
#include "./bang_lexer.h"
#include "./bang_parser.h"

#define BANG_GLOBAL_VARS_CAPACITY 1024
#define BANG_PROCS_CAPACITY 1024

// TODO(#433): there is no generic ptr type
typedef enum {
    BANG_TYPE_VOID = 0,
    BANG_TYPE_I64,
    BANG_TYPE_U8,
    BANG_TYPE_BOOL,
    BANG_TYPE_PTR,
    COUNT_BANG_TYPES,
} Bang_Type;

bool bang_type_by_name(String_View name, Bang_Type *out_type);

typedef struct {
    const char *name;
    size_t size;
    Inst_Type read;
    Inst_Type write;
} Bang_Type_Def;

Bang_Type_Def bang_type_def(Bang_Type type);

typedef struct {
    Bang_Var_Def def;
    Bang_Type type;
    Memory_Addr addr;
} Compiled_Var;

typedef struct {
    Bang_Proc_Def def;
    Inst_Addr addr;
} Compiled_Proc;

typedef struct {
    Native_ID write_id;

    Compiled_Var global_vars[BANG_GLOBAL_VARS_CAPACITY];
    size_t global_vars_count;

    Compiled_Proc procs[BANG_PROCS_CAPACITY];
    size_t procs_count;

    Bang_Loc entry_loc;
} Bang;

void compile_begin_begin(Bang *bang);

Compiled_Var *bang_get_global_var_by_name(Bang *bang, String_View name);

typedef struct {
    Bang_Expr ast;
    Inst_Addr addr;
    Bang_Type type;
} Compiled_Expr;

Compiled_Proc *bang_get_compiled_proc_by_name(Bang *bang, String_View name);

void compile_typed_read(Basm *basm, Bang_Type type);
void compile_typed_write(Basm *basm, Bang_Type type);
Compiled_Expr compile_bang_expr_into_basm(Bang *bang, Basm *basm, Bang_Expr expr);
void compile_stmt_into_basm(Bang *bang, Basm *basm, Bang_Stmt stmt);
void compile_block_into_basm(Bang *bang, Basm *basm, Bang_Block *block);
void compile_proc_def_into_basm(Bang *bang, Basm *basm, Bang_Proc_Def proc_def);
void compile_bang_if_into_basm(Bang *bang, Basm *basm, Bang_If eef);
void compile_bang_while_into_basm(Bang *bang, Basm *basm, Bang_While hwile);
void compile_bang_var_assign_into_basm(Bang *bang, Basm *basm, Bang_Var_Assign var_assign);
void compile_bang_module_into_basm(Bang *bang, Basm *basm, Bang_Module module);
void compile_var_def_into_basm(Bang *bang, Basm *basm, Bang_Var_Def var_def);
Bang_Type compile_var_read_into_basm(Bang *bang, Basm *basm, Bang_Var_Read var_read);
Bang_Type compile_binary_op_into_basm(Bang *bang, Basm *basm, Bang_Binary_Op binary_op);

void bang_generate_entry_point(Bang *bang, Basm *basm, String_View entry_proc_name);
void bang_generate_heap_base(Bang *bang, Basm *basm, String_View heap_base_var_name);

void bang_funcall_expect_arity(Bang_Funcall funcall, size_t expected_arity);

#endif // BANG_COMPILER_H_
