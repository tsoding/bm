#define NOBUILD_IMPLEMENTATION
#include "./nobuild.h"
#include "./nobuild.global.h"

static const char *const subdirs[] = {
    "bm", "debasm", "bdb", "basm", "bang"
};
static const size_t subdirs_count = sizeof(subdirs) / sizeof(subdirs[0]);

// TODO: the root nobuild does not have the fmt subcommand

int main()
{
    for (size_t i = 0; i < subdirs_count; ++i) {
        INFO("------------------------------");
        INFO("SUBDIR: %s", subdirs[i]);
        INFO("------------------------------");
        chdir(subdirs[i]);
#ifdef _WIN32
        CMD("cl.exe", "nobuild.c");
        CMD(".\\nobuild.exe");
#else
        CMD("cc", "-o", "nobuild", "nobuild.c");
        CMD("./nobuild");
#endif
        chdir("..");
    }

    return 0;
}
