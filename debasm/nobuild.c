#define NOBUILD_IMPLEMENTATION
#include "../nobuild.h"
#include "../nobuild.global.h"

#define INCLUDES "-I../common/", \
                 "-I../bm/src/"
#define COMMON_UNITS "../common/sv.c"
#define BM_UNITS "../bm/src/bm.c", \
                 "../bm/src/types.c"
#define UNITS COMMON_UNITS, BM_UNITS
#define LIBS "-lm"

int main()
{
    MKDIRS("bin");
    CMD("cc", CFLAGS, INCLUDES, "-o", "./bin/debasm", UNITS, "./src/debasm.c", LIBS);

    return 0;
}
