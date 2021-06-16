#define NOBUILD_IMPLEMENTATION
#include "../nobuild.h"
#include "../nobuild.global.h"

#define INCLUDES "-I../common/", \
                 "-I../bm/src/", \
                 "-I../basm/src/"
#define COMMON_UNITS "../common/sv.c", \
                     "../common/arena.c", \
                     "../common/path.c"
#define BM_UNITS   "../bm/src/types.c", \
                   "../bm/src/bm.c"
#define BASM_UNITS "../basm/src/compiler.c", \
                   "../basm/src/expr.c", \
                   "../basm/src/fl.c", \
                   "../basm/src/gas_arm64.c", \
                   "../basm/src/linizer.c", \
                   "../basm/src/nasm_sysv_x86_64.c", \
                   "../basm/src/statement.c", \
                   "../basm/src/target.c", \
                   "../basm/src/tokenizer.c", \
                   "../basm/src/verifier.c"
#define BANG_UNITS "./src/bang.c", \
                   "./src/bang_compiler.c", \
                   "./src/bang_lexer.c", \
                   "./src/bang_parser.c"
#define UNITS BM_UNITS, COMMON_UNITS, BASM_UNITS, BANG_UNITS
#define LIBS "-lm"

int main()
{
    MKDIRS("bin");
    CMD("cc", CFLAGS, INCLUDES, "-o", "bin/bang", UNITS, LIBS);
    return 0;
}
