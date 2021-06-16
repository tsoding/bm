// Global nobuild definitions that are reusable among the all subprojects
#ifndef NOBUILD_GLOBAL_H_
#define NOBUILD_GLOBAL_H_

#ifdef _WIN32
#define COMMON_FLAGS "/std:c11", "/O2", "/FC", "/W4", "/WX", "/wd4996", "/wd4200", "/wd5105", "/nologo"
#else
#define COMMON_FLAGS "-Wall", "-Wextra", "-Wswitch-enum", "-Wmissing-prototypes", "-Wconversion", "-Wno-missing-braces", "-pedantic", "-fno-strict-aliasing", "-ggdb", "-std=c11"
#endif

#define CC(out_path, entry_unit) \
    do { \
        const char *cc = getenv("CC"); \
        if (cc == NULL) { \
            cc = "cc"; \
        } \
        CMD(cc, CFLAGS, INCLUDES, "-o", out_path, UNITS, entry_unit, LIBS); \
    } while(0)

#endif // NOBUILD_GLOBAL_H_
