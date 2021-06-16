#include <string.h>
#include <ctype.h>

#include "./sv.h"

String_View sv_from_cstr(const char *cstr)
{
    return (String_View) {
        .count = strlen(cstr),
        .data = cstr,
    };
}

String_View sv_trim_left(String_View sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[i])) {
        i += 1;
    }

    return (String_View) {
        .count = sv.count - i,
        .data = sv.data + i,
    };
}

String_View sv_trim_right(String_View sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[sv.count - 1 - i])) {
        i += 1;
    }

    return (String_View) {
        .count = sv.count - i,
        .data = sv.data
    };
}

String_View sv_trim(String_View sv)
{
    return sv_trim_right(sv_trim_left(sv));
}

String_View sv_chop_left(String_View *sv, size_t n)
{
    if (n > sv->count) {
        n = sv->count;
    }

    String_View result = {
        .data = sv->data,
        .count = n,
    };

    sv->data  += n;
    sv->count -= n;

    return result;
}

String_View sv_chop_right(String_View *sv, size_t n)
{
    if (n > sv->count) {
        n = sv->count;
    }

    String_View result = {
        .data = sv->data + sv->count - n,
        .count = n
    };

    sv->count -= n;

    return result;
}

bool sv_index_of(String_View sv, char c, size_t *index)
{
    size_t i = 0;
    while (i < sv.count && sv.data[i] != c) {
        i += 1;
    }

    if (i < sv.count) {
        *index = i;
        return true;
    } else {
        return false;
    }
}

String_View sv_chop_by_delim(String_View *sv, char delim)
{
    size_t i = 0;
    while (i < sv->count && sv->data[i] != delim) {
        i += 1;
    }

    String_View result = {
        .count = i,
        .data = sv->data,
    };

    if (i < sv->count) {
        sv->count -= i + 1;
        sv->data  += i + 1;
    } else {
        sv->count -= i;
        sv->data  += i;
    }

    return result;
}

bool sv_starts_with(String_View sv, String_View expected_prefix)
{
    if (expected_prefix.count <= sv.count) {
        String_View actual_prefix = {
            .data = sv.data,
            .count = expected_prefix.count,
        };

        return sv_eq(expected_prefix, actual_prefix);
    }

    return false;
}

bool sv_ends_with(String_View sv, String_View expected_suffix)
{
    if (expected_suffix.count <= sv.count) {
        String_View actual_suffix = {
            .data = sv.data + sv.count - expected_suffix.count,
            .count = expected_suffix.count
        };

        return sv_eq(expected_suffix, actual_suffix);
    }

    return false;
}

bool sv_eq(String_View a, String_View b)
{
    if (a.count != b.count) {
        return false;
    } else {
        return memcmp(a.data, b.data, a.count) == 0;
    }
}

uint64_t sv_to_u64(String_View sv)
{
    uint64_t result = 0;

    for (size_t i = 0; i < sv.count && isdigit(sv.data[i]); ++i) {
        result = result * 10 + (uint64_t) sv.data[i] - '0';
    }

    return result;
}

String_View sv_chop_left_while(String_View *sv, bool (*predicate)(char x))
{
    size_t i = 0;
    while (i < sv->count && predicate(sv->data[i])) {
        i += 1;
    }
    return sv_chop_left(sv, i);
}

bool sv_parse_hex(String_View sv, uint64_t *output)
{
    uint64_t result = 0;

    for (size_t i = 0; i < sv.count; ++i) {
        const char x = sv.data[i];
        if ('0' <= x && x <= '9') {
            result = result * 0x10 + (uint64_t) (x - '0');
        } else if ('a' <= x && x <= 'f') {
            result = result * 0x10 + (uint64_t) (x + 10 - 'a');
        } else if ('A' <= x && x <= 'F') {
            result = result * 0x10 + (uint64_t) (x + 10 - 'A');
        } else {
            return false;
        }
    }

    if (output) {
        *output = result;
    }

    return true;
}
