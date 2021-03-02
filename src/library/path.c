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

