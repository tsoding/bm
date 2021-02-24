#include "./chunk.h"

static void regular_chunk_dump(FILE *stream,
                               const Regular_Chunk *chunk,
                               int level)
{
    for (size_t i = 0; i < chunk->insts_size; ++i) {
        fprintf(stream, "%*s%s\n", level * 2, "", inst_name(chunk->insts[i].type));
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

        chunk = chunk->next;
    }
}

Chunk *chunk_translate_lines(Basm *basm, const Line *lines, size_t lines_size)
{
    for (size_t i = 0; i < lines_size; ++i) {
        File_Location location = lines[i].location;
        switch (lines[i].kind) {
        case LINE_KIND_DIRECTIVE: {
            String_View name = lines[i].value.as_directive.name;
            String_View body = lines[i].value.as_directive.body;
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
                    chunk_translate_file(basm, expr.value.as_lit_str);
                    basm->include_location = prev_include_location;
                    basm->include_level -= 1;
                }
            }
        }
        break;
        case LINE_KIND_LABEL: {
            String_View label = lines[i].value.as_label.name;
            basm_bind_value(basm, label, word_u64(basm->program_size), BINDING_LABEL, location);
        }
        break;
        case LINE_KIND_INSTRUCTION:
            break;
        }
    }
    return NULL;
}

Chunk *chunk_translate_file(Basm *basm, String_View input_file_path_)
{
    {
        String_View resolved_path = SV_NULL;
        if (basm_resolve_include_file_path(basm, input_file_path_, &resolved_path)) {
            input_file_path_ = resolved_path;
        }
    }

    String_View original_source = {0};
    if (arena_slurp_file(&basm->arena, input_file_path_, &original_source) < 0) {
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

    String_View source = original_source;

    File_Location location = {
        .file_path = input_file_path_,
    };

#define LINES_CAPACITY 1024

    Line *lines = arena_alloc(&basm->arena, sizeof(Line) * LINES_CAPACITY);
    size_t lines_size = linize_source(source, lines, LINES_CAPACITY, location);
    return chunk_translate_lines(basm, lines, lines_size);
}
