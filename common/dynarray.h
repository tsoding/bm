#ifndef DYNARRAY_H_
#define DYNARRAY_H_

#define DYNARRAY_PUSH(arena, dynarray, item_type, item)                       \
    do {                                                                      \
        if (dynarray.size >= dynarray.capacity) {                             \
            size_t new_capacity = dynarray.capacity;                          \
            if (new_capacity == 0) {                                          \
                new_capacity = 128;                                           \
            } else {                                                          \
                new_capacity *= 2;                                            \
            }                                                                 \
            dynarray.items = arena_realloc(arena,                             \
                                           dynarray.items,                    \
                                           dynarray.capacity *                \
                                           sizeof(item_type),                 \
                                           new_capacity * sizeof(item_type)); \
            dynarray.capacity = new_capacity;                                 \
        }                                                                     \
                                                                              \
        dynarray.items[dynarray.size++] = item;                               \
    } while (0)

#endif // DYNARRAY_H_
