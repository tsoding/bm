#ifndef VERIFIER_H_
#define VERIFIER_H_

#include "./types.h"
#include "./bm.h"
#include "./compiler.h"

typedef struct {
    Type type;
    File_Location location;
} Frame;

typedef struct {
    Frame stack[BM_STACK_CAPACITY];
    size_t stack_size;
} Verifier;

bool verifier_push_frame(Verifier *verifier, Type type, File_Location location);
bool verifier_pop_frame(Verifier *verifier, Frame *output);

void verifier_verify(Verifier *verifier, const Basm *basm);

#endif // VERIFIER_H_
