// Global nobuild definitions that are reusable among the all subprojects
#ifndef NOBUILD_GLOBAL_H_
#define NOBUILD_GLOBAL_H_

#ifdef _WIN32
#define COMMON_FLAGS "/std:c11", "/O2", "/FC", "/W4", "/WX", "/wd4996", "/wd4200", "/wd5105", "/nologo"
#else
#define COMMON_FLAGS "-Wall", "-Wextra", "-Wswitch-enum", "-Wmissing-prototypes", "-Wconversion", "-Wno-missing-braces", "-pedantic", "-fno-strict-aliasing", "-ggdb", "-std=c11"
#endif

#ifdef _WIN32
#define INCLUDE_FLAG(path) "/I", (path)
#define CC(out_dir, out_name, entry_unit) \
    do { \
        CMD("cl.exe", CFLAGS, \
            CONCAT("/Fe.\\", PATH(out_dir, CONCAT(out_name, ".exe"))), \
            CONCAT("/Fo.\\", out_dir, "\\"), \
            INCLUDES, \
            UNITS, \
            entry_unit); \
    } while(0)
#else
#define INCLUDE_FLAG(path) CONCAT("-I", (path))
#define CC(out_dir, out_name, entry_unit) \
    do { \
        const char *cc = getenv("CC"); \
        if (cc == NULL) { \
            cc = "cc"; \
        } \
        CMD(cc, CFLAGS, INCLUDES, "-o", PATH(out_dir, out_name), UNITS, entry_unit, LIBS); \
    } while(0)
#endif // _WIN32

// CMD(cc(), CFLAGS,
//     "/Fe.\\build\\toolchain\\",
//     "/Fo.\\build\\toolchain\\",
//     "/I", PATH("src", "library"),
//     PATH("src", "toolchain", CONCAT(name, ".c")),
//     "bm.lib",
//     "/link", CONCAT("/LIBPATH:", PATH("build", "library")));

#endif // NOBUILD_GLOBAL_H_
