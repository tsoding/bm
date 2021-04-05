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
    void (*run)(void);
} Command;

void tools_command(void);
void cases_command(void);
void test_command(void);
void record_command(void);
void fmt_command(void);
void help_command(void);
void lib_command(void);
void wrappers_command(void);
void examples_command(void);

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

void examples_command(void)
{
    tools_command();
    wrappers_command();

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
}

void wrappers_command(void)
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
    PANIC("TODO: build C wrappers is not implemented for Windows");
#endif
}

void tools_command(void)
{
    lib_command();

    RM(PATH("build", "toolchain"));
    MKDIRS("build", "toolchain");

    FOREACH_FILE_IN_DIR(file, PATH("src", "toolchain"), {
        if (ENDS_WITH(file, ".c"))
        {
            build_tool(NOEXT(file));
        }
    });
}

void cases_command(void)
{
    tools_command();

    RM(PATH("build", "test", "cases"));
    MKDIRS("build", "test", "cases");

    FOREACH_FILE_IN_DIR(caze, PATH("test", "cases"), {
        if (ENDS_WITH(caze, ".basm"))
        {
            CMD(PATH("build", "toolchain", "basm"),
                "-I", "./lib/",
                PATH("test", "cases", caze),
                "-o", PATH("build", "test", "cases", CONCAT(NOEXT(caze), ".bm")));
        }
    });
}

void build_x86_64_example(const char *example)
{
    CMD(PATH("build", "bin", "basm2nasm"),
        PATH("examples", CONCAT(example, ".basm")),
        PATH("build", "examples", CONCAT(example, ".asm")));

    CMD("nasm", "-felf64", "-F", "dwarf", "-g",
        PATH("build", "examples", CONCAT(example, ".asm")),
        "-o",
        PATH("build", "examples", CONCAT(example, ".o")));

    CMD("ld",
        "-o", PATH("build", "examples", CONCAT(example, ".exe")),
        PATH("build", "examples", CONCAT(example, ".o")));
}

void test_command(void)
{
    cases_command();

    FOREACH_FILE_IN_DIR(caze, PATH("test", "cases"), {
        if (ENDS_WITH(caze, ".basm"))
        {
            const char *caze_base = NOEXT(caze);
            CMD(PATH("build", "toolchain", "bmr"),
                "-p", PATH("build", "test", "cases", CONCAT(caze_base, ".bm")),
                "-eo", PATH("test", "outputs", CONCAT(caze_base, ".expected.out")));
        }
    });
}

void record_command(void)
{
    cases_command();

    FOREACH_FILE_IN_DIR(example, "examples", {
        if (ENDS_WITH(example, ".basm"))
        {
            const char *example_base = NOEXT(example);
            CMD(PATH("build", "toolchain", "bmr"),
                "-p", PATH("build", "examples", CONCAT(example_base, ".bm")),
                "-ao", PATH("test", "examples", CONCAT(example_base, ".expected.out")));
        }
    });
}

void fmt_command(void)
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

void help_command(void)
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

void lib_command(void)
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

void run_command_by_name(const char *command_name)
{
    for (size_t i = 0; i < commands_size; ++i) {
        if (strcmp(command_name, commands[i].name) == 0) {
            commands[i].run();
            return;
        }
    }

    print_help(stderr);
    fprintf(stderr, "Could not run command `%s`. Such command does not exist!\n", command_name);
    exit(1);
}

int main(int argc, char **argv)
{
    shift_args(&argc, &argv);        // skip program

    if (argc == 0) {
        print_help(stderr);
        fprintf(stderr, "ERROR: no commands were provided\n");
        exit(1);
    }

    run_command_by_name(argv[0]);

    return 0;
}
