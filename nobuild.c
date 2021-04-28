#define NOBUILD_IMPLEMENTATION
#include "./nobuild.h"

#ifdef _WIN32
#define CFLAGS "/std:c11", "/O2", "/FC", "/W4", "/WX", "/wd4996", "/wd4200", "/wd5105", "/nologo" //, "/Fe.\\build\\bin\\", "/Fo.\\build\\bin\\"
#else
#define CFLAGS "-Wall", "-Wextra", "-Wswitch-enum", "-Wmissing-prototypes", "-Wconversion", "-Wno-missing-braces", "-pedantic", "-fno-strict-aliasing", "-ggdb", "-std=c11"
#endif

typedef struct {
    const char *name;
    const char *description;
    void (*run)(int argc, char **argv);
} Command;

void tools_command(int argc, char **argv);
void cases_command(int argc, char **argv);
void test_command(int argc, char **argv);
void record_command(int argc, char **argv);
void fmt_command(int argc, char **argv);
void help_command(int argc, char **argv);
void lib_command(int argc, char **argv);
void wrappers_command(int argc, char **argv);
void examples_command(int argc, char **argv);

Command commands[] = {
    {
        .name = "tools",
        .description = "Build toolchain",
        .run = tools_command
    },
    {
        .name = "cases",
        .description = "Build test cases",
        .run = cases_command
    },
    {
        .name = "test",
        .description = "Run the tests",
        .run = test_command
    },
    {
        .name = "record",
        .description = "Capture the current output of examples as the expected one for the tests",
        .run = record_command
    },
    {
        .name = "fmt",
        .description = "Format the source code using astyle: http://astyle.sourceforge.net/",
        .run = fmt_command
    },
    {
        .name = "help",
        .description = "Show this help message",
        .run = help_command
    },
    {
        .name = "lib",
        .description = "Build the BM library (experimental)",
        .run = lib_command,
    },
    {
        .name = "wrappers",
        .description = "Build all the C library wrappers",
        .run = wrappers_command,
    },
    {
        .name = "examples",
        .description = "Build all the examples from the ./examples/ folder",
        .run = examples_command,
    }
};
size_t commands_size = sizeof(commands) / sizeof(commands[0]);

void build_tool(const char *name)
{
#ifdef _WIN32
    CMD("cl.exe", CFLAGS,
        "/Fe.\\build\\toolchain\\",
        "/Fo.\\build\\toolchain\\",
        "/I", PATH("src", "library"),
        PATH("src", "toolchain", CONCAT(name, ".c")),
        "bm.lib",
        "/link", CONCAT("/LIBPATH:", PATH("build", "library")));
#else
    const char *cc = getenv("CC");
    if (cc == NULL) {
        cc = "cc";
    }

    CMD(cc, CFLAGS,
        "-o", PATH("build", "toolchain", name),
        "-I", PATH("src", "library"),
        "-L", PATH("build", "library"),
        PATH("src", "toolchain", CONCAT(name, ".c")),
        "-lbm",
        "-ldl");
#endif // _WIN32
}

void examples_command(int argc, char **argv)
{
    if (argc <= 0) {
        tools_command(argc, argv);
        wrappers_command(argc, argv);

        RM(PATH("build", "examples"));
        MKDIRS("build", "examples");

        FOREACH_FILE_IN_DIR(example, "examples", {
            if (ENDS_WITH(example, ".basm"))
            {
                CMD(PATH("build", "toolchain", "basm"),
                    "-I", "lib",
                    "-o", PATH("build", "examples", CONCAT(NOEXT(example), ".bm")),
                    PATH("examples", example));
            }
        });
    } else {
        const char *subcommand = shift_args(&argc, &argv);

        if (strcmp(subcommand, "run") == 0) {
            const char *example = shift_args(&argc, &argv);
            CMD(PATH("build", "toolchain", "basm"),
                "-I", "lib",
                "-o", PATH("build", "examples", CONCAT(example, ".bm")),
                PATH("examples", CONCAT(example, ".basm")));
            CMD(PATH("build", "toolchain", "bme"),
                "-n", PATH("build", "wrappers", "libbm_sdl.so"),
                PATH("build", "examples", CONCAT(example, ".bm")));
        } else if (strcmp(subcommand, "debasm") == 0) {
            const char *example = shift_args(&argc, &argv);
            CMD(PATH("build", "toolchain", "basm"),
                "-I", "lib",
                "-o", PATH("build", "examples", CONCAT(example, ".bm")),
                PATH("examples", CONCAT(example, ".basm")));
            CMD(PATH("build", "toolchain", "debasm"),
                PATH("build", "examples", CONCAT(example, ".bm")));
        } else if (strcmp(subcommand, "ast2svg") == 0) {
            MKDIRS("build", "svg");
            const char *example = shift_args(&argc, &argv);
            CHAIN(CHAIN_CMD(PATH("build", "toolchain", "basm2dot"),
                            PATH("examples", CONCAT(example, ".basm"))),
                  CHAIN_CMD("dot", "-Tsvg"),
                  OUT(PATH("build", "svg", CONCAT(example, ".svg"))));
        } else {
            fprintf(stderr, "ERROR: unknown subcommand `%s`\n", subcommand);
            exit(1);
        }
    }
}

void wrappers_command(int argc, char **argv)
{
    RM(PATH("build", "wrappers"));
    MKDIRS("build", "wrappers");

#ifndef _WIN32
    // SDL wrapper
    {
        CMD("cc", CFLAGS,
            "-c", "-fpic",
#ifdef __FreeBSD__
            // NOTE: Technically, this should be determined by pkg-config
            "-I/usr/local/include",
#endif
            "-I", PATH("src", "library"),
            "-o", PATH("build", "wrappers", "bm_sdl.o"),
            PATH("wrappers", "bm_sdl.c"));

        CMD("cc", CFLAGS,
            "-shared",
#ifdef __FreeBSD__
            // NOTE: See above
            "-L/usr/local/lib",
#endif
            "-o", PATH("build", "wrappers", "libbm_sdl.so"),
            PATH("build", "wrappers", "bm_sdl.o"),
            "-lSDL2");
    }

    // hello wrapper
    {
        CMD("cc", CFLAGS,
            "-c", "-fpic",
            "-I", PATH("src", "library"),
            "-o", PATH("build", "wrappers", "bm_hello.o"),
            PATH("wrappers", "bm_hello.c"));

        CMD("cc", CFLAGS,
            "-shared",
            "-o", PATH("build", "wrappers", "libbm_hello.so"),
            PATH("build", "wrappers", "bm_hello.o"));
    }
#else
    // TODO(#295): consider adding the SDL binary dependency to the repo
    // It is actually quite common for Windows C/C++ project to have binary blobs in the repos. So there is nothing to be ashamed about.

    // SDL wrapper
    {
        CMD("cl.exe", CFLAGS,
            "/I", PATH("vendor", "sdl2", "include"),
            "/I", PATH("src", "library"),
            PATH("wrappers", "bm_sdl.c"),
            "/link", PATH("vendor", "sdl2", "lib", "SDL2.lib"),
            "/DLL",
            CONCAT("/out:", PATH("build", "wrappers", "libbm_sdl.dll")),
            "/NOENTRY"
           );
    }

    // hello wrapper
    {
        CMD("cl.exe", CFLAGS,
            "/I", PATH("src", "library"),
            PATH("wrappers", "bm_hello.c"),
            "/link",
            "/DLL",
            "libucrt.lib",
            CONCAT("/out:", PATH("build", "wrappers", "libbm_hello.dll")),
            "/NOENTRY"
           );
    }
#endif
}

void tools_command(int argc, char **argv)
{
    lib_command(argc, argv);

    RM(PATH("build", "toolchain"));
    MKDIRS("build", "toolchain");

    FOREACH_FILE_IN_DIR(file, PATH("src", "toolchain"), {
        if (ENDS_WITH(file, ".c"))
        {
            build_tool(NOEXT(file));
        }
    });
}

void cases_command(int argc, char **argv)
{
    tools_command(argc, argv);

    RM(PATH("build", "test", "cases"));
    MKDIRS("build", "test", "cases");

    if (argc == 0 || strcmp(argv[0], "nasm") != 0) {
        FOREACH_FILE_IN_DIR(caze, PATH("test", "cases"), {
            if (ENDS_WITH(caze, ".basm"))
            {
                CMD(PATH("build", "toolchain", "basm"),
                    "-I", "./lib/",
                    PATH("test", "cases", caze),
                    "-o", PATH("build", "test", "cases", CONCAT(NOEXT(caze), ".bm")));
            }
        });
    } else {
#ifdef __linux__
        FOREACH_FILE_IN_DIR(caze, PATH("test", "cases"), {
            if (ENDS_WITH(caze, ".basm"))
            {
                CMD(PATH("build", "toolchain", "basm"),
                    "-I", "./lib/",
                    "-f", "nasm",
                    PATH("test", "cases", caze),
                    "-o", PATH("build", "test", "cases", CONCAT(NOEXT(caze), ".asm")));
                CMD("nasm", "-felf64", PATH("build", "test", "cases", CONCAT(NOEXT(caze), ".asm")), "-o", PATH("build", "test", "cases", CONCAT(NOEXT(caze), ".o")));
                CMD("ld", PATH("build", "test", "cases", CONCAT(NOEXT(caze), ".o")), "-o", PATH("build", "test", "cases", CONCAT(NOEXT(caze), ".elf")));
            }
        });
    }
#else
    assert(0 && "FIXME: right now assembly that basm produces is linux only");
#endif
}

void test_command(int argc, char **argv)
{
    cases_command(argc, argv);

    if (argc == 0 || strcmp(argv[0], "nasm") != 0) {
        FOREACH_FILE_IN_DIR(caze, PATH("test", "cases"), {
            if (ENDS_WITH(caze, ".basm"))
            {
                const char *caze_base = NOEXT(caze);
                CMD(PATH("build", "toolchain", "bmr"),
                    "-p", PATH("build", "test", "cases", CONCAT(caze_base, ".bm")),
                    "-eo", PATH("test", "outputs", CONCAT(caze_base, ".expected.out")));
            }
        });
    } else {
        assert(0 && "FIXME: implement some mechanism to test native executables");
    }
}

void record_command(int argc, char **argv)
{
    cases_command(argc, argv);

    FOREACH_FILE_IN_DIR(caze, PATH("test", "cases"), {
        if (ENDS_WITH(caze, ".basm"))
        {
            const char *case_base = NOEXT(caze);
            CMD(PATH("build", "toolchain", "bmr"),
                "-p", PATH("build", "test", "cases", CONCAT(case_base, ".bm")),
                "-ao", PATH("test", "outputs", CONCAT(case_base, ".expected.out")));
        }
    });
}

void fmt_command(int argc, char **argv)
{
    FOREACH_FILE_IN_DIR(file, PATH("src", "library"), {
        if (ENDS_WITH(file, ".c") || ENDS_WITH(file, ".h"))
        {
            const char *file_path = PATH("src", "library", file);
            CMD("astyle", "--style=kr", file_path);
        }
    });

    FOREACH_FILE_IN_DIR(file, PATH("src", "toolchain"), {
        if (ENDS_WITH(file, ".c") || ENDS_WITH(file, ".h"))
        {
            const char *file_path = PATH("src", "toolchain", file);
            CMD("astyle", "--style=kr", file_path);
        }
    });

    FOREACH_FILE_IN_DIR(file, "wrappers", {
        if (ENDS_WITH(file, ".c") || ENDS_WITH(file, ".h"))
        {
            const char *file_path = PATH("wrappers", file);
            CMD("astyle", "--style=kr", file_path);
        }
    });
}

void print_help(FILE *stream);

void help_command(int argc, char **argv)
{
    print_help(stdout);
}

void build_lib_object(const char *name)
{
#ifdef _WIN32
    CMD("cl.exe", CFLAGS, "/c",
        PATH("src", "library", CONCAT(name, ".c")),
        "/Fo.\\build\\library\\");
#else
    CMD("cc", CFLAGS, "-c",
        PATH("src", "library", CONCAT(name, ".c")),
        "-o", PATH("build", "library", CONCAT(name, ".o")));
#endif // _WIN32
}

void link_lib_objects(Cstr *objects, size_t objects_count)
{
    Cmd cmd = {0};

#ifdef _WIN32
    cmd.line = cstr_array_make(
                   "lib",
                   CONCAT("/out:", PATH("build", "library", "bm.lib")),
                   NULL);

    for (size_t i = 0; i < objects_count; ++i) {
        cmd.line = cstr_array_append(
                       cmd.line,
                       PATH("build", "library", CONCAT(objects[i], ".obj")));
    }
#else
    cmd.line = cstr_array_make(
                   "ar", "-crs",
                   PATH("build", "library", "libbm.a"),
                   NULL);

    for (size_t i = 0; i < objects_count; ++i) {
        cmd.line = cstr_array_append(
                       cmd.line,
                       PATH("build", "library", CONCAT(objects[i], ".o")));
    }
#endif // _WIN32

    INFO("CMD: %s", cmd_show(cmd));
    cmd_run_sync(cmd);
}

void lib_command(int argc, char **argv)
{
    MKDIRS("build", "library");

    size_t objects_size = 0;

    FOREACH_FILE_IN_DIR(file, PATH("src", "library"), {
        if (ENDS_WITH(file, ".c"))
        {
            objects_size += 1;
            build_lib_object(NOEXT(file));
        }
    });

    Cstr *objects = malloc(sizeof(Cstr) * objects_size);
    objects_size = 0;
    FOREACH_FILE_IN_DIR(file, PATH("src", "library"), {
        if (ENDS_WITH(file, ".c"))
        {
            objects[objects_size++] = NOEXT(file);
        }
    });

    link_lib_objects(objects, objects_size);
}

void print_help(FILE *stream)
{
    int longest = 0;

    for (size_t i = 0; i < commands_size; ++i) {
        int n = strlen(commands[i].name);
        if (n > longest) {
            longest = n;
        }
    }

    fprintf(stream, "Usage:\n");
    fprintf(stream, "  Available subcommands:\n");
    for (size_t i = 0; i < commands_size; ++i) {
        fprintf(stream, "    ./nobuild %s%*s - %s\n",
                commands[i].name,
                (int) (longest - strlen(commands[i].name)), "",
                commands[i].description);
    }

    fprintf(stream,
            "  You can provide several subcommands like this (they will be executed sequentially):\n"
            "    ./nobuild build examples test\n");
}

Command *find_command_by_name(const char *command_name)
{
    for (size_t i = 0; i < commands_size; ++i) {
        if (strcmp(command_name, commands[i].name) == 0) {
            return &commands[i];
        }
    }

    return NULL;
}

int main(int argc, char **argv)
{
    shift_args(&argc, &argv);        // skip program

    if (argc == 0) {
        print_help(stderr);
        fprintf(stderr, "ERROR: no commands were provided\n");
        exit(1);
    }

    const char *command_name = shift_args(&argc, &argv);

    Command *command = find_command_by_name(command_name);
    if (command != NULL) {
        command->run(argc, argv);
    } else {
        print_help(stderr);
        fprintf(stderr, "ERROR: Command `%s` does not exist\n", command_name);
        exit(1);
    }

    return 0;
}
