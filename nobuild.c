#define NOBUILD_IMPLEMENTATION
#include "./nobuild.h"
#include "./nobuild.global.h"

static const char *const subdirs[] = {
    "bm", "debasm", "bdb", "basm", "bang"
};
static const size_t subdirs_count = sizeof(subdirs) / sizeof(subdirs[0]);

// TODO(#452): the root nobuild does not have the fmt subcommand

int main(int argc, char **argv)
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
        for (int i = 1; i < argc; ++i) {
            nobuild_args = cstr_array_append(nobuild_args, argv[i]);
        }
        Cmd nobuild_cmd = {nobuild_args};
        INFO("CMD: %s", cmd_show(nobuild_cmd));
        cmd_run_sync(nobuild_cmd);

        chdir("..");
    }

    return 0;
}
