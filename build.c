#include "./build.h"

#ifdef _WIN32
// Choose only one thing here
//	#define USE_MINGW
	#define USE_CL
#else
	#define USE_LINUX_GCC
#endif

#ifdef USE_CL
	#define CFLAGS "/std:c11", "/O2", "/FC", "/W4", "/WX", "/wd4996", "/nologo", "/Fe.\\build\\bin\\", "/Fo.\\build\\bin\\"
#elif defined(USE_LINUX_GCC)
	#define CFLAGS "-Wall", "-Wextra", "-Wswitch-enum", "-Wmissing-prototypes", "-Wconversion", "-pedantic", "-fno-strict-aliasing", "-ggdb", "-std=c11"
#elif defined(USE_MINGW)
	#define CFLAGS "-Wall", "-Wextra", "-Wswitch-enum", "-Wmissing-prototypes", "-Wconversion", "-pedantic", "-fno-strict-aliasing", "-ggdb", "-std=c11"
#else
	#error Choose c-compiller in defines of build.c
#endif


const char *toolchain[] = {
    "basm", "bme", "bmr", "debasm", "bdb", "basm2nasm"
};

#ifdef USE_CL
void build_c_file(const char *input_path, const char *output_path)
{
    CMD("cl.exe", CFLAGS, input_path);
} 
#elif defined(USE_LINUX_GCC)
void build_c_file(const char *input_path, const char *output_path)
{
    CMD("cc", CFLAGS, "-o", output_path, input_path);
}
#elif defined(USE_MINGW)
void build_c_file(const char *input_path, const char *output_path)
{
    CMD("gcc.exe", CFLAGS, "-o", output_path, input_path); 
} 
#else
#error Define build_c_file() function for this c-compiler
#endif 

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

void run_tests()
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

int main()
{
    build_toolchain();
    build_examples();
    run_tests();

    return 0;
}
