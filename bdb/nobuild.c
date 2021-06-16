#define NOBUILD_IMPLEMENTATION
#include "../nobuild.h"
#include "../nobuild.global.h"

#define INCLUDES "-I../common/", \
                 "-I../bm/src/"
#define COMMON_UNITS "../common/sv.c", \
                     "../common/arena.c"
#define BM_UNITS "../bm/src/bm.c", "../bm/src/types.c"
#define UNITS COMMON_UNITS, BM_UNITS
#define LIBS "-lm"

int main()
{
    // TODO: bdb subproject build is not crossplatform
    MKDIRS("bin");
    CMD("cc", CFLAGS, INCLUDES, "-o", "./bin/bdb", "./src/bdb.c", UNITS, LIBS);

    return 0;
}
