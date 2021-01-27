#include "./nobuild.h"

#ifdef _WIN32
#define CFLAGS "/std:c11", "/O2", "/FC", "/W4", "/WX", "/wd4996", "/nologo", "/Fe.\\build\\bin\\", "/Fo.\\build\\bin\\"
#else
#define CFLAGS "-Wall", "-Wextra", "-Wswitch-enum", "-Wmissing-prototypes", "-Wconversion", "-pedantic", "-fno-strict-aliasing", "-ggdb", "-std=c11"
#endif

const char *toolchain[] = {
    "basm", "bme", "bmr", "debasm", "bdb", "basm2nasm"
};

void build_c_file(const char *input_path, const char *output_path)
{
#ifdef _WIN32
    CMD("cl.exe", CFLAGS, input_path);
#else
    CMD("cc", CFLAGS, "-o", output_path, input_path);
#endif // WIN32
}

void build_toolchain(void)
{
    MKDIRS("build", "bin");

    FOREACH_ARRAY(const char *, tool, toolchain, {
        build_c_file(PATH("src", CONCAT(tool, ".c")),
                     PATH("build", "bin", tool));
    });
}

void build_examples(void)
{
    MKDIRS("build", "examples");

    FOREACH_FILE_IN_DIR(example, "examples", {
        size_t n = strlen(example);
        if (*example != '.') {
            assert(n >= 4);
            if (strcmp(example + n - 4, "basm") == 0) {
                const char *example_base = remove_ext(example);
                CMD(PATH("build", "bin", "basm"),
                    "-g",
                    PATH("examples", example),
                    PATH("build", "examples", CONCAT(example_base, ".bm")));
            }
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

void build_x86_64_examples(void)
{
    build_x86_64_example("123i");
    build_x86_64_example("fib");
}

void run_tests(void)
{
    FOREACH_FILE_IN_DIR(example, "examples", {
        size_t n = strlen(example);
        if (*example != '.') {
            assert(n >= 4);
            if (strcmp(example + n - 4, "basm") == 0) {
                const char *example_base = remove_ext(example);
                CMD(PATH("build", "bin", "bmr"),
                    "-p", PATH("build", "examples", CONCAT(example_base, ".bm")),
                    "-eo", PATH("test", "examples", CONCAT(example_base, ".expected.out")));
            }
        }
    });
}

void record_tests(void)
{
    FOREACH_FILE_IN_DIR(example, "examples", {
        size_t n = strlen(example);
        if (*example != '.') {
            assert(n >= 4);
            if (strcmp(example + n - 4, "basm") == 0) {
                const char *example_base = remove_ext(example);
                CMD(PATH("build", "bin", "bmr"),
                    "-p", PATH("build", "examples", CONCAT(example_base, ".bm")),
                    "-ao", PATH("test", "examples", CONCAT(example_base, ".expected.out")));
            }
        }
    });
}

void print_help(FILE *stream)
{
    fprintf(stream, "./nobuild          - Build toolchain and examples\n");
    fprintf(stream, "./nobuild test     - Run the tests\n");
    fprintf(stream, "./nobuild record   - Capture the current output of examples as the expected on for the tests\n");
    fprintf(stream, "./nobuild help     - Show this help message\n");
}

int main(int argc, char **argv)
{
    shift(&argc, &argv);

    const char *subcommand = NULL;

    if (argc > 0) {
        subcommand = shift(&argc, &argv);
    }

    if (subcommand != NULL && strcmp(subcommand, "help") == 0) {
        print_help(stdout);
        exit(0);
    }

    build_toolchain();
    build_examples();
#ifdef __linux__
    build_x86_64_examples();
#endif // __linux__

    if (subcommand) {
        if (strcmp(subcommand, "test") == 0) {
            run_tests();
        } else if (strcmp(subcommand, "record") == 0) {
            record_tests();
        } else {
            print_help(stderr);
            fprintf(stderr, "[ERROR] unknown subcommand `%s`\n", subcommand);
            exit(1);
        }
    }

    return 0;
}
