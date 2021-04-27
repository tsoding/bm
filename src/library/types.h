#ifndef TYPES_H_
#define TYPES_H_

#include <stdio.h>

typedef enum {
    TYPE_ANY = 0,
    TYPE_NUMBER,
    TYPE_FLOAT,
    TYPE_INTEGER,
    TYPE_SIGNED,
    TYPE_UNSIGNED,
    TYPE_MEM_ADDR,
    TYPE_INST_ADDR,
    TYPE_STACK_ADDR,
    TYPE_NATIVE_ID,
    COUNT_TYPES
} Type;

const char *type_name(Type type);
Type supertype_of(Type subtype);
void dump_type_hierarchy_as_dot(FILE *stream);

#endif // TYPES_H_
