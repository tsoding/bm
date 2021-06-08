#include "./bang_compiler.h"

// TODO(#426): bang does not support type casting

static const char *const bang_type_names[COUNT_BANG_TYPES] = {
    [BANG_TYPE_VOID] = "void",
    [BANG_TYPE_I64] = "i64",
    [BANG_TYPE_U8] = "u8",
    [BANG_TYPE_BOOL] = "bool",
    [BANG_TYPE_PTR] = "ptr",
};
static_assert(
    COUNT_BANG_TYPES == 5,
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
    Compiled_Var *var = bang_get_global_var_by_name(bang, var_read.name);
    if (var == NULL) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: could not read non-existing variable `"SV_Fmt"`\n",
                Bang_Loc_Arg(var_read.loc),
                SV_Arg(var_read.name));
        exit(1);
    }

    switch (var->def.type) {
    case BANG_TYPE_I64:
    case BANG_TYPE_BOOL:
    case BANG_TYPE_PTR:
        basm_push_inst(basm, INST_PUSH, word_u64(var->addr));
        basm_push_inst(basm, INST_READ64I, word_u64(0));
        break;

    case BANG_TYPE_U8:
        basm_push_inst(basm, INST_PUSH, word_u64(var->addr));
        basm_push_inst(basm, INST_READ8U, word_u64(0));
        break;

    case BANG_TYPE_VOID:
    case COUNT_BANG_TYPES:
    default:
        assert(false && "compile_var_read_into_basm: unreachable");
    }

    return var->def.type;
}

Bang_Type compile_binary_op_into_basm(Bang *bang, Basm *basm, Bang_Binary_Op binary_op)
{
    assert(binary_op.lhs != NULL);
    const Compiled_Expr compiled_lhs = compile_bang_expr_into_basm(bang, basm, *binary_op.lhs);
    if (compiled_lhs.type != BANG_TYPE_I64 && compiled_lhs.type != BANG_TYPE_PTR) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: `%s` is only supported for types `%s` and `%s`\n",
                Bang_Loc_Arg(binary_op.loc),
                bang_token_kind_name(bang_binary_op_token(binary_op.kind)),
                bang_type_name(BANG_TYPE_I64),
                bang_type_name(BANG_TYPE_PTR));
        exit(1);
    }

    assert(binary_op.rhs != NULL);
    const Compiled_Expr compiled_rhs = compile_bang_expr_into_basm(bang, basm, *binary_op.rhs);
    if (compiled_rhs.type != BANG_TYPE_I64 && compiled_rhs.type != BANG_TYPE_PTR) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: `%s` is only supported for types `%s` and `%s`\n",
                Bang_Loc_Arg(binary_op.loc),
                bang_token_kind_name(bang_binary_op_token(binary_op.kind)),
                bang_type_name(BANG_TYPE_I64),
                bang_type_name(BANG_TYPE_PTR));
        exit(1);
    }

    if (compiled_lhs.type != compiled_rhs.type) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: LHS of `%s` has type `%s` but RHS has type `%s`\n",
                Bang_Loc_Arg(binary_op.loc),
                bang_token_kind_name(bang_binary_op_token(binary_op.kind)),
                bang_type_name(compiled_lhs.type),
                bang_type_name(compiled_rhs.type));
        exit(1);
    }

    switch (binary_op.kind) {
    case BANG_BINARY_OP_KIND_PLUS: {
        basm_push_inst(basm, INST_PLUSI, word_u64(0));
        assert(compiled_lhs.type == compiled_rhs.type);
        return compiled_lhs.type;
    }
    break;

    case BANG_BINARY_OP_KIND_LESS: {
        basm_push_inst(basm, INST_LTI, word_u64(0));
        return BANG_TYPE_BOOL;
    }
    break;

    case COUNT_BANG_BINARY_OP_KINDS:
    default: {
        assert(false && "compile_binary_op_into_basm: unreachable");
        exit(1);
    }
    }
}

static Bang_Type reinterpret_expr_as_type(Bang_Expr type_arg)
{
    if (type_arg.kind != BANG_EXPR_KIND_VAR_READ) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: expected type name\n",
                Bang_Loc_Arg(type_arg.loc));
        exit(1);
    }

    Bang_Type type = 0;
    if (!bang_type_by_name(type_arg.as.var_read.name, &type)) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: `"SV_Fmt"` is not a valid type\n",
                Bang_Loc_Arg(type_arg.loc),
                SV_Arg(type_arg.as.var_read.name));
        exit(1);
    }

    return type;
}

static void type_check_expr(Compiled_Expr expr, Bang_Type expected_type)
{
    if (expr.type != expected_type) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: expected type `%s` but `%s`\n",
                Bang_Loc_Arg(expr.ast.loc),
                bang_type_name(expected_type),
                bang_type_name(expr.type));
        exit(1);
    }
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
            compile_bang_expr_into_basm(bang, basm, funcall.args->value);
            basm_push_inst(basm, INST_NATIVE, word_u64(bang->write_id));
            result.type = BANG_TYPE_VOID;
        } else if (sv_eq(funcall.name, SV("ptr"))) {
            bang_funcall_expect_arity(funcall, 1);
            Bang_Expr arg = funcall.args->value;
            if (arg.kind != BANG_EXPR_KIND_VAR_READ) {
                fprintf(stderr, Bang_Loc_Fmt": ERROR: expected variable name as the argument of `ptr` function\n",
                        Bang_Loc_Arg(funcall.loc));
                exit(1);
            }

            Compiled_Var *var = bang_get_global_var_by_name(bang, arg.as.var_read.name);
            basm_push_inst(basm, INST_PUSH, word_u64(var->addr));
            result.type = BANG_TYPE_PTR;
        } else if (sv_eq(funcall.name, SV("write_ptr"))) {
            bang_funcall_expect_arity(funcall, 2);
            Bang_Funcall_Arg *args = funcall.args;
            Bang_Expr arg0 = args->value;
            Bang_Expr arg1 = args->next->value;

            Compiled_Expr buffer = compile_bang_expr_into_basm(bang, basm, arg0);
            type_check_expr(buffer, BANG_TYPE_PTR);

            Compiled_Expr size = compile_bang_expr_into_basm(bang, basm, arg1);
            type_check_expr(size, BANG_TYPE_I64);
            
            basm_push_inst(basm, INST_NATIVE, word_u64(0));

            result.type = BANG_TYPE_VOID;
        } else if (sv_eq(funcall.name, SV("cast"))) {
            bang_funcall_expect_arity(funcall, 2);
            Bang_Funcall_Arg *args = funcall.args;
            Bang_Expr arg0 = args->value;
            Bang_Expr arg1 = args->next->value;

            Bang_Type type = reinterpret_expr_as_type(arg0);

            Compiled_Expr value = compile_bang_expr_into_basm(bang, basm, arg1);

            if (value.type == BANG_TYPE_I64 && type == BANG_TYPE_PTR) {
                result.type = type;
            } else if (value.type == BANG_TYPE_I64 && type == BANG_TYPE_U8) {
                result.type = type;
            } else {
                fprintf(stderr, Bang_Loc_Fmt": ERROR: can't convert value of type `%s` to `%s`\n",
                        Bang_Loc_Arg(funcall.loc),
                        bang_type_name(value.type),
                        bang_type_name(type));
                exit(1);
            }
            
            // TODO: there is no special syntax for dereferencing the pointer
        } else if (sv_eq(funcall.name, SV("store_ptr"))) {
            bang_funcall_expect_arity(funcall, 3);
            Bang_Funcall_Arg *args = funcall.args;
            Bang_Expr arg0 = args->value;
            Bang_Expr arg1 = args->next->value;
            Bang_Expr arg2 = args->next->next->value;

            Bang_Type type = reinterpret_expr_as_type(arg0);

            Compiled_Expr ptr = compile_bang_expr_into_basm(bang, basm, arg1);
            type_check_expr(ptr, BANG_TYPE_PTR);

            Compiled_Expr value = compile_bang_expr_into_basm(bang, basm, arg2);
            type_check_expr(value, type);

            switch (type) {
            case BANG_TYPE_I64:
            case BANG_TYPE_BOOL:
            case BANG_TYPE_PTR:
                basm_push_inst(basm, INST_WRITE64, word_u64(0));
                break;

            case BANG_TYPE_U8:
                basm_push_inst(basm, INST_WRITE8, word_u64(0));
                break;

            case BANG_TYPE_VOID: {
                fprintf(stderr, Bang_Loc_Fmt": ERROR: cannot store `%s` types\n",
                        Bang_Loc_Arg(funcall.loc),
                        bang_type_name(BANG_TYPE_VOID));
                exit(1);
            }
            break;

            case COUNT_BANG_TYPES:
            default:
                assert(false && "compile_bang_expr_into_basm: unreachable");
                exit(1);
            }

            result.type = BANG_TYPE_VOID;
        } else if (sv_eq(funcall.name, SV("load_ptr"))) {
            bang_funcall_expect_arity(funcall, 2);
            Bang_Funcall_Arg *args = funcall.args;
            Bang_Expr arg0 = args->value;
            Bang_Expr arg1 = args->next->value;

            Bang_Type type = reinterpret_expr_as_type(arg0);
            Compiled_Expr ptr = compile_bang_expr_into_basm(bang, basm, arg1);
            type_check_expr(ptr, BANG_TYPE_PTR);

            switch (type) {
            case BANG_TYPE_I64:
                basm_push_inst(basm, INST_READ64I, word_u64(0));
                break;

            case BANG_TYPE_BOOL:
            case BANG_TYPE_PTR:
                basm_push_inst(basm, INST_READ64U, word_u64(0));
                break;

            case BANG_TYPE_U8:
                basm_push_inst(basm, INST_READ8U, word_u64(0));
                break;

            case BANG_TYPE_VOID: {
                fprintf(stderr, Bang_Loc_Fmt": ERROR: cannot load `%s` types\n",
                        Bang_Loc_Arg(funcall.loc),
                        bang_type_name(BANG_TYPE_VOID));
                exit(1);
            }
            break;

            case COUNT_BANG_TYPES:
            default:
                assert(false && "compile_bang_expr_into_basm: unreachable");
                exit(1);
            }

            result.type = type;
        } else {
            Compiled_Proc *proc = bang_get_compiled_proc_by_name(bang, funcall.name);
            if (proc == NULL) {
                fprintf(stderr, Bang_Loc_Fmt": ERROR: unknown function `"SV_Fmt"`\n",
                        Bang_Loc_Arg(funcall.loc),
                        SV_Arg(funcall.name));
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
    if (cond_expr.type != BANG_TYPE_BOOL) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: expected type `%s` but got `%s`\n",
                Bang_Loc_Arg(eef.loc),
                bang_type_name(BANG_TYPE_BOOL),
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

Compiled_Var *bang_get_global_var_by_name(Bang *bang, String_View name)
{
    for (size_t i = 0; i < bang->global_vars_count; ++i) {
        if (sv_eq(bang->global_vars[i].def.name, name)) {
            return &bang->global_vars[i];
        }
    }
    return NULL;
}

void compile_bang_var_assign_into_basm(Bang *bang, Basm *basm, Bang_Var_Assign var_assign)
{
    Compiled_Var *var = bang_get_global_var_by_name(bang, var_assign.name);
    if (var == NULL) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: cannot assign non-existing variable `"SV_Fmt"`\n",
                Bang_Loc_Arg(var_assign.loc),
                SV_Arg(var_assign.name));
        exit(1);
    }

    basm_push_inst(basm, INST_PUSH, word_u64(var->addr));
    Compiled_Expr expr = compile_bang_expr_into_basm(bang, basm, var_assign.value);

    if (expr.type != var->def.type) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: cannot assign expression of type `%s` to a variable of type `%s`\n",
                Bang_Loc_Arg(var_assign.loc),
                bang_type_name(expr.type),
                bang_type_name(var->def.type));
        exit(1);
    }

    switch (expr.type) {
    case BANG_TYPE_I64:
    case BANG_TYPE_BOOL:
    case BANG_TYPE_PTR:
        basm_push_inst(basm, INST_WRITE64, word_u64(0));
        break;

    case BANG_TYPE_U8:
        basm_push_inst(basm, INST_WRITE8, word_u64(0));
        break;

    case BANG_TYPE_VOID:
    case COUNT_BANG_TYPES:
    default: {
        assert(false && "compile_bang_var_assign_into_basm: unreachable");
        exit(1);
    }
    }
}

void compile_bang_while_into_basm(Bang *bang, Basm *basm, Bang_While hwile)
{
    const Compiled_Expr cond_expr = compile_bang_expr_into_basm(bang, basm, hwile.condition);
    if (cond_expr.type != BANG_TYPE_BOOL) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: expected type `%s` but got `%s`\n",
                Bang_Loc_Arg(hwile.loc),
                bang_type_name(BANG_TYPE_BOOL),
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
        if (sv_eq(bang->procs[i].def.name, name)) {
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
                Bang_Loc_Arg(existing_proc->def.loc));
        exit(1);
    }

    Compiled_Proc proc = {0};
    proc.def = proc_def;
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
        fprintf(stderr, Bang_Loc_Fmt"ERROR: function `"SV_Fmt"` expectes %zu amount of arguments but provided %zu\n",
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
    case BANG_TYPE_BOOL:
    case BANG_TYPE_PTR:
        return 8;
    case BANG_TYPE_U8:
        return 1;
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

    Compiled_Var *existing_var = bang_get_global_var_by_name(bang, var_def.name);
    if (existing_var) {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: variable `"SV_Fmt"` is already defined\n",
                Bang_Loc_Arg(var_def.loc),
                SV_Arg(var_def.name));
        fprintf(stderr, Bang_Loc_Fmt": NOTE: the first definition is located here\n",
                Bang_Loc_Arg(existing_var->def.loc));
        exit(1);
    }

    Compiled_Var new_var = {0};
    new_var.def = var_def;
    new_var.addr = basm_push_byte_array_to_memory(basm, bang_size_of_type(var_def.type), 0).as_u64;

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

void bang_generate_heap_base(Bang *bang, Basm *basm, String_View heap_base_var_name)
{
    Compiled_Var *heap_base_var = bang_get_global_var_by_name(bang, heap_base_var_name);

    if (heap_base_var != NULL) {
        if (heap_base_var->def.type != BANG_TYPE_PTR) {
            fprintf(stderr, Bang_Loc_Fmt": ERROR: the special `"SV_Fmt"` variable is expected to be of type `%s` but it was defined as type `%s`\n",
                    Bang_Loc_Arg(heap_base_var->def.loc),
                    SV_Arg(heap_base_var_name),
                    bang_type_name(BANG_TYPE_PTR),
                    bang_type_name(heap_base_var->def.type));
            exit(1);
        }

        const Memory_Addr heap_base_addr = basm->memory_size;
        memcpy(&basm->memory[heap_base_var->addr],
               &heap_base_addr,
               sizeof(heap_base_addr));
    }
}
