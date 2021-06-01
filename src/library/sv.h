#ifndef SV_H_
#define SV_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    size_t count;
    const char *data;
} String_View;

#define SV(cstr_lit) \
    ((String_View) { \
        .count = sizeof(cstr_lit) - 1, \
        .data = (cstr_lit) \
    })

#define SV_STATIC(cstr_lit) \
    { \
        .count = sizeof(cstr_lit) - 1, \
        .data = (cstr_lit) \
    }

#define SV_NULL (String_View) {0}

// printf macros for String_View
#define SV_Fmt "%.*s"
#define SV_Arg(sv) (int) (sv).count, (sv).data
// USAGE:
//   String_View name = ...;
//   printf("Name: "SV_Fmt"\n", SV_Arg(name));

String_View sv_from_cstr(const char *cstr);
String_View sv_trim_left(String_View sv);
String_View sv_trim_right(String_View sv);
String_View sv_trim(String_View sv);
String_View sv_chop_by_delim(String_View *sv, char delim);
String_View sv_chop_left(String_View *sv, size_t n);
String_View sv_chop_right(String_View *sv, size_t n);
String_View sv_chop_left_while(String_View *sv, bool (*predicate)(char x));
bool sv_index_of(String_View sv, char c, size_t *index);
bool sv_eq(String_View a, String_View b);
bool sv_starts_with(String_View sv, String_View prefix);
bool sv_ends_with(String_View sv, String_View suffix);
uint64_t sv_to_u64(String_View sv);
bool sv_parse_hex(String_View sv, uint64_t *output);

#endif  // SV_H_
