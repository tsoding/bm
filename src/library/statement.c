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

    case STATEMENT_KIND_BLOCK: {
        dump_block(stream, statement.value.as_block, level);
    }
    break;
    }
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
        case LINE_KIND_LABEL:
            assert(false && "TODO: parse_block_from_lines: label parsing is not implemented");
            break;
        case LINE_KIND_DIRECTIVE:
            assert(false && "TODO: parse_block_from_lines: directive parsing is not implemented");
            break;
        }
    }

    return result.begin;
}
