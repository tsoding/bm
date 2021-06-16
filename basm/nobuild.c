#define NOBUILD_IMPLEMENTATION
#include "../nobuild.h"
#include "../nobuild.global.h"

#define INCLUDES "-I../common/", \
                 "-I../bm/src/"

#define COMMON_UNITS "../common/sv.c", \
                     "../common/arena.c", \
                     "../common/path.c"

#define BM_UNITS     "../bm/src/types.c", \
                     "../bm/src/bm.c"

#define BASM_UNITS   "./src/compiler.c", \
                     "./src/expr.c", \
                     "./src/fl.c", \
                     "./src/gas_arm64.c", \
                     "./src/linizer.c", \
                     "./src/nasm_sysv_x86_64.c", \
                     "./src/statement.c", \
                     "./src/target.c", \
                     "./src/tokenizer.c", \
                     "./src/verifier.c"

#define UNITS COMMON_UNITS, BM_UNITS, BASM_UNITS

#define LIBS "-lm"

int main()
{
    MKDIRS("bin");
    CMD("cc", CFLAGS, INCLUDES, "-o", "./bin/basm", UNITS, "./src/basm.c", LIBS);
    CMD("cc", CFLAGS, INCLUDES, "-o", "./bin/basm2dot", UNITS, "./src/basm2dot.c", LIBS);
    CMD("cc", CFLAGS, INCLUDES, "-o", "./bin/expr2dot", UNITS, "./src/expr2dot.c", LIBS);

    // TODO: test is not implemented
    return 0;
}
