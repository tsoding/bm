#include "./bang_compiler.h"

void compile_bang_expr_into_basm(Basm *basm, Bang_Expr expr, Native_ID write_id)
{
    switch (expr.kind) {
    case BANG_EXPR_KIND_LIT_STR: {
        Word str_addr = basm_push_string_to_memory(basm, expr.value.as_lit_str);
        basm_push_inst(basm, INST_PUSH, str_addr);
        basm_push_inst(basm, INST_PUSH, word_u64(expr.value.as_lit_str.count));
    }
    break;

    case BANG_EXPR_KIND_FUNCALL: {
        if (sv_eq(expr.value.as_funcall.name, SV("write"))) {
            bang_funcall_expect_arity(expr.value.as_funcall, 1);
            compile_bang_expr_into_basm(basm, expr.value.as_funcall.args->value, write_id);
            basm_push_inst(basm, INST_NATIVE, word_u64(write_id));
        } else {
            fprintf(stderr, Bang_Loc_Fmt": ERROR: unknown function `"SV_Fmt"`\n",
                    Bang_Loc_Arg(expr.value.as_funcall.loc),
                    SV_Arg(expr.value.as_funcall.name));
            exit(1);
        }
    }
    break;

    default:
        assert(false && "compile_bang_expr_into_basm: unreachable");
        exit(1);
    }
}

void compile_stmt_into_basm(Basm *basm, Bang_Stmt stmt, Native_ID write_id)
{
    compile_bang_expr_into_basm(basm, stmt.expr, write_id);
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
