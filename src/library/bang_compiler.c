#include "./bang_compiler.h"

void compile_bang_expr_into_basm(Basm *basm, Bang_Expr expr, Native_ID write_id)
{
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
            compile_bang_expr_into_basm(basm, expr.as.funcall.args->value, write_id);
            basm_push_inst(basm, INST_NATIVE, word_u64(write_id));
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
    } break;

    default:
        assert(false && "compile_bang_expr_into_basm: unreachable");
        exit(1);
    }
}

void compile_bang_if_into_basm(Basm *basm, Bang_If eef, Native_ID write_id)
{
    compile_bang_expr_into_basm(basm, eef.condition, write_id);
    basm_push_inst(basm, INST_NOT, word_u64(0));

    Inst_Addr then_jmp_addr = basm_push_inst(basm, INST_JMP_IF, word_u64(0));
    compile_block_into_basm(basm, eef.then, write_id);
    Inst_Addr else_jmp_addr = basm_push_inst(basm, INST_JMP, word_u64(0));
    Inst_Addr else_addr = basm->program_size;
    compile_block_into_basm(basm, eef.elze, write_id);
    Inst_Addr end_addr = basm->program_size;

    basm->program[then_jmp_addr].operand = word_u64(else_addr);
    basm->program[else_jmp_addr].operand = word_u64(end_addr);
}

void compile_stmt_into_basm(Basm *basm, Bang_Stmt stmt, Native_ID write_id)
{
    switch (stmt.kind) {
    case BANG_STMT_KIND_EXPR:
        compile_bang_expr_into_basm(basm, stmt.as.expr, write_id);
        break;

    case BANG_STMT_KIND_IF:
        compile_bang_if_into_basm(basm, stmt.as.eef, write_id);
        break;

    default:
        assert(false && "compile_stmt_into_basm: unreachable");
        exit(1);
    }
}

void compile_block_into_basm(Basm *basm, Bang_Block *block, Native_ID write_id)
{
    while (block) {
        compile_stmt_into_basm(basm, block->stmt, write_id);
        block = block->next;
    }
}

void compile_proc_def_into_basm(Basm *basm, Bang_Proc_Def proc_def, Native_ID write_id)
{
    assert(!basm->has_entry);
    basm->entry = basm->program_size;
    compile_block_into_basm(basm, proc_def.body, write_id);
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
