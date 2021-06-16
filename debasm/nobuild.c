#define NOBUILD_IMPLEMENTATION
#include "../nobuild.h"
#include "../nobuild.global.h"

#define CFLAGS COMMON_FLAGS

#define INCLUDES "-I../common/", \
                 "-I../bm/src/"
#define COMMON_UNITS "../common/sv.c"
#define BM_UNITS "../bm/src/bm.c", \
                 "../bm/src/types.c"
#define UNITS COMMON_UNITS, BM_UNITS

#ifdef _WIN32
#define LIBS ""
#else
#define LIBS "-lm"
#endif

int main()
{
    MKDIRS("bin");
    CMD("cc", CFLAGS, INCLUDES, "-o", "./bin/debasm", UNITS, "./src/debasm.c", LIBS);

    return 0;
}
