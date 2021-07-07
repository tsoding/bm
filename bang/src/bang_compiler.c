#include "./error.h"
#include "./bang_compiler.h"

// TODO(#426): bang does not support type casting

typedef struct {
    bool exists;
    Inst_Type inst;
    Bang_Type ret;
} Binary_Op_Of_Type;

static const Binary_Op_Of_Type binary_op_of_type_table[COUNT_BANG_TYPES][COUNT_BANG_BINARY_OP_KINDS] = {
    [BANG_TYPE_VOID] = {
        [BANG_BINARY_OP_KIND_PLUS]  = {.exists = false},
        [BANG_BINARY_OP_KIND_MINUS] = {.exists = false},
        [BANG_BINARY_OP_KIND_MULT]  = {.exists = false},
        [BANG_BINARY_OP_KIND_LT]    = {.exists = false},
        [BANG_BINARY_OP_KIND_GE]    = {.exists = false},
        [BANG_BINARY_OP_KIND_NE]    = {.exists = false},
        [BANG_BINARY_OP_KIND_AND]   = {.exists = false},
        [BANG_BINARY_OP_KIND_OR]    = {.exists = false},
        [BANG_BINARY_OP_KIND_EQ]    = {.exists = false},
    },
    [BANG_TYPE_I64] = {
        [BANG_BINARY_OP_KIND_PLUS]  = {.exists = true, .inst = INST_PLUSI,  .ret = BANG_TYPE_I64},
        [BANG_BINARY_OP_KIND_MINUS] = {.exists = true, .inst = INST_MINUSI, .ret = BANG_TYPE_I64},
        [BANG_BINARY_OP_KIND_MULT]  = {.exists = true, .inst = INST_MULTI,  .ret = BANG_TYPE_I64},
        [BANG_BINARY_OP_KIND_LT]    = {.exists = true, .inst = INST_LTI,    .ret = BANG_TYPE_BOOL},
        [BANG_BINARY_OP_KIND_GE]    = {.exists = true, .inst = INST_GEI,    .ret = BANG_TYPE_BOOL},
        [BANG_BINARY_OP_KIND_NE]    = {.exists = true, .inst = INST_NEI,    .ret = BANG_TYPE_BOOL},
        [BANG_BINARY_OP_KIND_AND]   = {.exists = false},
        [BANG_BINARY_OP_KIND_OR]    = {.exists = false},
        [BANG_BINARY_OP_KIND_EQ]    = {.exists = true, .inst = INST_EQI,    .ret = BANG_TYPE_BOOL},
    },
    [BANG_TYPE_U8] = {
        [BANG_BINARY_OP_KIND_PLUS]  = {.exists = true, .inst = INST_PLUSI,  .ret = BANG_TYPE_U8},
        [BANG_BINARY_OP_KIND_MINUS] = {.exists = true, .inst = INST_MINUSI, .ret = BANG_TYPE_U8},
        [BANG_BINARY_OP_KIND_MULT]  = {.exists = true, .inst = INST_MULTU,  .ret = BANG_TYPE_U8},
        [BANG_BINARY_OP_KIND_LT]    = {.exists = true, .inst = INST_LTU,    .ret = BANG_TYPE_BOOL},
        [BANG_BINARY_OP_KIND_GE]    = {.exists = true, .inst = INST_GEU,    .ret = BANG_TYPE_BOOL},
        [BANG_BINARY_OP_KIND_NE]    = {.exists = true, .inst = INST_NEU,    .ret = BANG_TYPE_BOOL},
        [BANG_BINARY_OP_KIND_AND]   = {.exists = false},
        [BANG_BINARY_OP_KIND_OR]    = {.exists = false},
        [BANG_BINARY_OP_KIND_EQ]    = {.exists = true, .inst = INST_EQU,    .ret = BANG_TYPE_BOOL},
    },
    [BANG_TYPE_BOOL] = {
        [BANG_BINARY_OP_KIND_PLUS]  = {.exists = false},
        [BANG_BINARY_OP_KIND_MINUS] = {.exists = false},
        [BANG_BINARY_OP_KIND_MULT]  = {.exists = false},
        [BANG_BINARY_OP_KIND_LT]    = {.exists = false},
        [BANG_BINARY_OP_KIND_GE]    = {.exists = false},
        [BANG_BINARY_OP_KIND_NE]    = {.exists = false},
        [BANG_BINARY_OP_KIND_AND]   = {.exists = true, .inst = INST_ANDB, .ret = BANG_TYPE_BOOL},
        [BANG_BINARY_OP_KIND_OR]    = {.exists = true, .inst = INST_ORB,  .ret = BANG_TYPE_BOOL},
        [BANG_BINARY_OP_KIND_EQ]    = {.exists = false},
    },
    [BANG_TYPE_PTR] = {
        [BANG_BINARY_OP_KIND_PLUS]  = {.exists = true, .inst = INST_PLUSI,  .ret = BANG_TYPE_PTR},
        [BANG_BINARY_OP_KIND_MINUS] = {.exists = true, .inst = INST_MINUSI, .ret = BANG_TYPE_PTR},
        [BANG_BINARY_OP_KIND_MULT]  = {.exists = true, .inst = INST_MULTU,  .ret = BANG_TYPE_PTR},
        [BANG_BINARY_OP_KIND_LT]    = {.exists = true, .inst = INST_LTU,    .ret = BANG_TYPE_BOOL},
        [BANG_BINARY_OP_KIND_GE]    = {.exists = true, .inst = INST_GEU,    .ret = BANG_TYPE_BOOL},
        [BANG_BINARY_OP_KIND_NE]    = {.exists = true, .inst = INST_NEU,    .ret = BANG_TYPE_BOOL},
        [BANG_BINARY_OP_KIND_AND]   = {.exists = false},
        [BANG_BINARY_OP_KIND_OR]    = {.exists = false},
        [BANG_BINARY_OP_KIND_EQ]    = {.exists = true, .inst = INST_EQU,    .ret = BANG_TYPE_BOOL},
    },
};
static_assert(
    COUNT_BANG_TYPES == 5,
    "The amount of types have changed. "
    "Please update the binary_op_of_type table accordingly. "
    "Thanks!");
static_assert(
    COUNT_BANG_BINARY_OP_KINDS == 9,
    "The amount of binary operations have changed. "
    "Please update the binary_op_of_type table accordingly. "
    "Thanks!");

static Binary_Op_Of_Type binary_op_of_type(Bang_Type type, Bang_Binary_Op_Kind op_kind)
{
    assert(type >= 0);
    assert(type < COUNT_BANG_TYPES);
    assert(op_kind >= 0);
    assert(op_kind < COUNT_BANG_BINARY_OP_KINDS);
    return binary_op_of_type_table[type][op_kind];
}

static Bang_Type_Def const bang_type_defs[COUNT_BANG_TYPES] = {
    [BANG_TYPE_VOID] = {.name  = "void", .size  = 0, .read  = INST_NOP,     .write = INST_NOP,},
    [BANG_TYPE_I64]  = {.name  = "i64",  .size  = 8, .read  = INST_READ64I, .write = INST_WRITE64,},
    [BANG_TYPE_U8]   = {.name  = "u8",   .size  = 1, .read  = INST_READ8U,  .write = INST_WRITE8,},
    [BANG_TYPE_BOOL] = {.name  = "bool", .size  = 8, .read  = INST_READ64U, .write = INST_WRITE64,},
    [BANG_TYPE_PTR]  = {.name  = "ptr",  .size  = 8, .read  = INST_READ64U, .write = INST_WRITE64,},
};
static_assert(
    COUNT_BANG_TYPES == 5,
    "The amount of types have changed. "
    "Please update the type definition table accordingly. "
    "Thanks!");

Bang_Type_Def bang_type_def(Bang_Type type)
{
    assert(type >= 0);
    assert(type < COUNT_BANG_TYPES);
    return bang_type_defs[type];
}

bool bang_type_by_name(String_View name, Bang_Type *out_type)
{
    for (Bang_Type type = 0; type < COUNT_BANG_TYPES; ++type) {
        if (sv_eq(name, sv_from_cstr(bang_type_def(type).name))) {
            if (out_type) {
                *out_type = type;
            }
            return true;
        }
    }
    return false;
}

void compile_typed_read(Basm *basm, Bang_Type type)
{
    basm_push_inst(basm, bang_type_def(type).read, word_u64(0));
}

void compile_typed_write(Basm *basm, Bang_Type type)
{
    basm_push_inst(basm, bang_type_def(type).write, word_u64(0));
}

void compile_get_var_addr(Bang *bang, Basm *basm, Compiled_Var *var)
{
    switch (var->storage) {
    case BANG_VAR_STATIC_STORAGE: {
        basm_push_inst(basm, INST_PUSH, word_u64(var->addr));
    }
    break;

    case BANG_VAR_STACK_STORAGE: {
        compile_read_frame_addr(bang, basm);
        basm_push_inst(basm, INST_PUSH, word_u64(var->addr));
        basm_push_inst(basm, INST_MINUSI, word_u64(0));
    }
    break;
    }
}

Bang_Type compile_var_read_into_basm(Bang *bang, Basm *basm, Bang_Var_Read var_read)
{
    Compiled_Var *var = bang_get_compiled_var_by_name(bang, var_read.name);
    if (var == NULL) {
        bang_diag_msg(var_read.loc, BANG_DIAG_ERROR,
                      "could not read non-existing variable `"SV_Fmt"`",
                      SV_Arg(var_read.name));
        exit(1);
    }

    compile_get_var_addr(bang, basm, var);
    compile_typed_read(basm, var->type);

    return var->type;
}

Bang_Type compile_binary_op_into_basm(Bang *bang, Basm *basm, Bang_Binary_Op binary_op)
{
    const Compiled_Expr compiled_lhs = compile_bang_expr_into_basm(bang, basm, binary_op.lhs);
    const Compiled_Expr compiled_rhs = compile_bang_expr_into_basm(bang, basm, binary_op.rhs);

    if (compiled_lhs.type != compiled_rhs.type) {
        bang_diag_msg(binary_op.loc, BANG_DIAG_ERROR,
                      "LHS of `%s` has type `%s` but RHS has type `%s`",
                      bang_token_kind_name(bang_binary_op_def(binary_op.kind).token_kind),
                      bang_type_def(compiled_lhs.type).name,
                      bang_type_def(compiled_rhs.type).name);
        exit(1);
    }
    const Bang_Type type = compiled_lhs.type;

    Binary_Op_Of_Type boot = binary_op_of_type(type, binary_op.kind);
    if (!boot.exists) {
        bang_diag_msg(binary_op.loc, BANG_DIAG_ERROR,
                      "binary operation `%s` does not exist for type `%s`",
                      bang_token_kind_name(bang_binary_op_def(binary_op.kind).token_kind),
                      bang_type_def(type).name);
        exit(1);
    }

    basm_push_inst(basm, boot.inst, word_u64(0));

    return boot.ret;
}

static Bang_Type reinterpret_expr_as_type(Bang_Expr type_arg)
{
    if (type_arg.kind != BANG_EXPR_KIND_VAR_READ) {
        bang_diag_msg(type_arg.loc, BANG_DIAG_ERROR,
                      "expected type name");
        exit(1);
    }

    Bang_Type type = 0;
    if (!bang_type_by_name(type_arg.as.var_read.name, &type)) {
        bang_diag_msg(type_arg.loc, BANG_DIAG_ERROR,
                      "`"SV_Fmt"` is not a valid type",
                      SV_Arg(type_arg.as.var_read.name));
        exit(1);
    }

    return type;
}

static void type_check_expr(Compiled_Expr expr, Bang_Type expected_type)
{
    if (expr.type != expected_type) {
        bang_diag_msg(expr.ast.loc, BANG_DIAG_ERROR,
                      "expected type `%s` but `%s`",
                      bang_type_def(expected_type).name,
                      bang_type_def(expr.type).name);
        exit(1);
    }
}

void compile_bang_funcall_into_basm(Bang *bang, Basm *basm, Bang_Funcall funcall)
{
    Compiled_Proc *proc = bang_get_compiled_proc_by_name(bang, funcall.name);
    if (proc == NULL) {
        bang_diag_msg(funcall.loc, BANG_DIAG_ERROR,
                      "unknown function `"SV_Fmt"`",
                      SV_Arg(funcall.name));
        exit(1);
    }

    // Check arity
    {
        size_t params_arity = proc->params.size;
        size_t args_arity = funcall.args.size;

        if (params_arity != args_arity) {
            bang_diag_msg(funcall.loc, BANG_DIAG_ERROR,
                          "Function `"SV_Fmt"` expects %zu amount of arguments but got %zu\n",
                          SV_Arg(funcall.name),
                          params_arity,
                          args_arity);
            exit(1);
        }
    }

    // Compile Funcall args
    {
        size_t n = proc->params.size;

        for (size_t i = 0; i < n; ++i) {
            Bang_Proc_Param param = proc->params.items[i];
            Bang_Funcall_Arg arg = funcall.args.items[i];

            Compiled_Expr expr = compile_bang_expr_into_basm(bang, basm, arg.value);
            Bang_Type param_type = 0;
            if (!bang_type_by_name(param.type_name, &param_type)) {
                bang_diag_msg(param.loc, BANG_DIAG_ERROR,
                              "`"SV_Fmt"` is not a valid type",
                              SV_Arg(param.type_name));
                exit(1);
            }

            type_check_expr(expr, param_type);
        }
    }

    compile_push_new_frame(bang, basm);
    basm_push_inst(basm, INST_CALL, word_u64(proc->addr));
    compile_pop_frame(bang, basm);
}

Compiled_Expr compile_bang_expr_into_basm(Bang *bang, Basm *basm, Bang_Expr expr)
{
    Compiled_Expr result = {0};
    result.addr = basm->program_size;
    result.ast = expr;

    switch (expr.kind) {
    case BANG_EXPR_KIND_LIT_STR: {
        Word str_addr = basm_push_string_to_memory(basm, expr.as.lit_str);
        basm_push_inst(basm, INST_PUSH, str_addr);
        basm_push_inst(basm, INST_PUSH, word_u64(expr.as.lit_str.count));
        // TODO(#420): strings don't have a separate type in Bang
        result.type = BANG_TYPE_I64;
    }
    break;

    case BANG_EXPR_KIND_FUNCALL: {
        Bang_Funcall funcall = expr.as.funcall;

        if (sv_eq(funcall.name, SV("write"))) {
            bang_funcall_expect_arity(funcall, 1);
            compile_bang_expr_into_basm(bang, basm, funcall.args.items[0].value);
            basm_push_inst(basm, INST_NATIVE, word_u64(bang->write_id));
            result.type = BANG_TYPE_VOID;
        } else if (sv_eq(funcall.name, SV("ptr"))) {
            bang_funcall_expect_arity(funcall, 1);
            Bang_Expr arg = funcall.args.items[0].value;
            if (arg.kind != BANG_EXPR_KIND_VAR_READ) {
                bang_diag_msg(funcall.loc, BANG_DIAG_ERROR,
                              "expected variable name as the argument of `ptr` function");
                exit(1);
            }

            Compiled_Var *var = bang_get_compiled_var_by_name(bang, arg.as.var_read.name);
            compile_get_var_addr(bang, basm, var);
            result.type = BANG_TYPE_PTR;
        } else if (sv_eq(funcall.name, SV("write_ptr"))) {
            bang_funcall_expect_arity(funcall, 2);
            Bang_Expr arg0 = funcall.args.items[0].value;
            Bang_Expr arg1 = funcall.args.items[1].value;

            Compiled_Expr buffer = compile_bang_expr_into_basm(bang, basm, arg0);
            type_check_expr(buffer, BANG_TYPE_PTR);

            Compiled_Expr size = compile_bang_expr_into_basm(bang, basm, arg1);
            type_check_expr(size, BANG_TYPE_I64);

            basm_push_inst(basm, INST_NATIVE, word_u64(0));

            result.type = BANG_TYPE_VOID;
        } else if (sv_eq(funcall.name, SV("cast"))) {
            bang_funcall_expect_arity(funcall, 2);
            Bang_Expr arg0 = funcall.args.items[0].value;
            Bang_Expr arg1 = funcall.args.items[1].value;

            Bang_Type type = reinterpret_expr_as_type(arg0);

            Compiled_Expr value = compile_bang_expr_into_basm(bang, basm, arg1);

            if (value.type == BANG_TYPE_I64 && type == BANG_TYPE_PTR) {
                result.type = type;
            } else if (value.type == BANG_TYPE_I64 && type == BANG_TYPE_U8) {
                result.type = type;
            } else if (value.type == BANG_TYPE_U8 && type == BANG_TYPE_PTR) {
                result.type = type;
            } else {
                bang_diag_msg(funcall.loc, BANG_DIAG_ERROR,
                              "can't convert value of type `%s` to `%s`",
                              bang_type_def(value.type).name,
                              bang_type_def(type).name);
                exit(1);
            }

            // TODO(#432): there is no special syntax for dereferencing the pointer
        } else if (sv_eq(funcall.name, SV("store_ptr"))) {
            bang_funcall_expect_arity(funcall, 3);
            Bang_Expr arg0 = funcall.args.items[0].value;
            Bang_Expr arg1 = funcall.args.items[1].value;
            Bang_Expr arg2 = funcall.args.items[2].value;

            Bang_Type type = reinterpret_expr_as_type(arg0);
            if (type == BANG_TYPE_VOID) {
                bang_diag_msg(funcall.loc, BANG_DIAG_ERROR,
                              "cannot store `%s` types",
                              bang_type_def(BANG_TYPE_VOID).name);
                exit(1);
            }

            Compiled_Expr ptr = compile_bang_expr_into_basm(bang, basm, arg1);
            type_check_expr(ptr, BANG_TYPE_PTR);

            Compiled_Expr value = compile_bang_expr_into_basm(bang, basm, arg2);
            type_check_expr(value, type);

            compile_typed_write(basm, type);

            result.type = BANG_TYPE_VOID;
        } else if (sv_eq(funcall.name, SV("load_ptr"))) {
            bang_funcall_expect_arity(funcall, 2);
            Bang_Expr arg0 = funcall.args.items[0].value;
            Bang_Expr arg1 = funcall.args.items[1].value;

            Bang_Type type = reinterpret_expr_as_type(arg0);
            if (type == BANG_TYPE_VOID) {
                bang_diag_msg(funcall.loc, BANG_DIAG_ERROR,
                              "cannot load `%s` types",
                              bang_type_def(BANG_TYPE_VOID).name);
                exit(1);
            }

            Compiled_Expr ptr = compile_bang_expr_into_basm(bang, basm, arg1);
            type_check_expr(ptr, BANG_TYPE_PTR);

            compile_typed_read(basm, type);

            result.type = type;
        } else if (sv_eq(funcall.name, SV("halt"))) {
            bang_funcall_expect_arity(funcall, 0);
            basm_push_inst(basm, INST_HALT, word_u64(0));
            result.type = BANG_TYPE_VOID;
        } else {
            compile_bang_funcall_into_basm(bang, basm, funcall);
            result.type = BANG_TYPE_VOID;
        }
    }
    break;

    case BANG_EXPR_KIND_LIT_BOOL: {
        if (expr.as.boolean) {
            basm_push_inst(basm, INST_PUSH, word_u64(1));
        } else {
            basm_push_inst(basm, INST_PUSH, word_u64(0));
        }
        result.type = BANG_TYPE_BOOL;
    }
    break;

    case BANG_EXPR_KIND_LIT_INT: {
        basm_push_inst(basm, INST_PUSH, word_i64(expr.as.lit_int));
        result.type = BANG_TYPE_I64;
    }
    break;

    case BANG_EXPR_KIND_VAR_READ: {
        result.type = compile_var_read_into_basm(bang, basm, expr.as.var_read);
    }
    break;

    case BANG_EXPR_KIND_BINARY_OP: {
        result.type = compile_binary_op_into_basm(bang, basm, *expr.as.binary_op);
    }
    break;

    case COUNT_BANG_EXPR_KINDS:
    default:
        assert(false && "compile_bang_expr_into_basm: unreachable");
        exit(1);
    }

    return result;
}

void compile_bang_if_into_basm(Bang *bang, Basm *basm, Bang_If eef)
{
    const Compiled_Expr cond_expr = compile_bang_expr_into_basm(bang, basm, eef.condition);
    if (cond_expr.type != BANG_TYPE_BOOL) {
        bang_diag_msg(eef.loc, BANG_DIAG_ERROR,
                      "expected type `%s` but got `%s`",
                      bang_type_def(BANG_TYPE_BOOL).name,
                      bang_type_def(cond_expr.type).name);
        exit(1);
    }

    basm_push_inst(basm, INST_NOT, word_u64(0));

    Inst_Addr then_jmp_addr = basm_push_inst(basm, INST_JMP_IF, word_u64(0));
    compile_block_into_basm(bang, basm, eef.then);
    Inst_Addr else_jmp_addr = basm_push_inst(basm, INST_JMP, word_u64(0));
    Inst_Addr else_addr = basm->program_size;
    compile_block_into_basm(bang, basm, eef.elze);
    Inst_Addr end_addr = basm->program_size;

    basm->program[then_jmp_addr].operand = word_u64(else_addr);
    basm->program[else_jmp_addr].operand = word_u64(end_addr);
}

void bang_scope_push_var(Bang_Scope *scope, Compiled_Var var)
{
    assert(scope);
    assert(scope->vars_count < BANG_SCOPE_VARS_CAPACITY);
    scope->vars[scope->vars_count++] = var;
}

Compiled_Var *bang_scope_get_compiled_var_by_name(Bang_Scope *scope, String_View name)
{
    assert(scope);

    for (size_t i = 0; i < scope->vars_count; ++i) {
        if (sv_eq(scope->vars[i].name, name)) {
            return &scope->vars[i];
        }
    }

    return NULL;
}

Compiled_Var *bang_get_compiled_var_by_name(Bang *bang, String_View name)
{
    for (Bang_Scope *scope = bang->scope;
            scope != NULL;
            scope = scope->parent) {
        Compiled_Var *var = bang_scope_get_compiled_var_by_name(scope, name);
        if (var != NULL) {
            return var;
        }
    }
    return NULL;
}

void compile_read_frame_addr(Bang *bang, Basm *basm)
{
    basm_push_inst(basm, INST_PUSH, word_u64(bang->stack_frame_var_addr));
    basm_push_inst(basm, INST_READ64U, word_u64(0));
}

void compile_write_frame_addr(Bang *bang, Basm *basm)
{
    basm_push_inst(basm, INST_PUSH, word_u64(bang->stack_frame_var_addr));
    basm_push_inst(basm, INST_SWAP, word_u64(1));
    basm_push_inst(basm, INST_WRITE64, word_u64(0));
}

void compile_bang_var_assign_into_basm(Bang *bang, Basm *basm, Bang_Var_Assign var_assign)
{
    Compiled_Var *var = bang_get_compiled_var_by_name(bang, var_assign.name);
    if (var == NULL) {
        bang_diag_msg(var_assign.loc, BANG_DIAG_ERROR,
                      "cannot assign non-existing variable `"SV_Fmt"`",
                      SV_Arg(var_assign.name));
        exit(1);
    }

    compile_get_var_addr(bang, basm, var);

    Compiled_Expr expr = compile_bang_expr_into_basm(bang, basm, var_assign.value);
    if (expr.type != var->type) {
        bang_diag_msg(var_assign.loc, BANG_DIAG_ERROR,
                      "cannot assign expression of type `%s` to a variable of type `%s`",
                      bang_type_def(expr.type).name,
                      bang_type_def(var->type).name);
        exit(1);
    }

    compile_typed_write(basm, expr.type);
}

void compile_bang_while_into_basm(Bang *bang, Basm *basm, Bang_While hwile)
{
    const Compiled_Expr cond_expr = compile_bang_expr_into_basm(bang, basm, hwile.condition);
    if (cond_expr.type != BANG_TYPE_BOOL) {
        bang_diag_msg(hwile.loc, BANG_DIAG_ERROR,
                      "expected type `%s` but got `%s`",
                      bang_type_def(BANG_TYPE_BOOL).name,
                      bang_type_def(cond_expr.type).name);
        exit(1);
    }

    basm_push_inst(basm, INST_NOT, word_u64(0));
    Inst_Addr fallthrough_addr = basm_push_inst(basm, INST_JMP_IF, word_u64(0));
    compile_block_into_basm(bang, basm, hwile.body);
    basm_push_inst(basm, INST_JMP, word_u64(cond_expr.addr));

    const Inst_Addr body_end_addr = basm->program_size;
    basm->program[fallthrough_addr].operand = word_u64(body_end_addr);
}


void compile_stmt_into_basm(Bang *bang, Basm *basm, Bang_Stmt stmt)
{
    switch (stmt.kind) {
    case BANG_STMT_KIND_EXPR: {
        const Compiled_Expr expr = compile_bang_expr_into_basm(bang, basm, stmt.as.expr);
        if (expr.type != BANG_TYPE_VOID) {
            basm_push_inst(basm, INST_DROP, word_u64(0));
        }
    }
    break;

    case BANG_STMT_KIND_IF:
        compile_bang_if_into_basm(bang, basm, stmt.as.eef);
        break;

    case BANG_STMT_KIND_VAR_ASSIGN:
        compile_bang_var_assign_into_basm(bang, basm, stmt.as.var_assign);
        break;

    case BANG_STMT_KIND_WHILE:
        compile_bang_while_into_basm(bang, basm, stmt.as.hwile);
        break;

    case BANG_STMT_KIND_VAR_DEF:
        compile_var_def_into_basm(bang, basm, stmt.as.var_def, BANG_VAR_STACK_STORAGE);
        break;

    case COUNT_BANG_STMT_KINDS:
    default:
        assert(false && "compile_stmt_into_basm: unreachable");
        exit(1);
    }
}

void compile_block_into_basm(Bang *bang, Basm *basm, Bang_Block *block)
{
    bang_push_new_scope(bang);
    while (block) {
        compile_stmt_into_basm(bang, basm, block->stmt);
        block = block->next;
    }
    bang_pop_scope(bang);
}

Compiled_Proc *bang_get_compiled_proc_by_name(Bang *bang, String_View name)
{
    for (size_t i = 0; i < bang->procs_count; ++i) {
        if (sv_eq(bang->procs[i].name, name)) {
            return &bang->procs[i];
        }
    }
    return NULL;
}

void compile_proc_def_into_basm(Bang *bang, Basm *basm, Bang_Proc_Def proc_def)
{
    Compiled_Proc *existing_proc = bang_get_compiled_proc_by_name(bang, proc_def.name);
    if (existing_proc) {
        bang_diag_msg(proc_def.loc, BANG_DIAG_ERROR,
                      "procedure `"SV_Fmt"` is already defined",
                      SV_Arg(proc_def.name));
        bang_diag_msg(existing_proc->loc, BANG_DIAG_NOTE,
                      "the first definition is located here");
        exit(1);
    }

    Compiled_Proc proc = {0};
    proc.name = proc_def.name;
    proc.loc = proc_def.loc;
    proc.params = proc_def.params;
    proc.addr = basm->program_size;
    assert(bang->procs_count < BANG_PROCS_CAPACITY);
    bang->procs[bang->procs_count++] = proc;

    compile_block_into_basm(bang, basm, proc_def.body);

    basm_push_inst(basm, INST_RET, word_u64(0));
}

void bang_funcall_expect_arity(Bang_Funcall funcall, size_t expected_arity)
{
    const size_t actual_arity = funcall.args.size;
    if (expected_arity != actual_arity) {
        bang_diag_msg(funcall.loc, BANG_DIAG_ERROR,
                      "function `"SV_Fmt"` expects %zu amount of arguments but provided %zu",
                      SV_Arg(funcall.name),
                      expected_arity,
                      actual_arity);
        exit(1);
    }
}

void compile_bang_module_into_basm(Bang *bang, Basm *basm, Bang_Module module)
{
    for (Bang_Top *top = module.tops_begin; top != NULL; top = top->next) {
        switch (top->kind) {
        case BANG_TOP_KIND_PROC: {
            compile_proc_def_into_basm(bang, basm, top->as.proc);
        }
        break;
        case BANG_TOP_KIND_VAR: {
            compile_var_def_into_basm(bang, basm, top->as.var, BANG_VAR_STATIC_STORAGE);
        }
        break;
        case COUNT_BANG_TOP_KINDS:
        default:
            assert(false && "compile_bang_module_into_basm: unreachable");
            exit(1);
        }
    }
}

void bang_generate_entry_point(Bang *bang, Basm *basm, String_View entry_proc_name)
{
    Compiled_Proc *entry_proc = bang_get_compiled_proc_by_name(bang, entry_proc_name);
    if (entry_proc == NULL) {
        fprintf(stderr, "ERROR: could not find the entry point procedure `"SV_Fmt"`. Please make sure it is defined.\n", SV_Arg(entry_proc_name));
        exit(1);
    }

    basm->entry = basm->program_size;
    basm->has_entry = true;

    basm_push_inst(basm, INST_CALL, word_u64(entry_proc->addr));
    basm_push_inst(basm, INST_HALT, word_u64(0));
}

void bang_generate_heap_base(Bang *bang, Basm *basm, String_View heap_base_var_name)
{
    assert(bang->scope != NULL);
    Compiled_Var *heap_base_var = bang_get_compiled_var_by_name(bang, heap_base_var_name);

    if (heap_base_var != NULL) {
        assert(heap_base_var->storage == BANG_VAR_STATIC_STORAGE);
        if (heap_base_var->type != BANG_TYPE_PTR) {
            bang_diag_msg(heap_base_var->loc, BANG_DIAG_ERROR,
                          "the special `"SV_Fmt"` variable is expected to be of type `%s` but it was defined as type `%s`",
                          SV_Arg(heap_base_var_name),
                          bang_type_def(BANG_TYPE_PTR).name,
                          bang_type_def(heap_base_var->type).name);
            exit(1);
        }

        const Memory_Addr heap_base_addr = basm->memory_size;
        memcpy(&basm->memory[heap_base_var->addr],
               &heap_base_addr,
               sizeof(heap_base_addr));
    }
}

void bang_prepare_var_stack(Bang *bang, Basm *basm, size_t stack_size)
{
    basm_push_byte_array_to_memory(basm, stack_size, 0);
    const Memory_Addr stack_start_addr = stack_size;

    Bang_Type_Def ptr_def = bang_type_def(BANG_TYPE_PTR);
    assert(sizeof(stack_start_addr) == ptr_def.size);

    bang->stack_frame_var_addr = basm_push_buffer_to_memory(basm, (uint8_t*) &stack_start_addr, ptr_def.size).as_u64;
}

void compile_var_def_into_basm(Bang *bang, Basm *basm, Bang_Var_Def var_def, Bang_Var_Storage storage)
{
    Bang_Type type = 0;
    if (!bang_type_by_name(var_def.type_name, &type)) {
        bang_diag_msg(var_def.loc, BANG_DIAG_ERROR,
                      "type `"SV_Fmt"` does not exist",
                      SV_Arg(var_def.type_name));
        exit(1);
    }

    if (type == BANG_TYPE_VOID) {
        bang_diag_msg(var_def.loc, BANG_DIAG_ERROR,
                      "defining variables with type %s is not allowed",
                      bang_type_def(type).name);
        exit(1);
    }

    // Existing Var Error
    {
        Compiled_Var *existing_var = bang_scope_get_compiled_var_by_name(bang->scope, var_def.name);
        if (existing_var) {
            bang_diag_msg(var_def.loc, BANG_DIAG_ERROR,
                          "variable `"SV_Fmt"` is already defined",
                          SV_Arg(var_def.name));
            bang_diag_msg(existing_var->loc, BANG_DIAG_ERROR,
                          "the first definition is located here");
            exit(1);
        }
    }

    // TODO(#480): bang does not warn about unused variables

    // Shadowed Var Warning
    {
        Compiled_Var *shadowed_var = bang_get_compiled_var_by_name(bang, var_def.name);
        if (shadowed_var) {
            bang_diag_msg(var_def.loc, bang->warnings_as_errors ? BANG_DIAG_ERROR : BANG_DIAG_WARNING,
                          "variable `"SV_Fmt"` is shadowing another variable with the same name",
                          SV_Arg(var_def.name));
            bang_diag_msg(shadowed_var->loc, BANG_DIAG_NOTE,
                          "the shadowed variable is located here");

            if (bang->warnings_as_errors) {
                exit(1);
            }
        }
    }

    Compiled_Var new_var = {0};
    new_var.name = var_def.name;
    new_var.loc = var_def.loc;
    new_var.type = type;
    new_var.storage = storage;

    Bang_Type_Def type_def = bang_type_def(type);

    switch (storage) {
    case BANG_VAR_STATIC_STORAGE: {
        new_var.addr = basm_push_byte_array_to_memory(basm, type_def.size, 0).as_u64;

        // TODO(#476): global variables cannot be initialized at the moment
        if (var_def.has_init) {
            bang_diag_msg(var_def.loc, BANG_DIAG_ERROR,
                          "global variables cannot be initialized at the moment");
            exit(1);
        }
    }
    break;

    case BANG_VAR_STACK_STORAGE: {
        assert(type_def.size > 0);
        bang->frame_size += type_def.size;
        new_var.addr = bang->frame_size;

        if (var_def.has_init) {
            compile_get_var_addr(bang, basm, &new_var);

            Compiled_Expr expr = compile_bang_expr_into_basm(bang, basm, var_def.init);
            if (expr.type != new_var.type) {
                bang_diag_msg(var_def.loc, BANG_DIAG_ERROR,
                              "cannot assign expression of type `%s` to a variable of type `%s`",
                              bang_type_def(expr.type).name,
                              bang_type_def(new_var.type).name);
                exit(1);
            }

            compile_typed_write(basm, expr.type);
        }
    }
    break;

    default:
        assert(false && "unreachable");
        exit(1);
    }

    bang_scope_push_var(bang->scope, new_var);
}

void compile_push_new_frame(Bang *bang, Basm *basm)
{
    // 1. read frame addr
    compile_read_frame_addr(bang, basm);

    // 2. offset the frame addr to find the top of the stack
    assert(bang->scope != NULL);
    basm_push_inst(basm, INST_PUSH, word_u64(bang->frame_size));
    basm_push_inst(basm, INST_MINUSI, word_u64(0));

    // 3. allocate memory to store the prev frame addr
    Bang_Type_Def ptr_def = bang_type_def(BANG_TYPE_PTR);
    basm_push_inst(basm, INST_PUSH, word_u64(ptr_def.size));
    basm_push_inst(basm, INST_MINUSI, word_u64(0));
    basm_push_inst(basm, INST_DUP, word_u64(0));
    compile_read_frame_addr(bang, basm);
    basm_push_inst(basm, INST_WRITE64, word_u64(0));

    // 4. redirect the current frame
    compile_write_frame_addr(bang, basm);
}

void compile_pop_frame(Bang *bang, Basm *basm)
{
    // 1. read frame addr
    compile_read_frame_addr(bang, basm);

    // 2. read prev frame addr
    basm_push_inst(basm, INST_READ64U, word_u64(0));

    // 3. write the prev frame back to the current frame
    compile_write_frame_addr(bang, basm);
}

void bang_push_new_scope(Bang *bang)
{
    Bang_Scope *scope = arena_alloc(&bang->arena, sizeof(Bang_Scope));
    scope->parent = bang->scope;
    bang->scope = scope;
}

void bang_pop_scope(Bang *bang)
{
    assert(bang->scope != NULL);
    Bang_Scope *scope = bang->scope;

    size_t dealloc_size = 0;
    for (size_t i = 0; i < scope->vars_count; ++i) {
        if (scope->vars[i].storage == BANG_VAR_STACK_STORAGE) {
            Bang_Type_Def def = bang_type_def(scope->vars[i].type);
            dealloc_size += def.size;
        }
    }
    bang->frame_size -= dealloc_size;

    bang->scope = bang->scope->parent;
}
