#ifndef BM_H_
#define BM_H_

// TODO(#267): bm does not enforce endianess at runtime

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>

#include "./sv.h"
#include "./types.h"

// NOTE: Stolen from https://stackoverflow.com/a/3312896
#if defined(__GNUC__) || defined(__clang__)
#  define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#  define EXPORT
#elif defined(_MSC_VER)
#  define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#  define EXPORT __declspec(dllexport)
#else
#  error "Packed attributes for struct is not implemented for this compiler. This may result in a program working incorrectly. Feel free to fix that and submit a Pull Request to https://github.com/tsoding/bm"
#endif

#define BM_WORD_SIZE 8
#define BM_STACK_CAPACITY 1024
#define BM_PROGRAM_CAPACITY 1024
#define BM_NATIVES_CAPACITY 1024
#define BM_MEMORY_CAPACITY (640 * 1000)
#define BM_EXTERNAL_NATIVES_CAPACITY 1024

typedef enum {
    ERR_OK = 0,
    ERR_STACK_OVERFLOW,
    ERR_STACK_UNDERFLOW,
    ERR_ILLEGAL_INST,
    ERR_ILLEGAL_INST_ACCESS,
    ERR_ILLEGAL_OPERAND,
    ERR_ILLEGAL_MEMORY_ACCESS,
    ERR_DIV_BY_ZERO,
    ERR_NULL_NATIVE
} Err;

const char *err_as_cstr(Err err);

typedef enum {
    INST_NOP = 0,
    INST_PUSH,
    INST_DROP,
    INST_DUP,
    INST_SWAP,
    INST_PLUSI,
    INST_MINUSI,
    INST_MULTI,
    INST_DIVI,
    INST_MODI,
    INST_MULTU,
    INST_DIVU,
    INST_MODU,
    INST_PLUSF,
    INST_MINUSF,
    INST_MULTF,
    INST_DIVF,
    INST_JMP,
    INST_JMP_IF,
    INST_RET,
    INST_CALL,
    INST_NATIVE,
    INST_HALT,
    INST_NOT,

    INST_EQI,
    INST_GEI,
    INST_GTI,
    INST_LEI,
    INST_LTI,
    INST_NEI,

    INST_EQU,
    INST_GEU,
    INST_GTU,
    INST_LEU,
    INST_LTU,
    INST_NEU,

    INST_EQF,
    INST_GEF,
    INST_GTF,
    INST_LEF,
    INST_LTF,
    INST_NEF,

    INST_ANDB,
    INST_ORB,
    INST_XOR,
    INST_SHR,
    INST_SHL,
    INST_NOTB,

    INST_READ8U,
    INST_READ16U,
    INST_READ32U,
    INST_READ64U,

    INST_READ8I,
    INST_READ16I,
    INST_READ32I,
    INST_READ64I,

    INST_WRITE8,
    INST_WRITE16,
    INST_WRITE32,
    INST_WRITE64,

    INST_I2F,
    INST_U2F,
    INST_F2I,
    INST_F2U,

    NUMBER_OF_INSTS,
} Inst_Type;

#define TYPE_LIST_CAPACITY 2
typedef struct {
    Type types[TYPE_LIST_CAPACITY];
    size_t size;
} Type_List;

#define TYPE_LIST(...) {.types = {__VA_ARGS__}, .size = sizeof((Type[]){__VA_ARGS__}) / sizeof(Type)}

typedef struct {
    Inst_Type type;
    const char *name;
    bool has_operand;
    Type operand_type;
    Type_List input;
    Type_List output;
} Inst_Def;

Inst_Def get_inst_def(Inst_Type type);
bool inst_by_name(String_View name, Inst_Def *inst_def);

typedef uint64_t Inst_Addr;
typedef uint64_t Memory_Addr;
typedef uint64_t Stack_Addr;
typedef uint64_t Native_ID;

static_assert(sizeof(Word) == BM_WORD_SIZE,
              "The BM's Word is expected to be 64 bits");

typedef struct {
    Inst_Type type;
    Word operand;
} Inst;

typedef struct Bm Bm;

typedef Err (*Bm_Native)(Bm*);

#define NATIVE_NAME_CAPACITY 256

typedef struct {
    char name[NATIVE_NAME_CAPACITY];
} External_Native;

struct Bm {
    Word stack[BM_STACK_CAPACITY];
    uint64_t stack_size;

    Inst program[BM_PROGRAM_CAPACITY];
    uint64_t program_size;
    Inst_Addr ip;

    Bm_Native natives[BM_NATIVES_CAPACITY];
    size_t natives_size;

    External_Native externals[BM_EXTERNAL_NATIVES_CAPACITY];
    size_t externals_size;

    uint8_t memory[BM_MEMORY_CAPACITY];
    // NOTE: `expected_memory_size` is the size of the memory from the bm file.
    // The program is allowed to access memory beyond the `expected_memory_size`.
    // This variable is needed for debasm to reliably recover the source code.
    size_t expected_memory_size;
    size_t memory_base;

    bool halt;
};

Err bm_execute_inst(Bm *bm);
Err bm_execute_program(Bm *bm, int limit);
void bm_push_native(Bm *bm, Bm_Native native);
void bm_dump_stack(FILE *stream, const Bm *bm);
void bm_load_program_from_file(Bm *bm, const char *file_path);

#define BM_FILE_MAGIC 0xa4016d62
#define BM_FILE_VERSION 8

PACK(struct Bm_File_Meta {
    uint32_t magic;
    uint16_t version;
    uint64_t program_size;
    uint64_t entry;
    uint64_t memory_base;
    uint64_t memory_size;
    uint64_t memory_capacity;
    uint64_t externals_size;
});

typedef struct Bm_File_Meta Bm_File_Meta;

Err native_write(Bm *bm);
Err native_external(Bm *bm);

#endif // BM_H_
