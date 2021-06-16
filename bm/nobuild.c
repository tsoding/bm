#define NOBUILD_IMPLEMENTATION
#include "../nobuild.h"
#include "../nobuild.global.h"

#define INCLUDES     INCLUDE_FLAG(PATH("..", "common"))
#define CFLAGS       COMMON_FLAGS, \
                     INCLUDES
#define COMMON_UNITS "../common/sv.c", \
                     "../common/arena.c"
#define BM_UNITS     "./src/bm.c", \
                     "./src/native_loader.c", \
                     "./src/types.c"
#define UNITS        COMMON_UNITS, \
                     BM_UNITS
#ifdef _WIN32
#define LIBS
#else
#define LIBS         "-ldl", "-lm"
#endif // _WIN32

int main()
{
    MKDIRS("bin");
    CC("bin", "bme", "./src/bme.c");
    CC("bin", "bmr", "./src/bmr.c");

    return 0;
}
