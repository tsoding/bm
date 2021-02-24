#ifndef LINIZER_H_
#define LINIZER_H_

#include "./sv.h"
#include "./basm.h"

typedef enum {
    LINE_KIND_INSTRUCTION = 0,
    LINE_KIND_LABEL,
    LINE_KIND_DIRECTIVE,
} Line_Kind;

typedef struct {
    String_View name;
    String_View operand;
} Line_Instruction;

typedef struct {
    String_View name;
} Line_Label;

typedef struct {
    String_View name;
    String_View body;
} Line_Directive;

typedef union {
    Line_Instruction as_instruction;
    Line_Label as_label;
    Line_Directive as_directive;
} Line_Value;

typedef struct {
    Line_Kind kind;
    Line_Value value;
    File_Location location;
} Line;

typedef struct {
    String_View source;
    File_Location location;
    Line peek_buffer;
    bool peek_buffer_full;
} Linizer;

bool linizer_from_file(Linizer *linizer, Arena *arena, String_View file_path);

bool linizer_peek(Linizer *linizer, Line *output);
bool linizer_next(Linizer *linizer, Line *output);

void line_dump(FILE *stream, const Line *line);

#endif // LINIZER_H_
