#include "./statement.h"

void dump_block(FILE *stream, Block *block, int level)
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
                inst_name(type));
        fprintf(stream, "%*sOperand:\n", (level + 1) * 2, "");
        dump_expr(stream, operand, level + 2);
    }
    break;

    case STATEMENT_KIND_BIND_LABEL: {
        String_View name = statement.value.as_bind_label.name;

        fprintf(stream, "%*sLabel:\n", level * 2, "");
        fprintf(stream, "%*s"SV_Fmt"\n", (level + 1) * 2, "", SV_Arg(name));
    }
    break;

    case STATEMENT_KIND_BIND_CONST: {
        String_View name = statement.value.as_bind_const.name;
        Expr value = statement.value.as_bind_const.value;

        fprintf(stream, "%*sBind Const:\n", level * 2, "");
        fprintf(stream, "%*sName: "SV_Fmt"\n", (level + 1) * 2, "", SV_Arg(name));
        fprintf(stream, "%*sValue:\n", (level + 1) * 2, "");
        dump_expr(stream, value, level + 2);
    }
    break;

    case STATEMENT_KIND_BIND_NATIVE: {
        String_View name = statement.value.as_bind_native.name;
        Expr value = statement.value.as_bind_native.value;

        fprintf(stream, "%*sBind Native:\n", level * 2, "");
        fprintf(stream, "%*sName: "SV_Fmt"\n", (level + 1) * 2, "", SV_Arg(name));
        fprintf(stream, "%*sValue:\n", (level + 1) * 2, "");
        dump_expr(stream, value, level + 2);
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

    case STATEMENT_KIND_ENTRY: {
        fprintf(stream, "%*sEntry:\n", level * 2, "");
        dump_expr(stream, statement.value.as_entry.value, level + 1);
    }
    break;

    case STATEMENT_KIND_BLOCK: {
        dump_block(stream, statement.value.as_block, level);
    }
    break;
    }
}

int dump_statement_as_dot_edges(FILE *stream, Statement statement, int *counter)
{
    int id = (*counter)++;

    switch (statement.kind) {
    case STATEMENT_KIND_EMIT_INST: {
        Inst_Type type = statement.value.as_emit_inst.type;
        Expr operand = statement.value.as_emit_inst.operand;

        fprintf(stream, "Expr_%d [shape=box label=\"%s\"]\n",
                id, inst_name(type));

        if (inst_has_operand(type)) {
            int child_id = dump_expr_as_dot_edges(stream, operand, counter);
            fprintf(stream, "Expr_%d -> Expr_%d [style=dotted]\n", id, child_id);
        }
    }
    break;

    case STATEMENT_KIND_BIND_LABEL: {
        String_View name = statement.value.as_bind_label.name;
        fprintf(stream, "Expr_%d [shape=diamond label=\""SV_Fmt"\"]\n",
                id, SV_Arg(name));
    }
    break;

    case STATEMENT_KIND_BIND_CONST: {
        String_View name = statement.value.as_bind_const.name;
        Expr value = statement.value.as_bind_const.value;

        fprintf(stream, "Expr_%d [shape=diamond label=\"%%const "SV_Fmt"\"]\n",
                id, SV_Arg(name));
        int child_id = dump_expr_as_dot_edges(stream, value, counter);
        fprintf(stream, "Expr_%d -> Expr_%d [style=dotted]\n", id, child_id);
    }
    break;

    case STATEMENT_KIND_BIND_NATIVE: {
        String_View name = statement.value.as_bind_native.name;
        Expr value = statement.value.as_bind_native.value;

        fprintf(stream, "Expr_%d [shape=diamond label=\"%%native "SV_Fmt"\"]\n",
                id, SV_Arg(name));
        int child_id = dump_expr_as_dot_edges(stream, value, counter);
        fprintf(stream, "Expr_%d -> Expr_%d [style=dotted]\n", id, child_id);
    }
    break;

    case STATEMENT_KIND_INCLUDE: {
        int child_id = (*counter)++;

        String_View path = statement.value.as_include.path;
        fprintf(stream, "Expr_%d [shape=diamond label=\"%%include\"]\n",
                id);
        fprintf(stream, "Expr_%d [shape=box label=\""SV_Fmt"\"]\n",
                child_id, SV_Arg(path));
        fprintf(stream, "Expr_%d -> Expr_%d [style=dotted]\n", id, child_id);
    }
    break;

    case STATEMENT_KIND_ASSERT: {
        fprintf(stream, "Expr_%d [shape=diamond label=\"%%assert\"]\n",
                id);
        int child_id = dump_expr_as_dot_edges(stream, statement.value.as_assert.condition, counter);
        fprintf(stream, "Expr_%d -> Expr_%d [style=dotted]\n", id, child_id);
    }
    break;

    case STATEMENT_KIND_ENTRY: {
        fprintf(stream, "Expr_%d [shape=diamond label=\"%%entry\"]\n",
                id);
        int child_id = dump_expr_as_dot_edges(stream, statement.value.as_entry.value, counter);
        fprintf(stream, "Expr_%d -> Expr_%d [style=dotted]\n", id, child_id);
    }
    break;

    case STATEMENT_KIND_BLOCK: {
        fprintf(stream, "Expr_%d [shape=box label=\"Block\"]\n", id);

        int block_id = id;

        Block *block = statement.value.as_block;
        while (block) {
            int next_id = dump_statement_as_dot_edges(stream, block->statement, counter);
            if (block_id >= 0) {
                fprintf(stream, "Expr_%d -> Expr_%d\n", block_id, next_id);
            }

            block_id = next_id;
            block = block->next;
        }
    }
    break;
    }

    return id;
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
        Block *block = arena_alloc(arena, sizeof(Block));
        block->statement = statement;
        list->begin = block;
        list->end = block;
    } else {
        assert(list->begin != NULL);
        Block *block = arena_alloc(arena, sizeof(Block));
        block->statement = statement;
        list->end->next = block;
        list->end = block;
    }
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
        statement.kind = STATEMENT_KIND_INCLUDE;
        // TODO: it is a bit of an overhead to call the whole parser to just parse the path of the include
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
        // TODO: %const and %native directive should have = in it
        // Like so
        // ```basm
        // %const N = 69
        // %const M = 420
        // ```
        Statement statement = {0};
        statement.kind = STATEMENT_KIND_BIND_CONST;

        Tokenizer tokenizer = tokenizer_from_sv(body);
        Expr name = parse_expr_from_tokens(arena, &tokenizer, location);
        if (name.kind != EXPR_KIND_BINDING) {
            fprintf(stderr, FL_Fmt": ERROR: expected binding name for %%const binding\n",
                    FL_Arg(location));
            exit(1);
        }
        statement.value.as_bind_const.name = name.value.as_binding;

        statement.value.as_bind_const.value =
            parse_expr_from_tokens(arena, &tokenizer, location);
        expect_no_tokens(&tokenizer, location);

        block_list_push(arena, output, statement);
    } else if (sv_eq(name, sv_from_cstr("native"))) {
        Statement statement = {0};
        statement.kind = STATEMENT_KIND_BIND_NATIVE;

        Tokenizer tokenizer = tokenizer_from_sv(body);
        Expr name = parse_expr_from_tokens(arena, &tokenizer, location);
        if (name.kind != EXPR_KIND_BINDING) {
            fprintf(stderr, FL_Fmt": ERROR: expected binding name for %%native binding\n",
                    FL_Arg(location));
            exit(1);
        }
        statement.value.as_bind_native.name = name.value.as_binding;

        statement.value.as_bind_native.value =
            parse_expr_from_tokens(arena, &tokenizer, location);
        expect_no_tokens(&tokenizer, location);
        block_list_push(arena, output, statement);
    } else if (sv_eq(name, sv_from_cstr("assert"))) {
        Statement statement = {0};
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
            statement.kind = STATEMENT_KIND_ENTRY;
            statement.value.as_entry.value = expr;
            block_list_push(arena, output, statement);
        }

        if (inline_entry) {
            Statement statement = {0};
            statement.kind = STATEMENT_KIND_BIND_LABEL;

            if (expr.kind != EXPR_KIND_BINDING) {
                fprintf(stderr, FL_Fmt": ERROR: expected binding name for a label\n",
                        FL_Arg(location));
                exit(1);
            }

            statement.value.as_bind_label.name = expr.value.as_binding;
            block_list_push(arena, output, statement);
        }
    } else {
        fprintf(stderr, FL_Fmt": ERROR: unknown directive `"SV_Fmt"`\n",
                FL_Arg(line.location), SV_Arg(name));
        exit(1);
    }
}

Block *parse_block_from_lines(Arena *arena, Linizer *linizer)
{
    Block_List result = {0};

    Line line = {0};
    while (linizer_peek(linizer, &line)) {
        const File_Location location = line.location;
        switch (line.kind) {
        case LINE_KIND_INSTRUCTION: {
            String_View name = line.value.as_instruction.name;
            String_View operand_sv = line.value.as_instruction.operand;

            Inst_Type type;
            if (!inst_by_name(name, &type)) {
                fprintf(stderr, FL_Fmt": ERROR: unknown instruction `"SV_Fmt"`\n",
                        FL_Arg(location), SV_Arg(name));
                exit(1);
            }


            Statement statement = {0};
            statement.kind = STATEMENT_KIND_EMIT_INST;
            statement.value.as_emit_inst.type = type;

            if (inst_has_operand(type)) {
                Expr operand = parse_expr_from_sv(arena, operand_sv, location);
                statement.value.as_emit_inst.operand = operand;
            }

            block_list_push(arena, &result, statement);

            linizer_next(linizer, NULL);
        }
        break;

        case LINE_KIND_LABEL: {
            // TODO: it is a bit of an overhead to call the whole parser to just parse the name of the label
            // It would be better to extract specifically binding parsing into a separate function and call it directly.
            Expr label = parse_expr_from_sv(arena, line.value.as_label.name, location);

            if (label.kind != EXPR_KIND_BINDING) {
                fprintf(stderr, FL_Fmt": ERROR: expected binding name for a label\n",
                        FL_Arg(location));
                exit(1);
            }

            Statement statement = {0};
            statement.kind = STATEMENT_KIND_BIND_LABEL;
            statement.value.as_bind_label.name = label.value.as_binding;
            block_list_push(arena, &result, statement);

            linizer_next(linizer, NULL);
        }
        break;

        case LINE_KIND_DIRECTIVE: {
            parse_directive_from_line(arena, linizer, &result);
        }
        break;
        }
    }

    return result.begin;
}
