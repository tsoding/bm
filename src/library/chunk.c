#include "./chunk.h"

void regular_chunk_append(Regular_Chunk *chunk, Deferred_Inst inst)
{
    if (!regular_chunk_full(chunk)) {
        chunk->insts[chunk->insts_size++] = inst;
    }
}

bool regular_chunk_full(const Regular_Chunk *chunk)
{
    return chunk->insts_size >= CHUNK_CAPACITY;
}

static void regular_chunk_dump(FILE *stream,
                               const Regular_Chunk *chunk,
                               int level)
{
    for (size_t i = 0; i < chunk->insts_size; ++i) {
        fprintf(stream, "%*s%s\n", level * 2, "", inst_name(chunk->insts[i].type));
        if (inst_has_operand(chunk->insts[i].type)) {
            dump_expr(stream, chunk->insts[i].operand, level + 1);
        }
    }
}

static void conditional_chunk_dump(FILE *stream,
                                   const Conditional_Chunk *chunk,
                                   int level)
{
    fprintf(stream, "%*s%%if\n", level * 2, "");
    chunk_dump(stream, chunk->then, level + 1);
    fprintf(stream, "%*s%%end\n", level * 2, "");
}

void chunk_dump(FILE *stream, const Chunk *chunk, int level)
{
    while (chunk) {
        switch (chunk->kind) {
        case CHUNK_KIND_REGULAR:
            regular_chunk_dump(stream, &chunk->value.as_regular, level);
            break;
        case CHUNK_KIND_CONDITIONAL:
            conditional_chunk_dump(stream, &chunk->value.as_conditional, level);
            break;
        }

        fprintf(stream, "==============================\n");

        chunk = chunk->next;
    }
}

void chunk_list_push_inst(Arena *arena, Chunk_List *a, Deferred_Inst inst)
{
    assert(a);
    if (a->end == NULL) {
        assert(a->begin == NULL);
        Chunk *chunk = arena_alloc(arena, sizeof(Chunk));
        chunk->kind = CHUNK_KIND_REGULAR;
        a->begin = chunk;
        a->end = chunk;
    }

    assert(a->end->next == NULL);
    if (a->end->kind != CHUNK_KIND_REGULAR
            || regular_chunk_full(&a->end->value.as_regular)) {
        Chunk *chunk = arena_alloc(arena, sizeof(Chunk));
        chunk->kind = CHUNK_KIND_REGULAR;
        a->end->next = chunk;
        a->end = chunk;
    }

    regular_chunk_append(&a->end->value.as_regular, inst);
}

void chunk_list_append(Chunk_List *a, Chunk_List b)
{
    assert(a);

    if (a->end == NULL) {
        assert(a->begin == NULL);
        a->begin = b.begin;
        a->end = b.end;
    } else {
        assert(a->end->next == NULL);
        a->end->next = b.begin;
        a->end = b.end;
    }
}

Chunk_List chunk_translate_lines(Basm *basm, Linizer *linizer)
{
    Chunk_List result = {0};

    Line line = {0};
    while (linizer_next(linizer, &line)) {
        File_Location location = line.location;
        switch (line.kind) {
        case LINE_KIND_DIRECTIVE: {
            String_View name = line.value.as_directive.name;
            String_View body = line.value.as_directive.body;
            if (sv_eq(name, sv_from_cstr("const"))) {
                basm_translate_bind_directive(basm, &body, location, BINDING_CONST);
            } else if (sv_eq(name, sv_from_cstr("native"))) {
                basm_translate_bind_directive(basm, &body, location, BINDING_NATIVE);
            } else if (sv_eq(name, sv_from_cstr("assert"))) {
                Expr expr = parse_expr_from_sv(&basm->arena, sv_trim(body), location);
                basm->deferred_asserts[basm->deferred_asserts_size++] = (Deferred_Assert) {
                    .expr = expr,
                    .location = location,
                };
            } else if (sv_eq(name, sv_from_cstr("include"))) {
                Expr expr = parse_expr_from_sv(&basm->arena, sv_trim(body), location);
                if (expr.kind != EXPR_KIND_LIT_STR) {
                    fprintf(stderr, FL_Fmt": ERROR: include only accepts strings as file paths\n", FL_Arg(location));
                    exit(1);
                }

                if (basm->include_level + 1 >= BASM_MAX_INCLUDE_LEVEL) {
                    fprintf(stderr,
                            FL_Fmt": ERROR: exceeded maximum include level\n",
                            FL_Arg(location));
                    exit(1);
                }

                {
                    File_Location prev_include_location = basm->include_location;
                    basm->include_level += 1;
                    basm->include_location = location;
                    Chunk_List include = chunk_translate_file(basm, expr.value.as_lit_str);
                    chunk_list_append(&result, include);
                    basm->include_location = prev_include_location;
                    basm->include_level -= 1;
                }
            }
        }
        break;
        case LINE_KIND_LABEL: {
            String_View label = line.value.as_label.name;
            basm_bind_value(basm, label, word_u64(basm->program_size), BINDING_LABEL, location);
        }
        break;
        case LINE_KIND_INSTRUCTION: {
            String_View name = line.value.as_instruction.name;
            String_View operand = line.value.as_instruction.operand;
            Deferred_Inst inst = {0};

            if (!inst_by_name(name, &inst.type)) {
                fprintf(stderr, FL_Fmt": ERROR: unknown instruction `"SV_Fmt"`\n",
                        FL_Arg(location),
                        SV_Arg(name));
                exit(1);
            }

            if (inst_has_operand(inst.type)) {
                if (operand.count == 0) {
                    fprintf(stderr, FL_Fmt": ERROR: instruction `"SV_Fmt"` requires an operand\n",
                            FL_Arg(location),
                            SV_Arg(name));
                    exit(1);
                }

                inst.operand = parse_expr_from_sv(&basm->arena, operand, location);
            } else {
                if (operand.count > 0) {
                    fprintf(stderr, FL_Fmt": ERROR: instruction `"SV_Fmt"` does not accept an operand\n",
                            FL_Arg(location),
                            SV_Arg(name));
                    exit(1);
                }
            }

            chunk_list_push_inst(&basm->arena, &result, inst);
        }
        break;
        }
    }
    return result;
}

Chunk_List chunk_translate_file(Basm *basm, String_View input_file_path_)
{
    {
        String_View resolved_path = SV_NULL;
        if (basm_resolve_include_file_path(basm, input_file_path_, &resolved_path)) {
            input_file_path_ = resolved_path;
        }
    }

    Linizer linizer = {0};
    if (!linizer_from_file(&linizer, &basm->arena, input_file_path_)) {
        if (basm->include_level > 0) {
            fprintf(stderr, FL_Fmt": ERROR: could not read file `"SV_Fmt"`: %s\n",
                    FL_Arg(basm->include_location),
                    SV_Arg(input_file_path_), strerror(errno));
        } else {
            fprintf(stderr, "ERROR: could not read file `"SV_Fmt"`: %s\n",
                    SV_Arg(input_file_path_), strerror(errno));
        }
        exit(1);

    }

    return chunk_translate_lines(basm, &linizer);
}
