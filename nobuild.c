#define NOBUILD_IMPLEMENTATION
#include "./nobuild.h"
#include "./nobuild.global.h"

void run_nobuild(void)
{
    CMD("cc", CFLAGS, "-o", "nobuild", "nobuild.c");
    CMD("./nobuild");
}

int main()
{
    INFO("------------------------------");
    INFO("bm");
    INFO("------------------------------");
    chdir("./bm/");
    run_nobuild();
    chdir("..");

    INFO("------------------------------");
    INFO("bdb");
    INFO("------------------------------");
    chdir("./bdb/");
    run_nobuild();
    chdir("..");

    INFO("------------------------------");
    INFO("basm");
    INFO("------------------------------");
    chdir("./basm/");
    run_nobuild();
    chdir("..");

    INFO("------------------------------");
    INFO("debasm");
    INFO("------------------------------");
    chdir("./debasm/");
    run_nobuild();
    chdir("..");

    INFO("------------------------------");
    INFO("bang");
    INFO("------------------------------");
    chdir("./bang/");
    run_nobuild();
    chdir("..");

    return 0;
}
