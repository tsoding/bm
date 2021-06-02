#include "./bang_compiler.h"

void compile_var_read_into_basm(Bang *bang, Basm *basm, Bang_Var_Read var_read)
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
}

void compile_binary_op_into_basm(Bang *bang, Basm *basm, Bang_Binary_Op binary_op)
{
    switch (binary_op.kind) {
    case BANG_BINARY_OP_KIND_PLUS: {
        assert(binary_op.lhs != NULL);
        compile_bang_expr_into_basm(bang, basm, *binary_op.lhs);
        assert(binary_op.rhs != NULL);
        compile_bang_expr_into_basm(bang, basm, *binary_op.rhs);
        basm_push_inst(basm, INST_PLUSI, word_u64(0));
    }
    break;

    case BANG_BINARY_OP_KIND_LESS: {
        assert(binary_op.lhs != NULL);
        compile_bang_expr_into_basm(bang, basm, *binary_op.lhs);
        assert(binary_op.rhs != NULL);
        compile_bang_expr_into_basm(bang, basm, *binary_op.rhs);
        basm_push_inst(basm, INST_LTI, word_u64(0));
    }
    break;

    case COUNT_BANG_BINARY_OP_KINDS:
    default: {
        assert(false && "compile_binary_op_into_basm: unreachable");
        exit(1);
    }
    }
}

Inst_Addr compile_bang_expr_into_basm(Bang *bang, Basm *basm, Bang_Expr expr)
{
    Inst_Addr result = basm->program_size;

    switch (expr.kind) {
    case BANG_EXPR_KIND_LIT_STR: {
        Word str_addr = basm_push_string_to_memory(basm, expr.as.lit_str);
        basm_push_inst(basm, INST_PUSH, str_addr);
        basm_push_inst(basm, INST_PUSH, word_u64(expr.as.lit_str.count));
    }
    break;

    case BANG_EXPR_KIND_FUNCALL: {
        if (sv_eq(expr.as.funcall.name, SV("write"))) {
            bang_funcall_expect_arity(expr.as.funcall, 1);
            compile_bang_expr_into_basm(bang, basm, expr.as.funcall.args->value);
            basm_push_inst(basm, INST_NATIVE, word_u64(bang->write_id));
        } else {
            fprintf(stderr, Bang_Loc_Fmt": ERROR: unknown function `"SV_Fmt"`\n",
                    Bang_Loc_Arg(expr.as.funcall.loc),
                    SV_Arg(expr.as.funcall.name));
            exit(1);
        }
    }
    break;

    case BANG_EXPR_KIND_LIT_BOOL: {
        if (expr.as.boolean) {
            basm_push_inst(basm, INST_PUSH, word_u64(1));
        } else {
            basm_push_inst(basm, INST_PUSH, word_u64(0));
        }
    }
    break;

    case BANG_EXPR_KIND_LIT_INT: {
        basm_push_inst(basm, INST_PUSH, word_i64(expr.as.lit_int));
    }
    break;

    case BANG_EXPR_KIND_VAR_READ: {
        compile_var_read_into_basm(bang, basm, expr.as.var_read);
    }
    break;

    case BANG_EXPR_KIND_BINARY_OP: {
        compile_binary_op_into_basm(bang, basm, expr.as.binary_op);
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
    compile_bang_expr_into_basm(bang, basm, eef.condition);
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
    const Inst_Addr cond_addr = compile_bang_expr_into_basm(bang, basm, hwile.condition);
    basm_push_inst(basm, INST_NOT, word_u64(0));
    Inst_Addr fallthrough_addr = basm_push_inst(basm, INST_JMP_IF, word_u64(0));
    compile_block_into_basm(bang, basm, hwile.body);
    basm_push_inst(basm, INST_JMP, word_u64(cond_addr));

    const Inst_Addr body_end_addr = basm->program_size;
    basm->program[fallthrough_addr].operand = word_u64(body_end_addr);
}

void compile_stmt_into_basm(Bang *bang, Basm *basm, Bang_Stmt stmt)
{
    switch (stmt.kind) {
    case BANG_STMT_KIND_EXPR:
        compile_bang_expr_into_basm(bang, basm, stmt.as.expr);
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

Inst_Addr compile_proc_def_into_basm(Bang *bang, Basm *basm, Bang_Proc_Def proc_def)
{
    assert(!basm->has_entry);
    Inst_Addr addr = basm->program_size;
    compile_block_into_basm(bang, basm, proc_def.body);
    return addr;
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
    default:
        assert(false && "bang_size_of_type: unreachable");
        exit(1);
    }
}

void compile_var_def_into_basm(Bang *bang, Basm *basm, Bang_Var_Def var_def)
{
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

    assert(bang->global_vars_count < BANG_GLOBAL_VARS_CAPACITY);
    bang->global_vars[bang->global_vars_count++] = new_var;
}

void compile_bang_module_into_basm(Bang *bang, Basm *basm, Bang_Module module)
{
    for (Bang_Top *top = module.tops_begin; top != NULL; top = top->next) {
        switch (top->kind) {
        case BANG_TOP_KIND_PROC: {
            if (sv_eq(top->as.proc.name, SV("main"))) {
                if (!basm->has_entry) {
                    const Inst_Addr proc_addr = compile_proc_def_into_basm(bang, basm, top->as.proc);
                    basm->entry = proc_addr;
                    basm->has_entry = true;
                    bang->entry_loc = top->as.proc.loc;
                } else {
                    fprintf(stderr, Bang_Loc_Fmt": ERROR: redefinition of the entry point\n",
                            Bang_Loc_Arg(top->as.proc.loc));
                    fprintf(stderr, Bang_Loc_Fmt": NOTE: the entry point is already defined here\n",
                            Bang_Loc_Arg(bang->entry_loc));
                    exit(1);
                }
            } else {
                // TODO(#415): Bang does not support additional procedures
                fprintf(stderr, Bang_Loc_Fmt": ERROR: Bang does not support additional procedures. Only `main`. We are working on adding more procedures. Stay tuned!\n",
                        Bang_Loc_Arg(top->as.proc.loc));
                exit(1);
            }
        }
        break;
        case BANG_TOP_KIND_VAR:
            compile_var_def_into_basm(bang, basm, top->as.var);
            break;
        case COUNT_BANG_TOP_KINDS:
        default:
            assert(false && "compile_bang_module_into_basm: unreachable");
            exit(1);
        }
    }
}
