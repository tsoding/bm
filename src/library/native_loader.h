#ifndef NATIVE_LOADER_H_
#define NATIVE_LOADER_H_

#include "./bm.h"
#include "./arena.h"

#define NATIVE_LOADER_CAPACITY 128

typedef struct {
    void *objects[NATIVE_LOADER_CAPACITY];
    size_t objects_size;
} Native_Loader;

void native_loader_add_object(Native_Loader *loader, const char *object_path);
void native_loader_unload_all(Native_Loader *loader);

Bm_Native native_loader_find_function(Native_Loader *loader, Arena *arena, const char *function_name);

#endif // NATIVE_LOADER_H_
