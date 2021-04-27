#include "./statement.h"

void dump_block(FILE *stream, Block_Statement *block, int level)
{
    fprintf(stream, "%*sBlock:\n", level * 2, "");
    while (block != NULL) {
        dump_statement(stream, block->statement, level + 1);
        block = block->next;
    }
}

void dump_statement(FILE *stream, Statement statement, int level)
{
    switch (statement.kind) {
    case STATEMENT_KIND_EMIT_INST: {
        Inst_Type type = statement.value.as_emit_inst.type;
        Expr operand = statement.value.as_emit_inst.operand;

        fprintf(stream, "%*sEmit_Inst:\n", level * 2, "");
        fprintf(stream, "%*sInst_Type: %s\n", (level + 1) * 2, "",
                get_inst_def(type).name);
        fprintf(stream, "%*sOperand:\n", (level + 1) * 2, "");
        dump_expr(stream, operand, level + 2);
    }
    break;

    case STATEMENT_KIND_LABEL: {
        String_View name = statement.value.as_label.name;

        fprintf(stream, "%*sLabel:\n", level * 2, "");
        fprintf(stream, "%*s"SV_Fmt"\n", (level + 1) * 2, "", SV_Arg(name));
    }
    break;

    case STATEMENT_KIND_CONST: {
        String_View name = statement.value.as_const.name;
        Expr value = statement.value.as_const.value;

        fprintf(stream, "%*sConst:\n", level * 2, "");
        fprintf(stream, "%*sName: "SV_Fmt"\n", (level + 1) * 2, "", SV_Arg(name));
        fprintf(stream, "%*sValue:\n", (level + 1) * 2, "");
        dump_expr(stream, value, level + 2);
    }
    break;

    case STATEMENT_KIND_NATIVE: {
        assert(false && "TODO(#275): dumping Native statement is not implemented");
        exit(1);
    }
    break;

    case STATEMENT_KIND_INCLUDE: {
        String_View path = statement.value.as_include.path;

        fprintf(stream, "%*sInclude:\n", level * 2, "");
        fprintf(stream, "%*s"SV_Fmt"\n", (level + 1) * 2, "", SV_Arg(path));
    }
    break;

    case STATEMENT_KIND_ASSERT: {
        fprintf(stream, "%*sAssert:\n", level * 2, "");
        dump_expr(stream, statement.value.as_assert.condition, level + 1);
    }
    break;

    case STATEMENT_KIND_ERROR: {
        fprintf(stream, "%*sError: "SV_Fmt"\n",
                level * 2, "", SV_Arg(statement.value.as_error.message));
    }
    break;

    case STATEMENT_KIND_ENTRY: {
        fprintf(stream, "%*sEntry:\n", level * 2, "");
        dump_expr(stream, statement.value.as_entry.value, level + 1);
    }
    break;

    case STATEMENT_KIND_BLOCK: {
        dump_block(stream, statement.value.as_block, level);
    }
    break;

    case STATEMENT_KIND_IF: {
        fprintf(stream, "%*sIf:\n", level * 2, "");
        fprintf(stream, "%*sCondition:\n", (level + 1) * 2, "");
        dump_expr(stream, statement.value.as_if.condition, level + 2);
        fprintf(stream, "%*sThen:\n", (level + 1) * 2, "");
        dump_block(stream, statement.value.as_if.then, level + 2);
        fprintf(stream, "%*sElse:\n", (level + 1) * 2, "");
        dump_block(stream, statement.value.as_if.elze, level + 2);
    }
    break;

    case STATEMENT_KIND_SCOPE: {
        fprintf(stream, "%*sScope:\n", level * 2, "");
        dump_block(stream, statement.value.as_scope, level + 1);
    }
    break;

    case STATEMENT_KIND_FOR: {
        fprintf(stream, "%*sFor\n", level * 2, "");
        fprintf(stream, "%*sVar: "SV_Fmt"\n", (level + 1) * 2, "",
                SV_Arg(statement.value.as_for.var));
        fprintf(stream, "%*sFrom:\n", (level + 1) * 2, "");
        dump_expr(stream, statement.value.as_for.from, level + 2);
        fprintf(stream, "%*sTo:\n", (level + 1) * 2, "");
        dump_expr(stream, statement.value.as_for.to, level + 2);
    }
    break;

    case STATEMENT_KIND_FUNCDEF: {
        assert(false && "TODO(#270): Dumping function definitions is not implemented");
        exit(1);
    }
    break;

    case STATEMENT_KIND_MACROCALL: {
        assert(false && "Dumping macro calls is not implemented");
        exit(1);
    }
    break;

    case STATEMENT_KIND_MACRODEF: {
        assert(false && "Dumping macro definitions is not implemented");
        exit(1);
    }
    break;
    }
}

int dump_block_as_dot_edges(FILE *stream, Block_Statement *block, int *counter)
{
    int id = (*counter)++;

    fprintf(stream, "Expr_%d [shape=box label=\"Block\"]\n", id);

    int block_id = id;

    while (block) {
        int next_id = dump_statement_as_dot_edges(stream, block->statement, counter);
        if (block_id >= 0) {
            fprintf(stream, "Expr_%d -> Expr_%d\n", block_id, next_id);
        }

        block_id = next_id;
        block = block->next;
    }

    return id;
}

int dump_statement_as_dot_edges(FILE *stream, Statement statement, int *counter)
{
    switch (statement.kind) {
    case STATEMENT_KIND_EMIT_INST: {
        int id = (*counter)++;
        Inst_Type type = statement.value.as_emit_inst.type;
        Expr operand = statement.value.as_emit_inst.operand;
        Inst_Def inst_def = get_inst_def(type);

        fprintf(stream, "Expr_%d [shape=box label=\"%s\"]\n",
                id, inst_def.name);

        if (inst_def.has_operand) {
            int child_id = dump_expr_as_dot_edges(stream, operand, counter);
            fprintf(stream, "Expr_%d -> Expr_%d [style=dotted]\n", id, child_id);
        }
        return id;
    }
    break;

    case STATEMENT_KIND_LABEL: {
        int id = (*counter)++;
        String_View name = statement.value.as_label.name;
        fprintf(stream, "Expr_%d [shape=diamond label=\"Label: "SV_Fmt"\"]\n",
                id, SV_Arg(name));
        return id;
    }
    break;

    case STATEMENT_KIND_CONST: {
        int id = (*counter)++;
        String_View name = statement.value.as_const.name;
        Expr value = statement.value.as_const.value;

        fprintf(stream, "Expr_%d [shape=diamond label=\"%%const "SV_Fmt"\"]\n",
                id, SV_Arg(name));
        int child_id = dump_expr_as_dot_edges(stream, value, counter);
        fprintf(stream, "Expr_%d -> Expr_%d [style=dotted]\n", id, child_id);
        return id;
    }
    break;

    case STATEMENT_KIND_NATIVE: {
        int id = (*counter)++;
        String_View name = statement.value.as_native.name;

        fprintf(stream, "Expr_%d [shape=diamond label=\"%%native "SV_Fmt"\"]\n",
                id, SV_Arg(name));
        return id;
    }
    break;

    case STATEMENT_KIND_INCLUDE: {
        int id = (*counter)++;
        int child_id = (*counter)++;

        String_View path = statement.value.as_include.path;
        fprintf(stream, "Expr_%d [shape=diamond label=\"%%include\"]\n",
                id);
        fprintf(stream, "Expr_%d [shape=box label=\""SV_Fmt"\"]\n",
                child_id, SV_Arg(path));
        fprintf(stream, "Expr_%d -> Expr_%d [style=dotted]\n", id, child_id);
        return id;
    }
    break;

    case STATEMENT_KIND_ASSERT: {
        int id = (*counter)++;
        fprintf(stream, "Expr_%d [shape=diamond label=\"%%assert\"]\n",
                id);
        int child_id = dump_expr_as_dot_edges(stream, statement.value.as_assert.condition, counter);
        fprintf(stream, "Expr_%d -> Expr_%d [style=dotted]\n", id, child_id);
        return id;
    }
    break;

    case STATEMENT_KIND_ERROR: {
        int id = (*counter)++;
        fprintf(stream, "Expr_%d [shape=diamond label=\"%%error\"]\n",
                id);
        int child_id = (*counter)++;
        fprintf(stream, "Expr_%d [shape=box label=\"\\\""SV_Fmt"\\\"\"]\n",
                child_id, SV_Arg(statement.value.as_error.message));
        fprintf(stream, "Expr_%d -> Expr_%d [style=dotted]\n", id, child_id);
        return id;
    }
    break;

    case STATEMENT_KIND_ENTRY: {
        int id = (*counter)++;
        fprintf(stream, "Expr_%d [shape=diamond label=\"%%entry\"]\n",
                id);
        int child_id = dump_expr_as_dot_edges(stream, statement.value.as_entry.value, counter);
        fprintf(stream, "Expr_%d -> Expr_%d [style=dotted]\n", id, child_id);
        return id;
    }
    break;

    case STATEMENT_KIND_BLOCK: {
        return dump_block_as_dot_edges(stream, statement.value.as_block, counter);
    }
    break;

    case STATEMENT_KIND_IF: {
        int id = (*counter)++;
        fprintf(stream, "Expr_%d [shape=box label=\"If\"]\n", id);

        // Condition
        {
            int condition_id = (*counter)++;
            fprintf(stream, "Expr_%d [shape=box label=\"Condition\"]\n", condition_id);
            fprintf(stream, "Expr_%d -> Expr_%d\n", id, condition_id);
            int condition_child_id =
                dump_expr_as_dot_edges(stream, statement.value.as_if.condition,
                                       counter);
            fprintf(stream, "Expr_%d -> Expr_%d\n",
                    condition_id, condition_child_id);
        }

        // Then
        {
            int then_id = (*counter)++;
            fprintf(stream, "Expr_%d [shape=box label=\"Then\"]\n", then_id);
            fprintf(stream, "Expr_%d -> Expr_%d\n", id, then_id);
            int then_child_id =
                dump_block_as_dot_edges(stream, statement.value.as_if.then,
                                        counter);
            fprintf(stream, "Expr_%d -> Expr_%d\n", then_id, then_child_id);
        }

        // Else
        {
            int else_id = (*counter)++;
            fprintf(stream, "Expr_%d [shape=box label=\"Else\"]\n", else_id);
            fprintf(stream, "Expr_%d -> Expr_%d\n", id, else_id);
            int else_child_id =
                dump_block_as_dot_edges(stream, statement.value.as_if.elze,
                                        counter);
            fprintf(stream, "Expr_%d -> Expr_%d\n", else_id, else_child_id);
        }

        return id;
    }
    break;

    case STATEMENT_KIND_SCOPE: {
        int id = (*counter)++;
        fprintf(stream, "Expr_%d [shape=box label=\"Scope\"]\n", id);
        int child_id =
            dump_block_as_dot_edges(stream, statement.value.as_scope,
                                    counter);
        fprintf(stream, "Expr_%d -> Expr_%d\n", id, child_id);
        return id;
    }
    break;

    case STATEMENT_KIND_FOR: {
        int id = (*counter)++;
        fprintf(stream, "Expr_%d [shape=box label=\"For\"]\n", id);

        // Var
        {
            int var_id = (*counter)++;
            fprintf(stream, "Expr_%d [shape=box label=\"Var: "SV_Fmt"\"]\n", var_id, SV_Arg(statement.value.as_for.var));
            fprintf(stream, "Expr_%d -> Expr_%d\n", id, var_id);
        }

        // From
        {
            int from_id = (*counter)++;
            fprintf(stream, "Expr_%d [shape=box label=\"From\"]\n", from_id);
            fprintf(stream, "Expr_%d -> Expr_%d\n", id, from_id);

            int from_expr_id = dump_expr_as_dot_edges(stream, statement.value.as_for.from,
                               counter);
            fprintf(stream, "Expr_%d -> Expr_%d\n", from_id, from_expr_id);
        }

        // To
        {
            int to_id = (*counter)++;
            fprintf(stream, "Expr_%d [shape=box label=\"To\"]\n", to_id);
            fprintf(stream, "Expr_%d -> Expr_%d\n", id, to_id);

            int to_expr_id = dump_expr_as_dot_edges(stream, statement.value.as_for.to,
                                                    counter);
            fprintf(stream, "Expr_%d -> Expr_%d\n", to_id, to_expr_id);
        }

        // Body
        {
            int body_id = (*counter)++;
            fprintf(stream, "Expr_%d [shape=box label=\"Body\"]\n", body_id);
            fprintf(stream, "Expr_%d -> Expr_%d\n", id, body_id);

            int body_block_id = dump_block_as_dot_edges(stream, statement.value.as_for.body, counter);
            fprintf(stream, "Expr_%d -> Expr_%d\n", body_id, body_block_id);
        }

        return id;
    }
    break;

    case STATEMENT_KIND_FUNCDEF: {
        int id = (*counter)++;
        fprintf(stream, "Expr_%d [shape=box label=\"Fundef\"]\n", id);

        // Name + Args
        {
            int name_id = (*counter)++;
            fprintf(stream, "Expr_%d [shape=box label=\"Name: "SV_Fmt"\"]\n",
                    name_id, SV_Arg(statement.value.as_fundef.name));
            fprintf(stream, "Expr_%d -> Expr_%d\n", id, name_id);

            for (Fundef_Arg *arg = statement.value.as_fundef.args;
                    arg != NULL;
                    arg = arg->next) {
                int child_id = (*counter)++;
                fprintf(stream, "Expr_%d [shape=box label=\""SV_Fmt"\"]",
                        child_id, SV_Arg(arg->name));
                fprintf(stream, "Expr_%d -> Expr_%d\n", name_id, child_id);
            }
        }

        // Guard
        if (statement.value.as_fundef.guard) {
            int guard_id = (*counter)++;
            fprintf(stream, "Expr_%d [shape=box label=\"Guard\"]\n", guard_id);
            fprintf(stream, "Expr_%d -> Expr_%d\n", id, guard_id);

            int condition_id = dump_expr_as_dot_edges(stream, *statement.value.as_fundef.guard, counter);
            fprintf(stream, "Expr_%d -> Expr_%d\n", guard_id, condition_id);
        }

        // Body
        {
            int body_id = (*counter)++;
            fprintf(stream, "Expr_%d [shape=box label=\"Body\"]\n", body_id);
            fprintf(stream, "Expr_%d -> Expr_%d\n", id, body_id);

            int expr_id = dump_expr_as_dot_edges(stream, statement.value.as_fundef.body, counter);
            fprintf(stream, "Expr_%d -> Expr_%d\n", body_id, expr_id);
        }

        return id;
    }
    break;

    case STATEMENT_KIND_MACROCALL: {
        int id = (*counter)++;
        fprintf(stream, "Expr_%d [shape=box label=\"Macrocall: "SV_Fmt"\"]\n",
                id, SV_Arg(statement.value.as_macrocall.name));

        int args_id = dump_funcall_args_as_dot_edges(stream, statement.value.as_macrocall.args, counter);
        fprintf(stream, "Expr_%d -> Expr_%d [style=dotted]\n", id, args_id);

        return id;
    }
    break;

    case STATEMENT_KIND_MACRODEF: {
        int id = (*counter)++;

        fprintf(stream, "Expr_%d [shape=box label=\"Macrodef: "SV_Fmt"\"]\n",
                id, SV_Arg(statement.value.as_macrodef.name));

        int args_id = dump_fundef_args_as_dot_edges(stream, statement.value.as_macrodef.args, counter);
        fprintf(stream, "Expr_%d -> Expr_%d [style=dotted]\n", id, args_id);


        int body_id = dump_block_as_dot_edges(stream, statement.value.as_macrodef.body, counter);
        fprintf(stream, "Expr_%d -> Expr_%d [style=dotted]\n", id, body_id);

        return id;
    }
    break;

    default: {
        assert(false && "dump_statement_as_dot_edges: unreachable");
        exit(1);
    }
    }
}

void dump_statement_as_dot(FILE *stream, Statement statement)
{
    fprintf(stream, "digraph AST {\n");
    int counter = 0;
    dump_statement_as_dot_edges(stream, statement, &counter);
    fprintf(stream, "}\n");
}

void block_list_push(Arena *arena, Block_List *list, Statement statement)
{
    assert(list);

    if (list->end == NULL) {
        assert(list->begin == NULL);
        Block_Statement *block = arena_alloc(arena, sizeof(Block_Statement));
        block->statement = statement;
        list->begin = block;
        list->end = block;
    } else {
        assert(list->begin != NULL);
        Block_Statement *block = arena_alloc(arena, sizeof(Block_Statement));
        block->statement = statement;
        list->end->next = block;
        list->end = block;
    }
}

Statement parse_if_else_body_from_lines(Arena *arena, Linizer *linizer,
                                        Expr condition, File_Location location)
{
    Statement statement = {0};
    statement.location = location;
    statement.kind = STATEMENT_KIND_IF;
    statement.value.as_if.condition = condition;
    statement.value.as_if.then = parse_block_from_lines(arena, linizer);

    Line line = {0};

    if (!linizer_next(linizer, &line) || line.kind != LINE_KIND_DIRECTIVE) {
        fprintf(stderr, FL_Fmt": ERROR: expected `%%end` or `%%else` or `%%elif` after `%%if`\n",
                FL_Arg(linizer->location));
        fprintf(stderr, FL_Fmt": NOTE: %%if is here\n",
                FL_Arg(statement.location));
        exit(1);
    }

    if (sv_eq(line.value.as_directive.name, SV("else"))) {
        File_Location else_location = line.location;
        statement.value.as_if.elze = parse_block_from_lines(arena, linizer);
        if (!linizer_next(linizer, &line) ||
                line.kind != LINE_KIND_DIRECTIVE ||
                !sv_eq(line.value.as_directive.name, SV("end"))) {
            fprintf(stderr, FL_Fmt": ERROR: expected `%%end` after `%%else`\n",
                    FL_Arg(linizer->location));
            fprintf(stderr, FL_Fmt": NOTE: %%else is here\n",
                    FL_Arg(else_location));
            exit(1);
        }
    } else if (sv_eq(line.value.as_directive.name, SV("end"))) {
        // Elseless if
    } else if (sv_eq(line.value.as_directive.name, SV("elif"))) {
        Expr elif_condition = parse_expr_from_sv(arena, line.value.as_directive.body, line.location);
        statement.value.as_if.elze = arena_alloc(arena, sizeof(*statement.value.as_if.elze));
        statement.value.as_if.elze->statement =
            parse_if_else_body_from_lines(arena, linizer, elif_condition, line.location);
    } else {
        fprintf(stderr, FL_Fmt": ERROR: expected `%%end` or `%%else` after `%%if`, but got `"SV_Fmt"`\n",
                FL_Arg(linizer->location),
                SV_Arg(line.value.as_directive.name));
        fprintf(stderr, FL_Fmt": NOTE: %%if is here\n",
                FL_Arg(statement.location));
        exit(1);
    }

    return statement;
}

void parse_directive_from_line(Arena *arena, Linizer *linizer, Block_List *output)
{
    Line line = {0};

    if (!linizer_next(linizer, &line) || line.kind != LINE_KIND_DIRECTIVE) {
        fprintf(stderr, FL_Fmt": ERROR: expected a directive line\n",
                FL_Arg(linizer->location));
        exit(1);
    }

    File_Location location = line.location;
    String_View name = line.value.as_directive.name;
    String_View body = line.value.as_directive.body;

    if (sv_eq(name, sv_from_cstr("include"))) {
        Statement statement = {0};
        statement.location = location;
        statement.kind = STATEMENT_KIND_INCLUDE;
        // TODO(#226): it is a bit of an overhead to call the whole parser to just parse the path of the include
        // It would be better to extract specifically string parsing into a separate function and call it directly.
        Expr path = parse_expr_from_sv(arena, body, line.location);
        if (path.kind != EXPR_KIND_LIT_STR) {
            fprintf(stderr, FL_Fmt"ERROR: expected string literal as path for %%include directive\n",
                    FL_Arg(location));
            exit(1);
        }

        statement.value.as_include.path = path.value.as_lit_str;

        block_list_push(arena, output, statement);
    } else if (sv_eq(name, sv_from_cstr("const"))) {
        Statement statement = {0};
        statement.location = location;
        statement.kind = STATEMENT_KIND_CONST;

        Tokenizer tokenizer = tokenizer_from_sv(body);
        Expr binding_name = parse_expr_from_tokens(arena, &tokenizer, location);
        if (binding_name.kind != EXPR_KIND_BINDING) {
            fprintf(stderr, FL_Fmt": ERROR: expected binding name for %%const binding\n",
                    FL_Arg(location));
            exit(1);
        }
        statement.value.as_const.name = binding_name.value.as_binding;

        expect_token_next(&tokenizer, TOKEN_KIND_EQ, location);

        statement.value.as_const.value =
            parse_expr_from_tokens(arena, &tokenizer, location);
        expect_no_tokens(&tokenizer, location);

        block_list_push(arena, output, statement);
    } else if (sv_eq(name, sv_from_cstr("native"))) {
        Statement statement = {0};
        statement.location = location;
        statement.kind = STATEMENT_KIND_NATIVE;

        Tokenizer tokenizer = tokenizer_from_sv(body);
        Expr binding_name = parse_expr_from_tokens(arena, &tokenizer, location);
        if (binding_name.kind != EXPR_KIND_BINDING) {
            fprintf(stderr, FL_Fmt": ERROR: expected binding name for %%native binding\n",
                    FL_Arg(location));
            exit(1);
        }
        statement.value.as_native.name = binding_name.value.as_binding;
        expect_no_tokens(&tokenizer, location);

        block_list_push(arena, output, statement);
    } else if (sv_eq(name, sv_from_cstr("assert"))) {
        Statement statement = {0};
        statement.location = location;
        statement.kind = STATEMENT_KIND_ASSERT;
        statement.value.as_assert.condition =
            parse_expr_from_sv(arena, body, line.location);
        block_list_push(arena, output, statement);
    } else if (sv_eq(name, sv_from_cstr("entry"))) {
        body = sv_trim(body);
        bool inline_entry = false;

        if (sv_ends_with(body, sv_from_cstr(":"))) {
            sv_chop_right(&body, 1);
            inline_entry = true;
        }

        Expr expr = parse_expr_from_sv(arena, body, line.location);

        {
            Statement statement = {0};
            statement.location = location;
            statement.kind = STATEMENT_KIND_ENTRY;
            statement.value.as_entry.value = expr;
            block_list_push(arena, output, statement);
        }

        if (inline_entry) {
            Statement statement = {0};
            statement.location = location;
            statement.kind = STATEMENT_KIND_LABEL;

            if (expr.kind != EXPR_KIND_BINDING) {
                fprintf(stderr, FL_Fmt": ERROR: expected binding name for a label\n",
                        FL_Arg(location));
                exit(1);
            }

            statement.value.as_label.name = expr.value.as_binding;
            block_list_push(arena, output, statement);
        }
    } else if (sv_eq(name, SV("error"))) {
        Tokenizer tokenizer = tokenizer_from_sv(body);
        Statement statement = {0};
        statement.location = location;
        statement.kind = STATEMENT_KIND_ERROR;
        statement.value.as_error.message = parse_lit_str_from_tokens(&tokenizer, arena, location);
        expect_no_tokens(&tokenizer, location);
        block_list_push(arena, output, statement);
    } else if (sv_eq(name, SV("if"))) {
        Expr condition = parse_expr_from_sv(arena, body, location);
        Statement statement =
            parse_if_else_body_from_lines(arena, linizer, condition, location);
        block_list_push(arena, output, statement);
    } else if (sv_eq(name, SV("scope"))) {
        Statement statement = {0};
        statement.location = location;
        statement.kind = STATEMENT_KIND_SCOPE;
        statement.value.as_scope = parse_block_from_lines(arena, linizer);

        if (!linizer_next(linizer, &line) ||
                line.kind != LINE_KIND_DIRECTIVE ||
                !sv_eq(line.value.as_directive.name, SV("end"))) {
            fprintf(stderr, FL_Fmt": ERROR: expected `%%end` directive at the end of the `%%scope` block\n",
                    FL_Arg(linizer->location));
            fprintf(stderr, FL_Fmt": NOTE: the %%scope block starts here\n",
                    FL_Arg(statement.location));
            exit(1);
        }
        block_list_push(arena, output, statement);
    } else if (sv_eq(name, SV("for"))) {
        Statement statement = {0};
        statement.location = location;
        statement.kind = STATEMENT_KIND_FOR;

        Tokenizer tokenizer = tokenizer_from_sv(body);
        Token token = {0};

        token = expect_token_next(&tokenizer, TOKEN_KIND_NAME, location);
        statement.value.as_for.var = token.text;

        expect_token_next(&tokenizer, TOKEN_KIND_FROM, location);
        statement.value.as_for.from = parse_expr_from_tokens(arena, &tokenizer, location);
        expect_token_next(&tokenizer, TOKEN_KIND_TO, location);
        statement.value.as_for.to = parse_expr_from_tokens(arena, &tokenizer, location);
        expect_no_tokens(&tokenizer, location);

        statement.value.as_for.body = parse_block_from_lines(arena, linizer);

        if (!linizer_next(linizer, &line) ||
                line.kind != LINE_KIND_DIRECTIVE ||
                !sv_eq(line.value.as_directive.name, SV("end"))) {
            fprintf(stderr, FL_Fmt": ERROR: expected `%%end` directive at the end of the `%%for` block\n",
                    FL_Arg(linizer->location));
            fprintf(stderr, FL_Fmt": NOTE: the %%for block starts here\n",
                    FL_Arg(statement.location));
            exit(1);
        }

        block_list_push(arena, output, statement);
    } else if (sv_eq(name, SV("func"))) {
        Tokenizer tokenizer = tokenizer_from_sv(body);
        Token token = expect_token_next(&tokenizer, TOKEN_KIND_NAME, location);

        Statement statement = {0};
        statement.location = location;
        statement.kind = STATEMENT_KIND_FUNCDEF;
        statement.value.as_fundef.name = token.text;
        statement.value.as_fundef.args = parse_fundef_args(arena, &tokenizer, location);

        if (tokenizer_peek(&tokenizer, &token, location) &&
                token.kind == TOKEN_KIND_IF) {
            tokenizer_next(&tokenizer, NULL, location);
            statement.value.as_fundef.guard = arena_alloc(arena, sizeof(Expr));
            *statement.value.as_fundef.guard = parse_expr_from_tokens(arena, &tokenizer, location);
        }

        if (tokenizer_peek(&tokenizer, &token, location) &&
                token.kind == TOKEN_KIND_EQ) {
            tokenizer_next(&tokenizer, NULL, location);
            statement.value.as_fundef.body = parse_expr_from_tokens(arena, &tokenizer, location);
        } else {
            fprintf(stderr, FL_Fmt": ERROR: expected either `%s` or `%s`",
                    FL_Arg(location),
                    token_kind_name(TOKEN_KIND_IF),
                    token_kind_name(TOKEN_KIND_EQ));
            exit(1);
        }

        block_list_push(arena, output, statement);
    } else if (sv_eq(name, SV("end"))) {
        assert(false && "parse_directive_from_line: unreachable. %%end should not be handled here");
    } else if (sv_eq(name, SV("macro"))) {
        Statement statement = {0};
        statement.location = location;
        statement.kind = STATEMENT_KIND_MACRODEF;

        Tokenizer tokenizer = tokenizer_from_sv(body);
        Token token = expect_token_next(&tokenizer, TOKEN_KIND_NAME, location);

        statement.value.as_macrodef.name = token.text;
        statement.value.as_macrodef.args = parse_fundef_args(arena, &tokenizer, location);
        expect_no_tokens(&tokenizer, location);

        statement.value.as_macrodef.body = parse_block_from_lines(arena, linizer);

        if (!linizer_next(linizer, &line) ||
                line.kind != LINE_KIND_DIRECTIVE ||
                !sv_eq(line.value.as_directive.name, SV("end"))) {
            fprintf(stderr, FL_Fmt": ERROR: expected `%%end` directive at the end of the `%%macro` block\n",
                    FL_Arg(linizer->location));
            fprintf(stderr, FL_Fmt": NOTE: the %%macro block starts here\n",
                    FL_Arg(statement.location));
            exit(1);
        }

        block_list_push(arena, output, statement);
    } else {
        Tokenizer tokenizer = tokenizer_from_sv(body);

        {
            Token token = {0};
            if (!tokenizer_peek(&tokenizer, &token, location) || token.kind != TOKEN_KIND_OPEN_PAREN) {
                fprintf(stderr, FL_Fmt": ERROR: unknown directive `"SV_Fmt"`\n",
                        FL_Arg(location), SV_Arg(name));
                exit(1);
            }
        }

        Statement statement = {0};
        statement.location = location;
        statement.kind = STATEMENT_KIND_MACROCALL;
        statement.value.as_macrocall.name = name;
        statement.value.as_macrocall.args = parse_funcall_args(arena, &tokenizer, location);
        expect_no_tokens(&tokenizer, location);
        block_list_push(arena, output, statement);
    }
}

static bool is_block_stop_directive(Line_Directive directive)
{
    return sv_eq(directive.name, SV("end"))
           || sv_eq(directive.name, SV("else"))
           || sv_eq(directive.name, SV("elif"));
}

Block_Statement *parse_block_from_lines(Arena *arena, Linizer *linizer)
{
    Block_List result = {0};

    Line line = {0};
    while (linizer_peek(linizer, &line)) {
        const File_Location location = line.location;
        switch (line.kind) {
        case LINE_KIND_INSTRUCTION: {
            String_View name = line.value.as_instruction.name;
            String_View operand_sv = line.value.as_instruction.operand;

            Inst_Def inst_def;
            if (!inst_by_name(name, &inst_def)) {
                fprintf(stderr, FL_Fmt": ERROR: unknown instruction `"SV_Fmt"`\n",
                        FL_Arg(location), SV_Arg(name));
                exit(1);
            }


            Statement statement = {0};
            statement.location = location;
            statement.kind = STATEMENT_KIND_EMIT_INST;
            statement.value.as_emit_inst.type = inst_def.type;

            if (inst_def.has_operand) {
                Expr operand = parse_expr_from_sv(arena, operand_sv, location);
                statement.value.as_emit_inst.operand = operand;
            }

            block_list_push(arena, &result, statement);

            linizer_next(linizer, NULL);
        }
        break;

        case LINE_KIND_LABEL: {
            // TODO(#228): it is a bit of an overhead to call the whole parser to just parse the name of the label
            // It would be better to extract specifically binding parsing into a separate function and call it directly.
            Expr label = parse_expr_from_sv(arena, line.value.as_label.name, location);

            if (label.kind != EXPR_KIND_BINDING) {
                fprintf(stderr, FL_Fmt": ERROR: expected binding name for a label\n",
                        FL_Arg(location));
                exit(1);
            }

            Statement statement = {0};
            statement.location = location;
            statement.kind = STATEMENT_KIND_LABEL;
            statement.value.as_label.name = label.value.as_binding;
            block_list_push(arena, &result, statement);

            linizer_next(linizer, NULL);
        }
        break;

        case LINE_KIND_DIRECTIVE: {
            if (is_block_stop_directive(line.value.as_directive)) {
                return result.begin;
            }

            parse_directive_from_line(arena, linizer, &result);
        }
        break;
        }
    }

    return result.begin;
}
