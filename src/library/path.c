#ifndef _WIN32
#    define _DEFAULT_SOURCE
#    define _POSIX_C_SOURCE 200112L
#    include <sys/types.h>
#    include <sys/stat.h>
#    include <unistd.h>
#else
#    define  WIN32_LEAN_AND_MEAN
#    include "windows.h"
#endif // _WIN32

#include <assert.h>
#include <string.h>
#include "./path.h"

static bool is_path_sep(char x)
{
#ifdef _WIN32
    return x == '/' || x == '\\';
#else
    return x == '/';
#endif
}

static String_View dot_guard(String_View name)
{
    if (sv_eq(name, SV(".")) || sv_eq(name, SV(".."))) {
        return SV("");
    }

    return name;
}

String_View file_name_of_path(const char *begin)
{
    if (begin == NULL) {
        return SV_NULL;
    }

    // ./foooooo/hello.basm
    // ^         ^    ^
    // begin     sep  dot

    const size_t n = strlen(begin);
    const char *dot = begin + n - 1;

    while (begin <= dot && *dot != '.' && !is_path_sep(*dot)) {
        dot -= 1;
    }

    if (begin > dot) {
        return dot_guard(sv_from_cstr(begin));
    }

    if (is_path_sep(*dot)) {
        return dot_guard(sv_from_cstr(dot + 1));
    }

    const char *sep = dot;

    while (begin <= sep && !is_path_sep(*sep)) {
        sep -= 1;
    }
    sep += 1;

    assert(sep <= dot);
    return dot_guard((String_View) {
        .count = (size_t) (dot - sep),
        .data = sep,
    });
}

String_View path_join(Arena *arena, String_View base, String_View file_path)
{
#ifndef _WIN32
    const String_View sep = sv_from_cstr("/");
#else
    const String_View sep = sv_from_cstr("\\");
#endif // _WIN32
    const size_t result_size = base.count + sep.count + file_path.count;
    char *result = arena_alloc(arena, result_size);
    assert(result);

    {
        char *append = result;

        memcpy(append, base.data, base.count);
        append += base.count;

        memcpy(append, sep.data, sep.count);
        append += sep.count;

        memcpy(append, file_path.data, file_path.count);
        append += file_path.count;
    }

    return (String_View) {
        .count = result_size,
        .data = result,
    };
}

bool path_file_exist(const char *file_path)
{
#ifndef _WIN32
    struct stat statbuf = {0};
    return stat(file_path, &statbuf) == 0 &&
           ((statbuf.st_mode & S_IFMT) == S_IFREG);
#else
    // https://stackoverflow.com/a/6218957
    DWORD dwAttrib = GetFileAttributes(file_path);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
            !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#endif
}
