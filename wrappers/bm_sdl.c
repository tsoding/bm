#include <SDL2/SDL.h>
#include "./bm.h"

EXPORT Err bm_SDL_Init(Bm *bm);
EXPORT Err bm_SDL_Quit(Bm *bm);
EXPORT Err bm_SDL_CreateWindow(Bm *bm);
EXPORT Err bm_SDL_CreateRenderer(Bm *bm);
EXPORT Err bm_SDL_PollEvent(Bm *bm);
EXPORT Err bm_SDL_SetRenderDrawColor(Bm *bm);
EXPORT Err bm_SDL_RenderClear(Bm *bm);
EXPORT Err bm_SDL_RenderPresent(Bm *bm);
EXPORT Err bm_SDL_RenderFillRect(Bm *bm);
EXPORT Err bm_SDL_Delay(Bm *bm);
EXPORT Err bm_SDL_GetWindowSize(Bm *bm);

Err bm_SDL_Init(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    uint64_t flags = bm->stack[bm->stack_size - 1].as_u64;
    bm->stack[bm->stack_size - 1].as_u64 = (uint64_t) SDL_Init((Uint32) flags);

    return ERR_OK;
}

Err bm_SDL_Quit(Bm *bm)
{
    (void) bm;
    SDL_Quit();
    return ERR_OK;
}

Err bm_SDL_CreateWindow(Bm *bm)
{
    if (bm->stack_size < 6) {
        return ERR_STACK_UNDERFLOW;
    }

    uint64_t title_offset = bm->stack[bm->stack_size - 6].as_u64;
    int x = (int) bm->stack[bm->stack_size - 5].as_i64;
    int y = (int) bm->stack[bm->stack_size - 4].as_i64;
    int w = (int) bm->stack[bm->stack_size - 3].as_i64;
    int h = (int) bm->stack[bm->stack_size - 2].as_i64;
    Uint32 flags = (Uint32) bm->stack[bm->stack_size - 1].as_u64;

    // TODO(#292): bm_SDL_CreateWindow does not check if it's accessing valid memory of a title (vulnerability)
    void *window = SDL_CreateWindow((void*) (bm->memory + title_offset), x, y, w, h, flags);

    if (bm->stack_size >= BM_STACK_CAPACITY) {
        return ERR_STACK_OVERFLOW;
    }

    bm->stack[bm->stack_size++].as_ptr = window;

    return ERR_OK;
}

Err bm_SDL_CreateRenderer(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    // TODO(#272): renderer parameters are hardcoded in bm_SDL_CreateRenderer()
    bm->stack[bm->stack_size - 1].as_ptr = SDL_CreateRenderer(
            bm->stack[bm->stack_size - 1].as_ptr,
            -1,
            SDL_RENDERER_ACCELERATED);
    return ERR_OK;
}

Err bm_SDL_PollEvent(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    // TODO(#273): bm_SDL_PollEvent does not check if it's accessing valid memory (vulnerability)
    bm->stack[bm->stack_size - 1].as_u64 = (uint64_t) SDL_PollEvent((void*) (bm->memory + bm->stack[bm->stack_size - 1].as_u64));

    return ERR_OK;
}

Err bm_SDL_SetRenderDrawColor(Bm *bm)
{
    if (bm->stack_size < 5) {
        return ERR_STACK_UNDERFLOW;
    }

    void *renderer = bm->stack[bm->stack_size - 5].as_ptr;
    uint8_t r = (uint8_t) bm->stack[bm->stack_size - 4].as_u64;
    uint8_t g = (uint8_t) bm->stack[bm->stack_size - 3].as_u64;
    uint8_t b = (uint8_t) bm->stack[bm->stack_size - 2].as_u64;
    uint8_t a = (uint8_t) bm->stack[bm->stack_size - 1].as_u64;
    bm->stack_size -= 5;
    bm->stack[bm->stack_size++].as_u64 = (uint64_t) SDL_SetRenderDrawColor(renderer, r, g, b, a);
    return ERR_OK;
}

Err bm_SDL_RenderClear(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    bm->stack[bm->stack_size - 1].as_u64 = (uint64_t) SDL_RenderClear(bm->stack[bm->stack_size - 1].as_ptr);
    return ERR_OK;
}

Err bm_SDL_RenderPresent(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    SDL_RenderPresent(bm->stack[bm->stack_size - 1].as_ptr);
    bm->stack_size -= 1;
    return ERR_OK;
}

Err bm_SDL_RenderFillRect(Bm *bm)
{
    if (bm->stack_size < 2) {
        return ERR_STACK_UNDERFLOW;
    }

    void *renderer = bm->stack[bm->stack_size - 2].as_ptr;
    uint64_t rect_offset = bm->stack[bm->stack_size - 1].as_u64;
    bm->stack_size -= 2;

    bm->stack[bm->stack_size++].as_i64 =
        SDL_RenderFillRect(renderer, (void*) (bm->memory + rect_offset));

    return ERR_OK;
}

Err bm_SDL_Delay(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    SDL_Delay((Uint32) bm->stack[bm->stack_size - 1].as_u64);
    bm->stack_size -= 1;

    return ERR_OK;
}

Err bm_SDL_GetWindowSize(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    int w, h;
    SDL_GetWindowSize(bm->stack[bm->stack_size - 1].as_ptr, &w, &h);
    bm->stack_size -= 1;

    if (bm->stack_size + 2 >= BM_STACK_CAPACITY) {
        return ERR_STACK_OVERFLOW;
    }

    bm->stack[bm->stack_size++].as_i64 = w;
    bm->stack[bm->stack_size++].as_i64 = h;

    return ERR_OK;
}
