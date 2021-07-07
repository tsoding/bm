#ifndef LL_H_
#define LL_H_

#define LL_APPEND(begin, end, node) \
    do {                                     \
        if ((end) == NULL) {                 \
            assert((begin) == NULL);         \
            (begin) = (end) = (node);        \
        } else {                             \
            assert((begin) != NULL);         \
            (end)->next = node;              \
            (end) = node;                    \
        }                                    \
    } while (0)

#define LL_FOREACH(type, iter, begin) \
    for(type *iter = (begin); iter != NULL; iter = iter->next)

#endif // LL_H_
