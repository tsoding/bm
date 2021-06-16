#define NOBUILD_IMPLEMENTATION
#include "../nobuild.h"
#include "../nobuild.global.h"

#define INCLUDES     INCLUDE_FLAG(PATH("..", "common"))
#define CFLAGS       COMMON_FLAGS
#define COMMON_UNITS PATH("..", "common", "sv.c"), \
                     PATH("..", "common", "arena.c")
#define BM_UNITS     PATH("src", "bm.c"), \
                     PATH("src", "native_loader.c"), \
                     PATH("src", "types.c")
#define UNITS        COMMON_UNITS, \
                     BM_UNITS
#ifdef _WIN32
#define LIBS
#else
#define LIBS         "-ldl", "-lm"
#endif // _WIN32

int main(int argc, char **argv)
{
    if (argc >= 2 && strcmp(argv[1], "fmt") == 0) {
        astyle_format_folder("src");
    } else {
        MKDIRS("bin");
        CC("bin", "bme", PATH("src", "bme.c"));
        CC("bin", "bmr", PATH("src", "bmr.c"));
    }

    return 0;
}
