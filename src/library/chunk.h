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
    Inst_Type type;
    Expr operand;
} Deferred_Inst;

typedef struct {
    Deferred_Inst insts[CHUNK_CAPACITY];
    size_t insts_size;
} Regular_Chunk;

void regular_chunk_append(Regular_Chunk *chunk, Deferred_Inst inst);
bool regular_chunk_full(const Regular_Chunk *chunk);

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

Chunk *chunk_append_regular(Chunk *chunk, Regular_Chunk *regular_chunk);

void chunk_dump(FILE *stream, const Chunk *chunk, int level);

Chunk *chunk_translate_lines(Basm *basm, Linizer *linizer);
Chunk *chunk_translate_file(Basm *basm, String_View input_file_path);

#endif // CHUNK_H_
