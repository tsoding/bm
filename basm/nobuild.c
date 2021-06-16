#define NOBUILD_IMPLEMENTATION
#include "../nobuild.h"
#include "../nobuild.global.h"

#define CFLAGS COMMON_FLAGS

#define INCLUDES INCLUDE_FLAG(PATH("..", "common")), \
                 INCLUDE_FLAG(PATH("..", "bm", "src"))

#define COMMON_UNITS PATH("..", "common", "sv.c"), \
                     PATH("..", "common", "arena.c"), \
                     PATH("..", "common", "path.c")

#define BM_UNITS     PATH("..", "bm", "src", "types.c"), \
                     PATH("..", "bm", "src", "bm.c")

#define BASM_UNITS   PATH("src", "compiler.c"), \
                     PATH("src", "expr.c"), \
                     PATH("src", "fl.c"), \
                     PATH("src", "gas_arm64.c"), \
                     PATH("src", "linizer.c"), \
                     PATH("src", "nasm_sysv_x86_64.c"), \
                     PATH("src", "statement.c"), \
                     PATH("src", "target.c"), \
                     PATH("src", "tokenizer.c"), \
                     PATH("src", "verifier.c")

#define UNITS COMMON_UNITS, BM_UNITS, BASM_UNITS

#define LIBS "-lm"

int main()
{
    MKDIRS("bin");
    CC("bin", "basm", PATH("src", "basm.c"));
    CC("bin", "basm2dot", PATH("src", "basm2dot.c"));
    CC("bin", "expr2dot", PATH("src", "expr2dot.c"));

    // TODO: test is not implemented

    return 0;
}
