#define NOBUILD_IMPLEMENTATION
#include "../nobuild.h"
#include "../nobuild.global.h"

#define CFLAGS COMMON_FLAGS

#define INCLUDES INCLUDE_FLAG(PATH("..", "common")), \
                 INCLUDE_FLAG(PATH("..", "bm", "src"))
#define COMMON_UNITS PATH("..", "common", "sv.c")
#define BM_UNITS PATH("..", "bm", "src", "bm.c"), \
                 PATH("..", "bm", "src", "types.c")
#define UNITS COMMON_UNITS, BM_UNITS

#ifdef _WIN32
#define LIBS ""
#else
#define LIBS "-lm"
#endif

int main()
{
    MKDIRS("bin");
    CMD("cc", CFLAGS, INCLUDES, "-o", PATH("bin", "debasm"), UNITS, PATH("src", "debasm.c"), LIBS);

    return 0;
}
