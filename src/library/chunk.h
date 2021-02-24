#ifndef CHUNK_H_
#define CHUNK_H_

#include "./bm.h"
#include "./expr.h"
#include "./basm.h"
#include "./linizer.h"

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
