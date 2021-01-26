#include "./build.h"

#ifdef _WIN32
#define CFLAGS "/std:c11", "/O2", "/FC", "/W4", "/WX", "/wd4996", "/nologo", "/Fe.\\build\\bin\\", "/Fo.\\build\\bin\\"
#else
#define CFLAGS "-Wall", "-Wextra", "-Wswitch-enum", "-Wmissing-prototypes", "-Wconversion", "-pedantic", "-fno-strict-aliasing", "-ggdb", "-std=c11"
#endif

const char *toolchain[] = {
    "basm", "bme", "bmr", "debasm", "bdb", "basm2nasm"
};

#ifdef _WIN32
void build_c_file(const char *input_path, const char *output_path)
{
    CMD("cl.exe", CFLAGS, input_path);
}
#else
void build_c_file(const char *input_path, const char *output_path)
{
    CMD("cc", CFLAGS, "-o", output_path, input_path);
}
#endif // WIN32

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
                CMD(PATH("build", "bin", "basm"),
                    "-g",
                    PATH("examples", example),
                    PATH("build", CONCAT(example, ".bm")));
            }
        }
    });
}

int main()
{
    build_toolchain();
    // build_examples();

    return 0;
}
