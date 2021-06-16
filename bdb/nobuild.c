#define NOBUILD_IMPLEMENTATION
#include "../nobuild.h"
#include "../nobuild.global.h"

#define CFLAGS COMMON_FLAGS

#define INCLUDES INCLUDE_FLAG(PATH("..", "common")), \
                 INCLUDE_FLAG(PATH("..", "bm", "src"))
#define COMMON_UNITS PATH("..", "common", "sv.c"), \
                     PATH("..", "common", "arena.c")
#define BM_UNITS PATH("..", "bm", "src", "bm.c"), \
                 PATH("..", "bm", "src", "types.c")
#define UNITS COMMON_UNITS, BM_UNITS
#define LIBS "-lm"

int main()
{
    MKDIRS("bin");
    CC("bin", "bdb", PATH("src", "bdb.c"));

    return 0;
}
