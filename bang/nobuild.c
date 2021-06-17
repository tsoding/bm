#define NOBUILD_IMPLEMENTATION
#include "../nobuild.h"
#include "../nobuild.global.h"

#define CFLAGS COMMON_FLAGS

#define INCLUDES INCLUDE_FLAG(PATH("..", "common")), \
                 INCLUDE_FLAG(PATH("..", "bm", "src")), \
                 INCLUDE_FLAG(PATH("..", "basm", "src"))
#define COMMON_UNITS PATH("..", "common", "sv.c"), \
                     PATH("..", "common", "arena.c"), \
                     PATH("..", "common", "path.c")
#define BM_UNITS   PATH("..", "bm", "src", "types.c"), \
                   PATH("..", "bm", "src", "bm.c")
#define BASM_UNITS PATH("..", "basm", "src", "compiler.c"), \
                   PATH("..", "basm", "src", "expr.c"), \
                   PATH("..", "basm", "src", "fl.c"), \
                   PATH("..", "basm", "src", "gas_arm64.c"), \
                   PATH("..", "basm", "src", "linizer.c"), \
                   PATH("..", "basm", "src", "nasm_sysv_x86_64.c"), \
                   PATH("..", "basm", "src", "statement.c"), \
                   PATH("..", "basm", "src", "target.c"), \
                   PATH("..", "basm", "src", "tokenizer.c"), \
                   PATH("..", "basm", "src", "verifier.c")
#define BANG_UNITS PATH("src", "bang_compiler.c"), \
                   PATH("src", "bang_lexer.c"), \
                   PATH("src", "bang_parser.c")
#define UNITS BM_UNITS, COMMON_UNITS, BASM_UNITS, BANG_UNITS
#define LIBS "-lm"

int main()
{
    // TODO: there is no testing in Bang subproject
    MKDIRS("bin");
    CC("bin", "bang", PATH("src", "bang.c"));
    return 0;
}
