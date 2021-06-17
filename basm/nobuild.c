#include <stdbool.h>

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

void build_all_bins(void)
{
    MKDIRS("bin");
    CC("bin", "basm", PATH("src", "basm.c"));
    CC("bin", "basm2dot", PATH("src", "basm2dot.c"));
    CC("bin", "expr2dot", PATH("src", "expr2dot.c"));
}

void basm_test(bool record)
{
    build_all_bins();

    const char *bmr_path = PATH("..", "bm", "bin", "bmr");

    if (!PATH_EXISTS(bmr_path)) {
        PANIC("Could not find `%s` please go to the bm subproject and build it",
              bmr_path);
    }

    MKDIRS("bin", "test", "cases");
    FOREACH_FILE_IN_DIR(example, PATH("test", "cases"), {
        if (ENDS_WITH(example, ".basm"))
        {
            const char *expected_output_path = PATH("test", "outputs", CONCAT(NOEXT(example), ".expected.out"));
            const char *bm_path = PATH("bin", "test", "cases", CONCAT(NOEXT(example), ".bm"));

            CMD(PATH("bin", "basm"),
                "-I", PATH("lib"),
                "-o", bm_path,
                PATH("test", "cases", example));

            CMD(bmr_path,
                "-p", bm_path,
                record ? "-ao" : "-eo", expected_output_path);
        }
    });
}

int main(int argc, char **argv)
{
    if (argc >= 2) {
        if (strcmp(argv[1], "test") == 0) {
            basm_test(false);
        } else if (strcmp(argv[1], "record") == 0) {
            basm_test(true);
        } else {
            PANIC("unknown subcommand `%s`", argv[1]);
        }
    } else {
        build_all_bins();
    }

    return 0;
}
