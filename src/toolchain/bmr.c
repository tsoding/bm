#include "./bm.h"
#include "./basm.h"

#include <stdarg.h>

typedef struct {
    size_t size;
    size_t capacity;
    char *data;
} Buffer;

static void buffer_resize(Buffer *buffer, size_t new_capacity)
{
    buffer->capacity = new_capacity;
    buffer->data = realloc(buffer->data, buffer->capacity);
}

static void buffer_write(Buffer *buffer, const char *data, size_t size)
{
    if (buffer->size + size > buffer->capacity) {
        buffer_resize(buffer, buffer->capacity * 2 + size);
    }

    memcpy(buffer->data + buffer->size, data, size);
    buffer->size += size;
}

static void buffer_free(Buffer *buffer)
{
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
}

static String_View buffer_to_sv(Buffer *buffer)
{
    return (String_View) {
        .count = buffer->size,
        .data = buffer->data
    };
}

static Buffer actual_buffer = {0};

static void panic(const char *fmt, ...)
{
    fprintf(stderr, "ERROR: ");

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");

    exit(1);
}

static void panic_errno(const char *fmt, ...)
{
    fprintf(stderr, "ERROR: ");

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, ": %s\n", strerror(errno));

    exit(1);
}

static char *shift(int *argc, char ***argv)
{
    assert(*argc > 0);
    char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

static const char *parse_cstr_value(const char *flag, int *argc, char ***argv)
{
    if (*argc == 0) {
        panic("no value provided for flag `%s`", flag);
    }
    return shift(argc, argv);
}

static Err bmr_write(Bm *bm)
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

    buffer_write(&actual_buffer, (void*) &bm->memory[addr], count);

    bm->stack_size -= 2;

    return ERR_OK;
}

static void usage(FILE *stream)
{
    fprintf(stream, "Usage: ./bmr -p <program.bm> [-ao <actual-output.txt>] [-eo <expected-output.txt>]\n");
}

static void compare_outputs(const char *file_path, String_View expected, String_View actual)
{
    for (size_t line_number = 1; expected.count > 0 && actual.count > 0; ++line_number) {
        String_View expected_line = sv_chop_by_delim(&expected, '\n');
        String_View actual_line = sv_chop_by_delim(&actual, '\n');

        if (!sv_eq(expected_line, actual_line)) {
            fprintf(stderr, "%s:%zu: ERROR: Expected output differs from the actual one.\n",
                    file_path, line_number);
            fprintf(stderr, "    Expected line: `"SV_Fmt"`\n",
                    SV_Arg(expected_line));
            fprintf(stderr, "    Actual   line: `"SV_Fmt"`\n",
                    SV_Arg(actual_line));
            exit(1);
        }
    }

    if (expected.count > 0) {
        panic("Expected output is bigger");
    }

    if (actual.count > 0) {
        panic("Actual output is bigger");
    }
}

int main(int argc, char *argv[])
{
    shift(&argc, &argv);        // skip the program name

    const char *program_file_path = NULL;
    const char *actual_output_file_path = NULL;
    const char *expected_output_file_path = NULL;

    while (argc > 0) {
        const char *flag = shift(&argc, &argv);

        if (strcmp(flag, "-p") == 0) {
            program_file_path = parse_cstr_value(flag, &argc, &argv);
        } else if(strcmp(flag, "-ao") == 0) {
            actual_output_file_path = parse_cstr_value(flag, &argc, &argv);
        } else if(strcmp(flag, "-eo") == 0) {
            expected_output_file_path = parse_cstr_value(flag, &argc, &argv);
        } else {
            panic("unknown flag `%s`", flag);
        }
    }

    if (program_file_path == NULL) {
        usage(stderr);
        panic("program file path is not provided");
    }

    if (actual_output_file_path == NULL && expected_output_file_path == NULL) {
        usage(stderr);
        panic("at least -ao or -eo is expected");
    }

    // NOTE: The structures might be quite big due its arena. Better allocate it in the static memory.
    static Bm bm = {0};

    bm_load_program_from_file(&bm, program_file_path);

    for (size_t i = 0; i < bm.externals_size; ++i) {
        if (strcmp(bm.externals[i].name, "write") == 0) {
            bm_push_native(&bm, bmr_write);
        } else if (strcmp(bm.externals[i].name, "external") == 0) {
            bm_push_native(&bm, native_external);
        } else {
            fprintf(stderr, "ERROR: bmr does not provide native function `%s`\n", bm.externals[i].name);
            exit(1);
        }
    }

    bm_push_native(&bm, bmr_write); // 0

    Err err = bm_execute_program(&bm, -1);
    if (err != ERR_OK) {
        panic(err_as_cstr(err));
    }

    if (actual_output_file_path) {
        FILE *output_file = fopen(actual_output_file_path, "wb");
        if (!output_file) {
            panic_errno("could not save output to file `%s`", output_file);
        }

        fwrite(actual_buffer.data, sizeof(actual_buffer.data[0]),
               actual_buffer.size, output_file);

        if (ferror(output_file)) {
            panic_errno("could not save output to file `%s`", output_file);
        }

        fclose(output_file);
    }

    if (expected_output_file_path) {
        static Arena expected_arena = {0};
        String_View expected_output = {0};
        if (arena_slurp_file(&expected_arena, sv_from_cstr(expected_output_file_path), &expected_output) < 0) {
            fprintf(stderr, "ERROR: could not read file %s: %s",
                    expected_output_file_path, strerror(errno));
            exit(1);
        }
        String_View actual_output = buffer_to_sv(&actual_buffer);

        compare_outputs(expected_output_file_path, expected_output, actual_output);
        printf("Expected output\n");

        arena_free(&expected_arena);
    }

    buffer_free(&actual_buffer);

    return 0;
}
