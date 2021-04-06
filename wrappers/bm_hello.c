#include <stdio.h>

#include "bm.h"

EXPORT Err bm_hello(Bm *bm);

Err bm_hello(Bm *bm)
{
    (void) bm;
    printf("hello\n");
    return ERR_OK;
}
