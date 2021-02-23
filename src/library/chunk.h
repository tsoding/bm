#ifndef CHUNK_H_
#define CHUNK_H_

#include "./bm.h"
#include "./expr.h"
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

void line_dump(FILE *stream, const Line *line);

size_t linize_source(String_View source, Line *lines, size_t lines_capacity, File_Location location);

typedef struct Chunk Chunk;

typedef enum {
    CHUNK_KIND_REGULAR = 0,
    CHUNK_KIND_CONDITIONAL
} Chunk_Kind;

#define CHUNK_CAPACITY 256

typedef struct {
    Inst insts[CHUNK_CAPACITY];
    size_t insts_size;
} Regular_Chunk;

typedef struct {
    Expr condition;
    Chunk *then;
} Conditional_Chunk;

typedef union {
    Regular_Chunk as_regular;
    Conditional_Chunk as_conditional;
} Chunk_Value;

struct Chunk {
    Chunk_Kind kind;
    Chunk_Value value;
    Chunk *next;
};

void chunk_dump(FILE *stream, const Chunk *chunk, int level);

Chunk *chunk_translate_lines(Basm *basm, const Line *lines, size_t lines_size);
Chunk *chunk_translate_file(Basm *basm, String_View input_file_path);

#endif // CHUNK_H_
