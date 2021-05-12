#ifndef VERIFIER_H_
#define VERIFIER_H_

#include "./types.h"
#include "./bm.h"
#include "./basm.h"

typedef struct {
    Type types[BM_STACK_CAPACITY];
    size_t types_size;
} Verifier;

bool verifier_push_type(Verifier *verifier, Type type);
bool verifier_pop_type(Verifier *verifier, Type *output);

void verifier_verify(Verifier *verifier, const Basm *basm);

#endif // VERIFIER_H_
