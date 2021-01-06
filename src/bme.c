#define BM_IMPLEMENTATION
#include "./bm.h"

Bm bm = {0};

static  char *shift(int *argc, char ***argv)
{
    assert(*argc > 0);
    char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

static void usage(FILE *stream, const char *program)
{
    fprintf(stream, "Usage: %s -i <input.bm> [-l <limit>] [-h] [-d]\n", program);
}

static Err bm_alloc(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    bm->stack[bm->stack_size - 1].as_ptr = malloc(bm->stack[bm->stack_size - 1].as_u64);

    return ERR_OK;
}

static Err bm_free(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    free(bm->stack[bm->stack_size - 1].as_ptr);
    bm->stack_size -= 1;

    return ERR_OK;
}

static Err bm_print_f64(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    printf("%lf\n", bm->stack[bm->stack_size - 1].as_f64);
    bm->stack_size -= 1;
    return ERR_OK;
}

static Err bm_print_i64(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    printf("%" PRId64 "\n", bm->stack[bm->stack_size - 1].as_i64);
    bm->stack_size -= 1;
    return ERR_OK;
}

static Err bm_print_u64(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    printf("%" PRIu64 "\n", bm->stack[bm->stack_size - 1].as_u64);
    bm->stack_size -= 1;
    return ERR_OK;
}

static Err bm_print_ptr(Bm *bm)
{
    if (bm->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    printf("%p\n", bm->stack[bm->stack_size - 1].as_ptr);
    bm->stack_size -= 1;
    return ERR_OK;
}

static Err bm_dump_memory(Bm *bm)
{
    if (bm->stack_size < 2) {
        return ERR_STACK_UNDERFLOW;
    }

    Memory_Addr addr = bm->stack[bm->stack_size - 2].as_u64;
    uint64_t count = bm->stack[bm->stack_size - 1].as_u64;

    if (addr >= BM_MEMORY_CAPACITY) {
        return ERR_ILLEGAL_MEMORY_ACCESS;
    }

    if (addr + count < addr || addr + count >= BM_MEMORY_CAPACITY) {
        return ERR_ILLEGAL_MEMORY_ACCESS;
    }

    for (uint64_t i = 0; i < count; ++i) {
        printf("%02X ", bm->memory[addr + i]);
    }
    printf("\n");

    bm->stack_size -= 2;

    return ERR_OK;
}

// TODO: implement gdb-style (but better of course) debugger for bm
// TODO: rot13 example that read/writes data from/to the bm memory

int main(int argc, char **argv)
{
    const char *program = shift(&argc, &argv);
    const char *input_file_path = NULL;
    int limit = -1;
    int debug = 0;

    while (argc > 0) {
        const char *flag = shift(&argc, &argv);

        if (strcmp(flag, "-i") == 0) {
            if (argc == 0) {
                usage(stderr, program);
                fprintf(stderr, "ERROR: No argument is provided for flag `%s`\n", flag);
                exit(1);
            }

            input_file_path = shift(&argc, &argv);
        } else if (strcmp(flag, "-l") == 0) {
            if (argc == 0) {
                usage(stderr, program);
                fprintf(stderr, "ERROR: No argument is provided for flag `%s`\n", flag);
                exit(1);
            }

            limit = atoi(shift(&argc, &argv));
        } else if (strcmp(flag, "-h") == 0) {
            usage(stdout, program);
            exit(0);
        } else if (strcmp(flag, "-d") == 0) {
            debug = 1;
        } else {
            usage(stderr, program);
            fprintf(stderr, "ERROR: Unknown flag `%s`\n", flag);
            exit(1);
        }
    }

    if (input_file_path == NULL) {
        usage(stderr, program);
        fprintf(stderr, "ERROR: input was not provided\n");
        exit(1);
    }

    bm_load_program_from_file(&bm, input_file_path);
    // TODO(#35): some sort of mechanism to load native functions from DLLs
    bm_push_native(&bm, bm_alloc);     // 0
    bm_push_native(&bm, bm_free);      // 1
    bm_push_native(&bm, bm_print_f64); // 2
    bm_push_native(&bm, bm_print_i64); // 3
    bm_push_native(&bm, bm_print_u64); // 4
    bm_push_native(&bm, bm_print_ptr); // 5
    bm_push_native(&bm, bm_dump_memory); // 6

    if (!debug) {
        Err err = bm_execute_program(&bm, limit);

        if (err != ERR_OK) {
            fprintf(stderr, "ERROR: %s\n", err_as_cstr(err));
            return 1;
        }
    } else {
        while (limit != 0 && !bm.halt) {
            bm_dump_stack(stdout, &bm);
            printf("Instruction: %s %" PRIu64 "\n",
                   inst_name(bm.program[bm.ip].type),
                   bm.program[bm.ip].operand.as_u64);
            getchar();

            Err err = bm_execute_inst(&bm);
            if (err != ERR_OK) {
                fprintf(stderr, "ERROR: %s\n", err_as_cstr(err));
                return 1;
            }
            if (limit > 0) {
                --limit;
            }
        }
    }

    return 0;
}
