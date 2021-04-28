#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "./types.h"

bool type_by_name(String_View name, Type *output_type)
{
    for (Type type = 0; type < COUNT_TYPES; ++type) {
        if (sv_eq(name, sv_from_cstr(type_name(type)))) {
            if (output_type) {
                *output_type = type;
            }
            return true;
        }
    }
    return false;
}

const char *type_name(Type type)
{
    switch (type) {
    case TYPE_ANY:
        return "Any";
    case TYPE_NUMBER:
        return "Number";
    case TYPE_FLOAT:
        return "Float";
    case TYPE_INTEGER:
        return "Integer";
    case TYPE_SIGNED:
        return "Signed";
    case TYPE_UNSIGNED:
        return "Unsigned";
    case TYPE_MEM_ADDR:
        return "Mem_Addr";
    case TYPE_INST_ADDR:
        return "Inst_Addr";
    case TYPE_STACK_ADDR:
        return "Stack_Addr";
    case TYPE_NATIVE_ID:
        return "Native_ID";
    case COUNT_TYPES:
    default: {
        assert(false && "type_name: unreachable");
        exit(1);
    }
    }
}

bool is_subtype_of(Type a, Type b)
{
    while (a != b && a != TYPE_ANY) {
        a = supertype_of(a);
    }
    return a == b;
}

Type supertype_of(Type subtype)
{
    switch (subtype) {
    case TYPE_ANY:
        return TYPE_ANY;
    case TYPE_NUMBER:
        return TYPE_ANY;
    case TYPE_FLOAT:
        return TYPE_NUMBER;
    case TYPE_INTEGER:
        return TYPE_NUMBER;
    case TYPE_SIGNED:
        return TYPE_INTEGER;
    case TYPE_UNSIGNED:
        return TYPE_INTEGER;
    case TYPE_MEM_ADDR:
        return TYPE_UNSIGNED;
    case TYPE_INST_ADDR:
        return TYPE_UNSIGNED;
    case TYPE_STACK_ADDR:
        return TYPE_UNSIGNED;
    case TYPE_NATIVE_ID:
        return TYPE_UNSIGNED;
    case COUNT_TYPES:
    default: {
        assert(false && "type_name: unreachable");
        exit(1);
    }
    }
}

void dump_type_hierarchy_as_dot(FILE *stream)
{
    fprintf(stream, "digraph Types {\n");
    for (Type type = 0; type < COUNT_TYPES; ++type) {
        fprintf(stream, "    %s\n", type_name(type));
        fprintf(stream, "    %s -> %s\n",
                type_name(type),
                type_name(supertype_of(type)));
    }
    fprintf(stream, "}\n");
}

Type_Repr type_repr_of(Type type)
{
    switch (type) {
    case TYPE_ANY:
        return TYPE_REPR_U64 | TYPE_REPR_I64 | TYPE_REPR_F64 | TYPE_REPR_PTR;
    case TYPE_NUMBER:
        return TYPE_REPR_U64 | TYPE_REPR_I64 | TYPE_REPR_F64;
    case TYPE_FLOAT:
        return TYPE_REPR_F64;
    case TYPE_INTEGER:
        return TYPE_REPR_U64 | TYPE_REPR_I64;
    case TYPE_SIGNED:
        return TYPE_REPR_I64;
    case TYPE_UNSIGNED:
    case TYPE_MEM_ADDR:
    case TYPE_INST_ADDR:
    case TYPE_STACK_ADDR:
    case TYPE_NATIVE_ID:
        return TYPE_REPR_U64;
    case COUNT_TYPES:
    default: {
        assert(false && "type_repr_of: unreachable");
        exit(1);
    }
    }
}

Word convert_type_reprs(Word input, Type_Repr from, Type_Repr to)
{
    (void) input;
    (void) from;
    (void) to;
    assert(false && "TODO: convert_type_reprs is not implemented");
}

Word word_u64(uint64_t u64)
{
    return (Word) {
        .as_u64 = u64
    };
}

Word word_i64(int64_t i64)
{
    return (Word) {
        .as_i64 = i64
    };
}

Word word_f64(double f64)
{
    return (Word) {
        .as_f64 = f64
    };
}

Word word_ptr(void *ptr)
{
    return (Word) {
        .as_ptr = ptr
    };
}
