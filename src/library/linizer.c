#include "./linizer.h"

void line_dump(FILE *stream, const Line *line)
{
    switch (line->kind) {
    case LINE_KIND_INSTRUCTION:
        fprintf(stream, FL_Fmt": INSTRUCTION: name: "SV_Fmt", operand: "SV_Fmt"\n",
                FL_Arg(line->location),
                SV_Arg(line->value.as_instruction.name),
                SV_Arg(line->value.as_instruction.operand));
        break;
    case LINE_KIND_LABEL:
        fprintf(stream, FL_Fmt": LABEL: name: "SV_Fmt"\n",
                FL_Arg(line->location),
                SV_Arg(line->value.as_label.name));
        break;
    case LINE_KIND_DIRECTIVE:
        fprintf(stream, FL_Fmt": DIRECTIVE: name: "SV_Fmt", body: "SV_Fmt"\n",
                FL_Arg(line->location),
                SV_Arg(line->value.as_directive.name),
                SV_Arg(line->value.as_directive.body));
        break;
    }
}

bool linizer_from_file(Linizer *linizer, Arena *arena, String_View file_path)
{
    assert(linizer);

    if (arena_slurp_file(arena, file_path, &linizer->source) < 0) {
        return false;
    }

    linizer->location.file_path = file_path;

    return true;
}

bool linizer_peek(Linizer *linizer, Line *output)
{
    // We already have something cached in the peek buffer.
    // Let's just return it.
    if (linizer->peek_buffer_full) {
        if (output) {
            *output = linizer->peek_buffer;
        }
        return true;
    }

    // Skipping empty lines
    String_View line = {0};
    do {
        line = sv_trim(sv_chop_by_delim(&linizer->source, '\n'));
        line = sv_trim(sv_chop_by_delim(&line, BASM_COMMENT_SYMBOL));
        linizer->location.line_number += 1;
    } while (line.count == 0 && linizer->source.count > 0);

    // We reached the end. Nothing more to linize.
    if (line.count == 0 && linizer->source.count == 0) {
        return false;
    }

    // Linize
    Line result = {0};
    result.location = linizer->location;
    if (sv_starts_with(line, sv_from_cstr("%"))) {
        sv_chop_left(&line, 1);
        result.kind = LINE_KIND_DIRECTIVE;
        result.value.as_directive.name = sv_trim(sv_chop_by_delim(&line, ' '));
        result.value.as_directive.body = sv_trim(line);
    } else if (sv_ends_with(line, sv_from_cstr(":"))) {
        result.kind = LINE_KIND_LABEL;
        result.value.as_label.name = sv_trim(sv_chop_by_delim(&line, ':'));
    } else {
        result.kind = LINE_KIND_INSTRUCTION;
        result.value.as_instruction.name = sv_trim(sv_chop_by_delim(&line, ' '));
        result.value.as_instruction.operand = sv_trim(line);
    }

    if (output) {
        *output = result;
    }

    // Cache the linize into the peek buffer
    linizer->peek_buffer_full = true;
    linizer->peek_buffer = result;

    return true;
}

bool linizer_next(Linizer *linizer, Line *output)
{
    if (linizer_peek(linizer, output)) {
        linizer->peek_buffer_full = false;
        return true;
    }

    return false;
}
