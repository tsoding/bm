#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "arena.h"

Region *region_create(size_t capacity)
{
    const size_t region_size = sizeof(Region) + capacity;
    Region *region = malloc(region_size);
    memset(region, 0, region_size);
    region->capacity = capacity;
    return region;
}

void *arena_alloc_aligned(Arena *arena, size_t size, size_t alignment)
{
    if (arena->last == NULL) {
        assert(arena->first == NULL);

        Region *region = region_create(
                             size > ARENA_DEFAULT_CAPACITY ? size : ARENA_DEFAULT_CAPACITY);

        arena->last = region;
        arena->first = region;
    }

    // deal with the zero case here specially to simplify the alignment-calculating code below.
    if (size == 0) {
        // anyway, we now know we have *a* region -- so it's valid to just return it.
        return arena->last->buffer + arena->last->size;
    }

    // alignment must be a power of two.
    assert((alignment & (alignment - 1)) == 0);

    Region *cur = arena->last;
    while (true) {

        char *ptr = (char*) (((uintptr_t) (cur->buffer + cur->size + (alignment - 1))) & ~(alignment - 1));
        size_t real_size = (size_t) ((ptr + size) - (cur->buffer + cur->size));

        if (cur->size + real_size > cur->capacity) {
            if (cur->next) {
                cur = cur->next;
                continue;
            } else {
                // out of space, make a new one. even though we are making a new region, there
                // aren't really any guarantees on the alignment of memory that malloc() returns.
                // so, allocate enough extra bytes to fix the 'worst case' alignment.
                size_t worst_case = size + (alignment - 1);

                Region *region = region_create(worst_case > ARENA_DEFAULT_CAPACITY
                                               ? worst_case
                                               : ARENA_DEFAULT_CAPACITY);

                arena->last->next = region;
                arena->last = region;
                cur = arena->last;

                // ok, now we know we have enough space. just go back to the top of the loop here,
                // so we don't duplicate the code. we now know that we will definitely succeed,
                // so there won't be any infinite looping here.
                continue;
            }
        } else {
            memset(ptr, 0, real_size);
            cur->size += real_size;
            return ptr;
        }
    }
}

void *arena_alloc(Arena *arena, size_t size)
{
    // by default, align to a pointer size. this should be sufficient on most platforms.
    return arena_alloc_aligned(arena, size, sizeof(void*));
}

void *arena_realloc(Arena *arena, void *old_ptr, size_t old_size, size_t new_size)
{
    if (old_size < new_size) {
        void *new_ptr = arena_alloc(arena, new_size);
        memcpy(new_ptr, old_ptr, old_size);
        return new_ptr;
    } else {
        return old_ptr;
    }
}

void arena_clean(Arena *arena)
{
    for (Region *iter = arena->first;
            iter != NULL;
            iter = iter->next) {
        iter->size = 0;
    }

    arena->last = arena->first;
}

void arena_free(Arena *arena)
{
    Region *iter = arena->first;
    while (iter != NULL) {
        Region *next = iter->next;
        free(iter);
        iter = next;
    }
    arena->first = NULL;
    arena->last = NULL;
}

void arena_summary(Arena *arena)
{
    if (arena->first == NULL) {
        printf("[empty]");
    }

    for (Region *iter = arena->first;
            iter != NULL;
            iter = iter->next) {
        printf("[%zu/%zu] -> ", iter->size, iter->capacity);
    }
    printf("\n");
}

const char *arena_cstr_concat(Arena *arena, ...)
{
    size_t len = 0;

    va_list args;
    va_start(args, arena);
    const char *cstr = va_arg(args, const char*);
    while (cstr != NULL) {
        len += strlen(cstr);
        cstr = va_arg(args, const char*);
    }
    va_end(args);

    char *buffer = arena_alloc(arena, len + 1);
    len = 0;

    va_start(args, arena);
    cstr = va_arg(args, const char*);
    while (cstr != NULL) {
        size_t n = strlen(cstr);
        memcpy(buffer + len, cstr, n);
        len += n;
        cstr = va_arg(args, const char*);
    }
    va_end(args);

    buffer[len] = '\0';

    return buffer;
}

int arena_slurp_file(Arena *arena, String_View file_path, String_View *content)
{
    const char *file_path_cstr = arena_sv_to_cstr(arena, file_path);

    FILE *f = fopen(file_path_cstr, "rb");
    if (f == NULL) {
        return -1;
    }

    if (fseek(f, 0, SEEK_END) < 0) {
        return -1;
    }

    long m = ftell(f);
    if (m < 0) {
        return -1;
    }

    char *buffer = arena_alloc(arena, (size_t) m);
    if (buffer == NULL) {
        return -1;
    }

    if (fseek(f, 0, SEEK_SET) < 0) {
        return -1;
    }

    size_t n = fread(buffer, 1, (size_t) m, f);
    if (ferror(f)) {
        return -1;
    }

    fclose(f);

    if (content) {
        content->count = n;
        content->data = buffer;
    }

    return 0;
}

String_View arena_sv_concat(Arena *arena, ...)
{
    size_t len = 0;

    va_list args;
    va_start(args, arena);
    String_View sv = va_arg(args, String_View);
    while (sv.data != NULL) {
        len += sv.count;
        sv = va_arg(args, String_View);
    }
    va_end(args);

    char *buffer = arena_alloc(arena, len);
    len = 0;

    va_start(args, arena);
    sv = va_arg(args, String_View);
    while (sv.data != NULL) {
        memcpy(buffer + len, sv.data, sv.count);
        len += sv.count;
        sv = va_arg(args, String_View);
    }
    va_end(args);

    return (String_View) {
        .count = len,
        .data = buffer
    };
}

String_View arena_sv_dup(Arena *arena, String_View sv)
{
    char *buffer = arena_alloc(arena, sv.count);
    memcpy(buffer, sv.data, sv.count);
    return (String_View) {
        .count = sv.count,
        .data = buffer,
    };
}

const char *arena_sv_to_cstr(Arena *arena, String_View sv)
{
    char *cstr = arena_alloc(arena, sv.count + 1);
    memcpy(cstr, sv.data, sv.count);
    cstr[sv.count] = '\0';
    return cstr;
}
