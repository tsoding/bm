#ifndef DYNARRAY_H_
#define DYNARRAY_H_

#define DYNARRAY_INIT_CAP 16

#define DYNARRAY_PUSH(arena, dynarray, item)                                  \
    do {                                                                      \
        if (dynarray.size >= dynarray.capacity) {                             \
            size_t new_capacity = dynarray.capacity;                          \
            if (new_capacity == 0) {                                          \
                new_capacity = DYNARRAY_INIT_CAP;                             \
            } else {                                                          \
                new_capacity *= 2;                                            \
            }                                                                 \
            dynarray.items = arena_realloc(                                   \
                                 arena,                                       \
                                 dynarray.items,                              \
                                 dynarray.capacity * sizeof(*dynarray.items), \
                                 new_capacity * sizeof(*dynarray.items));     \
            dynarray.capacity = new_capacity;                                 \
        }                                                                     \
                                                                              \
        dynarray.items[dynarray.size++] = item;                               \
    } while (0);


#endif // DYNARRAY_H_
