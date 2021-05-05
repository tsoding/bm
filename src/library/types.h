#ifndef TYPES_H_
#define TYPES_H_

#include <stdio.h>
#include "sv.h"

typedef enum {
    TYPE_REPR_ANY,
    TYPE_REPR_U64,
    TYPE_REPR_I64,
    TYPE_REPR_F64,
} Type_Repr;

typedef union {
    uint64_t as_u64;
    int64_t as_i64;
    double as_f64;
    void *as_ptr;
} Word;

Word word_u64(uint64_t u64);
Word word_i64(int64_t i64);
Word word_f64(double f64);
Word word_ptr(void *ptr);

Word word_plus_repr(Word a, Word b, Type_Repr repr);
Word word_minus_repr(Word a, Word b, Type_Repr repr);
Word word_mult_repr(Word a, Word b, Type_Repr repr);
Word word_div_repr(Word a, Word b, Type_Repr repr);
Word word_gt_repr(Word a, Word b, Type_Repr repr);
Word word_lt_repr(Word a, Word b, Type_Repr repr);
Word word_eq_repr(Word a, Word b, Type_Repr repr);
Word word_mod_repr(Word a, Word b, Type_Repr repr);

typedef enum {
    TYPE_ANY = 0,
    TYPE_FLOAT,
    TYPE_SIGNED_INT,
    TYPE_UNSIGNED_INT,
    TYPE_MEM_ADDR,
    TYPE_INST_ADDR,
    TYPE_STACK_ADDR,
    TYPE_NATIVE_ID,
    TYPE_BOOL,
    COUNT_TYPES
} Type;

Type_Repr type_repr_of(Type type);
Word convert_type_reprs(Word input, Type_Repr from, Type_Repr to);

bool type_by_name(String_View name, Type *type);
const char *type_name(Type type);
Type supertype_of(Type subtype);
bool is_subtype_of(Type a, Type b);
void dump_type_hierarchy_as_dot(FILE *stream);

#endif // TYPES_H_
