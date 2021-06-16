#ifndef _WIN32
#include <dlfcn.h>
#else
#include <windows.h>
#endif // _WIN32

#include "./native_loader.h"

void native_loader_add_object(Native_Loader *loader, const char *object_path)
{
    if (loader->objects_size >= NATIVE_LOADER_CAPACITY) {
        fprintf(stderr, "ERROR: Exceeded the capacity of native objects\n");
        exit(1);
    }
#ifndef _WIN32
    void *result = dlopen(object_path, RTLD_LAZY);
    if (result == NULL) {
        fprintf(stderr, "ERROR: could not load object `%s`: %s\n",
                object_path, dlerror());
        exit(1);
    }
#else
    HMODULE result = LoadLibraryA(object_path);
    if (result == NULL) {
        fprintf(stderr, "ERROR: could not load object %s: %u\n", object_path, GetLastError());
        exit(1);
    }
#endif
    loader->objects[loader->objects_size++] = result;
    printf("INFO: successfully loaded object `%s`\n", object_path);
}

void native_loader_unload_all(Native_Loader *loader)
{
    for (size_t i = 0; i < loader->objects_size; ++i) {
#ifndef _WIN32
        if (dlclose(loader->objects[i]) != 0) {
            fprintf(stderr, "ERROR: could not unload object %zu: %s\n", i, dlerror());
            exit(1);
        }
#else
        if (FreeLibrary(loader->objects[i]) == 0) {
            fprintf(stderr, "ERROR: could not unload object %zu: %u\n", i, GetLastError());
            exit(1);
        }
#endif
    }
}

Bm_Native native_loader_find_function(Native_Loader *loader, Arena *arena, const char *function_name)
{
    for (size_t i = 0; i < loader->objects_size; ++i) {
        Bm_Native result = NULL;
#ifndef _WIN32
        *(void**)(&result) = dlsym(loader->objects[i], CSTR_CONCAT(arena, "bm_", function_name));
#else
        *(void**)(&result) = (void*)GetProcAddress(loader->objects[i], CSTR_CONCAT(arena, "bm_", function_name));
#endif
        if (result != NULL) {
            return result;
        }
    }
    return NULL;
}
