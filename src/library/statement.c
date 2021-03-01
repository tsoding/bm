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

            Expr operand = parse_expr_from_sv(arena, operand_sv, location);

            Statement statement = {0};
            statement.kind = STATEMENT_KIND_EMIT_INST;
            statement.value.as_emit_inst.type = type;
            statement.value.as_emit_inst.operand = operand;
            block_list_push(arena, &result, statement);

            linizer_next(linizer, NULL);
        }
        break;
        case LINE_KIND_LABEL: {
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
        case LINE_KIND_DIRECTIVE:
            assert(false && "TODO: parse_block_from_lines: directive parsing is not implemented");
            break;
        }
    }

    return result.begin;
}
