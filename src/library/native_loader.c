#include <dlfcn.h>

#include "./native_loader.h"

void native_loader_add_object(Native_Loader *loader, const char *object_path)
{
    if (loader->objects_size >= NATIVE_LOADER_CAPACITY) {
        fprintf(stderr, "ERROR: Exceeded the capacity of native objects\n");
        exit(1);
    }

    void *result = dlopen(object_path, RTLD_LAZY);
    if (result == NULL) {
        fprintf(stderr, "ERROR: could not load object `%s`: %s\n",
                object_path, dlerror());
        exit(1);
    }

    loader->objects[loader->objects_size++] = result;
    printf("INFO: successfully loaded object `%s`\n", object_path);
}

void native_loader_unload_all(Native_Loader *loader)
{
    for (size_t i = 0; i < loader->objects_size; ++i) {
        if (dlclose(loader->objects[i]) != 0) {
            fprintf(stderr, "ERROR: could not unload object %zu: %s\n", i, dlerror());
            exit(1);
        }
    }
}

Bm_Native native_loader_find_function(Native_Loader *loader, Arena *arena, const char *function_name)
{
    for (size_t i = 0; i < loader->objects_size; ++i) {
        Bm_Native result = NULL;
        *(void**)(&result) = dlsym(loader->objects[i], CSTR_CONCAT(arena, "bm_", function_name));
        if (result != NULL) {
            return result;
        }
    }

    return NULL;
}