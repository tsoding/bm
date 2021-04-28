#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "./types.h"

bool type_by_name(const char *name, Type *output_type)
{
    for (Type type = 0; type < COUNT_TYPES; ++type) {
        if (strcmp(name, type_name(type)) == 0) {
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
