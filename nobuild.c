#define NOBUILD_IMPLEMENTATION
#include "./nobuild.h"
#include "./nobuild.global.h"

static const char *const subdirs[] = {
    "bm", "debasm", "bdb", "basm", "bang"
};
static const size_t subdirs_count = sizeof(subdirs) / sizeof(subdirs[0]);

// TODO(#452): the root nobuild does not have the fmt subcommand

void nobuild_all_subdirs(int argc, char **argv)
{
    for (size_t i = 0; i < subdirs_count; ++i) {
        INFO("------------------------------");
        INFO("SUBDIR: %s", subdirs[i]);
        INFO("------------------------------");
        chdir(subdirs[i]);
#ifdef _WIN32
        CMD("cl.exe", "nobuild.c");
        Cstr_Array nobuild_args = cstr_array_make(".\\nobuild.exe", NULL);
#else
        CMD("cc", "-o", "nobuild", "nobuild.c");
        Cstr_Array nobuild_args = cstr_array_make("./nobuild", NULL);
#endif
        for (int i = 0; i < argc; ++i) {
            nobuild_args = cstr_array_append(nobuild_args, argv[i]);
        }
        Cmd nobuild_cmd = {nobuild_args};
        INFO("CMD: %s", cmd_show(nobuild_cmd));
        cmd_run_sync(nobuild_cmd);

        chdir("..");
    }
}

int main(int argc, char **argv)
{
    assert(argc > 0);

    if (argc >= 2 && strcmp(argv[1], "help") == 0) {
        printf("Usage: ./nobuild [ARGS]\n");
        printf("This is the root nobuild file of the project. It simply iterates over all of the subprojects. Builds their corresponding nobuild.c files. And runs them with the provided [ARGS]. The subprojects are\n");
        for (size_t i = 0; i < subdirs_count; ++i) {
            printf("  %s\n", subdirs[i]);
        }

    } else {
        nobuild_all_subdirs(argc - 1, argv + 1);
    }

    return 0;
}
