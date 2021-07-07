#ifndef ARENA_H_
#define ARENA_H_

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "./sv.h"

// NOTE: https://en.wikipedia.org/wiki/Region-based_memory_management
typedef struct Region Region;

struct Region {
    Region *next;
    size_t capacity;
    size_t size;
    char buffer[];
};

Region *region_create(size_t capacity);

#define ARENA_DEFAULT_CAPACITY (640 * 1000)

typedef struct {
    Region *first;
    Region *last;
} Arena;

void *arena_alloc_aligned(Arena *arena, size_t size, size_t alignment);
void *arena_alloc(Arena *arena, size_t size);
void *arena_realloc(Arena *arena, void *old_ptr, size_t old_size, size_t new_size);
void arena_clean(Arena *arena);
void arena_free(Arena *arena);
void arena_summary(Arena *arena);

int arena_slurp_file(Arena *arena, String_View file_path, String_View *content);
String_View arena_sv_concat(Arena *arena, ...);
const char *arena_cstr_concat(Arena *arena, ...);
const char *arena_sv_to_cstr(Arena *arena, String_View sv);
String_View arena_sv_dup(Arena *arena, String_View sv);

#define SV_CONCAT(arena, ...)                   \
    arena_sv_concat(arena, __VA_ARGS__, SV_NULL)

#define CSTR_CONCAT(arena, ...)                 \
    arena_cstr_concat(arena, __VA_ARGS__, NULL)


#endif // ARENA_H_
