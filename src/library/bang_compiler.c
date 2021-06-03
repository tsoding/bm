#include "./bang_compiler.h"

static const char *const bang_type_names[COUNT_BANG_TYPES] = {
    [BANG_TYPE_I64] = "i64",
    [BANG_TYPE_VOID] = "void",
};
static_assert(
    COUNT_BANG_TYPES == 2,
    "The amount of types have changed. "
    "Please update the type name table accordingly. "
    "Thanks!");

const char *bang_type_name(Bang_Type type)
{
    assert(type >= 0);
    assert(type < COUNT_BANG_TYPES);
    return bang_type_names[type];
}

Bang_Type compile_var_read_into_basm(Bang *bang, Basm *basm, Bang_Var_Read var_read)
{
    Bang_Global_Var *var = bang_get_global_var_by_name(bang, var_read.name);
    if (var == NULL) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: could not read non-existing variable `"SV_Fmt"`\n",
                Bang_Loc_Arg(var_read.loc),
                SV_Arg(var_read.name));
        exit(1);
    }

    basm_push_inst(basm, INST_PUSH, word_u64(var->addr));
    basm_push_inst(basm, INST_READ64I, word_u64(0));

    return var->type;
}

Bang_Type compile_binary_op_into_basm(Bang *bang, Basm *basm, Bang_Binary_Op binary_op)
{
    assert(binary_op.lhs != NULL);
    const Compiled_Expr compiled_lhs = compile_bang_expr_into_basm(bang, basm, *binary_op.lhs);
    if (compiled_lhs.type != BANG_TYPE_I64) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: `%s` is only supported for type `%s`\n",
                Bang_Loc_Arg(binary_op.loc),
                bang_token_kind_name(bang_binary_op_token(binary_op.kind)),
                bang_type_name(BANG_TYPE_I64));
        exit(1);
    }

    assert(binary_op.rhs != NULL);
    const Compiled_Expr compiled_rhs = compile_bang_expr_into_basm(bang, basm, *binary_op.rhs);
    if (compiled_rhs.type != BANG_TYPE_I64) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: `%s` is only supported for type `%s`\n",
                Bang_Loc_Arg(binary_op.loc),
                bang_token_kind_name(bang_binary_op_token(binary_op.kind)),
                bang_type_name(BANG_TYPE_I64));
        exit(1);
    }

    switch (binary_op.kind) {
    case BANG_BINARY_OP_KIND_PLUS: {
        basm_push_inst(basm, INST_PLUSI, word_u64(0));
    }
    break;

    case BANG_BINARY_OP_KIND_LESS: {
        basm_push_inst(basm, INST_LTI, word_u64(0));
    }
    break;

    case COUNT_BANG_BINARY_OP_KINDS:
    default: {
        assert(false && "compile_binary_op_into_basm: unreachable");
        exit(1);
    }
    }

    return BANG_TYPE_I64;
}

Compiled_Expr compile_bang_expr_into_basm(Bang *bang, Basm *basm, Bang_Expr expr)
{
    Compiled_Expr result = {0};
    result.addr = basm->program_size;

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
        if (sv_eq(expr.as.funcall.name, SV("write"))) {
            bang_funcall_expect_arity(expr.as.funcall, 1);
            compile_bang_expr_into_basm(bang, basm, expr.as.funcall.args->value);
            basm_push_inst(basm, INST_NATIVE, word_u64(bang->write_id));
            result.type = BANG_TYPE_VOID;
        } else {
            Compiled_Proc *proc = bang_get_compiled_proc_by_name(bang, expr.as.funcall.name);
            if (proc == NULL) {
                fprintf(stderr, Bang_Loc_Fmt": ERROR: unknown function `"SV_Fmt"`\n",
                        Bang_Loc_Arg(expr.as.funcall.loc),
                        SV_Arg(expr.as.funcall.name));
                exit(1);
            }
            basm_push_inst(basm, INST_CALL, word_u64(proc->addr));
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
        // TODO(#421): booleans don't have a separate type in Bang
        result.type = BANG_TYPE_I64;
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
        result.type = compile_binary_op_into_basm(bang, basm, expr.as.binary_op);
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
    if (cond_expr.type == BANG_TYPE_VOID) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: can't use `%s` expression as the condition of if-else\n",
                Bang_Loc_Arg(eef.loc),
                bang_type_name(cond_expr.type));
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

Bang_Global_Var *bang_get_global_var_by_name(Bang *bang, String_View name)
{
    for (size_t i = 0; i < bang->global_vars_count; ++i) {
        if (sv_eq(bang->global_vars[i].name, name)) {
            return &bang->global_vars[i];
        }
    }
    return NULL;
}

void compile_bang_var_assign_into_basm(Bang *bang, Basm *basm, Bang_Var_Assign var_assign)
{
    Bang_Global_Var *var =  bang_get_global_var_by_name(bang, var_assign.name);
    if (var == NULL) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: cannot assign non-existing variable `"SV_Fmt"`\n",
                Bang_Loc_Arg(var_assign.loc),
                SV_Arg(var_assign.name));
        exit(1);
    }

    basm_push_inst(basm, INST_PUSH, word_u64(var->addr));
    compile_bang_expr_into_basm(bang, basm, var_assign.value);
    basm_push_inst(basm, INST_WRITE64, word_u64(0));
}

void compile_bang_while_into_basm(Bang *bang, Basm *basm, Bang_While hwile)
{
    const Compiled_Expr cond_expr = compile_bang_expr_into_basm(bang, basm, hwile.condition);
    if (cond_expr.type == BANG_TYPE_VOID) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: can't use `%s` expression as the condition of the while-loop\n",
                Bang_Loc_Arg(hwile.loc),
                bang_type_name(cond_expr.type));
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

    case COUNT_BANG_STMT_KINDS:
    default:
        assert(false && "compile_stmt_into_basm: unreachable");
        exit(1);
    }
}

void compile_block_into_basm(Bang *bang, Basm *basm, Bang_Block *block)
{
    while (block) {
        compile_stmt_into_basm(bang, basm, block->stmt);
        block = block->next;
    }
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
        fprintf(stderr, Bang_Loc_Fmt": ERROR: procedure `"SV_Fmt"` is already defined\n",
                Bang_Loc_Arg(proc_def.loc),
                SV_Arg(proc_def.name));
        fprintf(stderr, Bang_Loc_Fmt": NOTE: the first definition is located here\n",
                Bang_Loc_Arg(existing_proc->loc));
        exit(1);
    }

    Compiled_Proc proc = {0};
    proc.loc = proc_def.loc;
    proc.name = proc_def.name;
    proc.addr = basm->program_size;
    assert(bang->procs_count < BANG_PROCS_CAPACITY);
    bang->procs[bang->procs_count++] = proc;

    compile_block_into_basm(bang, basm, proc_def.body);
    basm_push_inst(basm, INST_RET, word_u64(0));
}

void bang_funcall_expect_arity(Bang_Funcall funcall, size_t expected_arity)
{
    size_t actual_arity = 0;

    {
        Bang_Funcall_Arg *args = funcall.args;
        while (args != NULL) {
            actual_arity += 1;
            args = args->next;
        }
    }

    if (expected_arity != actual_arity) {
        fprintf(stderr, Bang_Loc_Fmt"ERROR: function `"SV_Fmt"` expectes %zu amoutn of arguments but provided %zu\n",
                Bang_Loc_Arg(funcall.loc),
                SV_Arg(funcall.name),
                expected_arity,
                actual_arity);
        exit(1);
    }
}

static size_t bang_size_of_type(Bang_Type type)
{
    switch (type) {
    case BANG_TYPE_I64:
        return 8;
    case BANG_TYPE_VOID:
        assert(false && "bang_size_of_type: unreachable: type void does not have a size");
        exit(1);
    case COUNT_BANG_TYPES:
    default:
        assert(false && "bang_size_of_type: unreachable");
        exit(1);
    }
}

void compile_var_def_into_basm(Bang *bang, Basm *basm, Bang_Var_Def var_def)
{
    if (var_def.type == BANG_TYPE_VOID) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: defining variables with type %s is not allowed\n",
                Bang_Loc_Arg(var_def.loc),
                bang_type_name(var_def.type));
        exit(1);
    }

    Bang_Global_Var *existing_var = bang_get_global_var_by_name(bang, var_def.name);
    if (existing_var) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: variable `"SV_Fmt"` is already defined\n",
                Bang_Loc_Arg(var_def.loc),
                SV_Arg(var_def.name));
        fprintf(stderr, Bang_Loc_Fmt": NOTE: the first definition is located here\n",
                Bang_Loc_Arg(existing_var->loc));
        exit(1);
    }

    Bang_Global_Var new_var = {0};
    new_var.loc = var_def.loc;
    new_var.addr = basm_push_byte_array_to_memory(basm, bang_size_of_type(var_def.type), 0).as_u64;
    new_var.name = var_def.name;
    new_var.type = var_def.type;

    assert(bang->global_vars_count < BANG_GLOBAL_VARS_CAPACITY);
    bang->global_vars[bang->global_vars_count++] = new_var;
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
            compile_var_def_into_basm(bang, basm, top->as.var);
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
