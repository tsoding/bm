#ifndef BANG_COMPILER_H_
#define BANG_COMPILER_H_

#include "./compiler.h"
#include "./bm.h"
#include "./bang_lexer.h"
#include "./bang_parser.h"

#define BANG_SCOPE_VARS_CAPACITY 1024
#define BANG_PROCS_CAPACITY 1024

typedef enum {
    BANG_VAR_STATIC_STORAGE = 0,
    BANG_VAR_STACK_STORAGE,
} Bang_Var_Storage;

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
    Bang_Var_Storage storage;
    // when storage == BANG_VAR_STATIC_STORAGE:
    //   addr is absolute address of the variable in memory
    // when storage == BANG_VAR_STACK_STORAGE:
    //   addr is an offset from the stack frame
    Memory_Addr addr;
} Compiled_Var;

typedef struct {
    Bang_Proc_Def def;
    Inst_Addr addr;
} Compiled_Proc;

typedef struct Bang_Scope Bang_Scope;

struct Bang_Scope {
    Bang_Scope *parent;
    Compiled_Var vars[BANG_SCOPE_VARS_CAPACITY];
    size_t vars_count;
};

Compiled_Var *bang_scope_get_compiled_var_by_name(Bang_Scope *scope, String_View
 name);
void bang_scope_push_var(Bang_Scope *scope, Compiled_Var var);

#define BANG_STACK_CAPACITY 32

#define STACK_TOP_VAR_TYPE BANG_TYPE_I64
#define STACK_FRAME_VAR_TYPE BANG_TYPE_I64

typedef struct {
    Arena arena;

    Native_ID write_id;

    Memory_Addr stack_top_var_addr;
    Memory_Addr stack_frame_var_addr;

    Bang_Scope *scope;

    Compiled_Proc procs[BANG_PROCS_CAPACITY];
    size_t procs_count;

    Bang_Loc entry_loc;
} Bang;

Compiled_Var *bang_get_compiled_var_by_name(Bang *bang, String_View name);

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
void compile_static_var_def_into_basm(Bang *bang, Basm *basm, Bang_Var_Def var_def);
void compile_stack_var_def_into_basm(Bang *bang, Basm *basm, Bang_Var_Def var_def);
Bang_Type compile_var_read_into_basm(Bang *bang, Basm *basm, Bang_Var_Read var_read);
Bang_Type compile_binary_op_into_basm(Bang *bang, Basm *basm, Bang_Binary_Op binary_op);

void bang_push_new_scope(Bang *bang);
void bang_pop_scope(Bang *bang);

void bang_generate_entry_point(Bang *bang, Basm *basm, String_View entry_proc_name);
void bang_generate_heap_base(Bang *bang, Basm *basm, String_View heap_base_var_name);

void bang_funcall_expect_arity(Bang_Funcall funcall, size_t expected_arity);

void bang_prepare_var_stack(Bang *bang, Basm *basm);

#endif // BANG_COMPILER_H_
