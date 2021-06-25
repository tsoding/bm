#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "./arena.h"
#include "./path.h"
// #include "./basm.h"
#include "./bang_compiler.h"

#define BANG_DEFAULT_STACK_SIZE 4096

// TODO: move intersecting `bang run` and `bang build` flags to `bang`

static void build_usage(FILE *stream)
{
    fprintf(stream, "Usage: bang build [OPTIONS] <input.bang>\n");
    fprintf(stream, "OPTIONS:\n");
    fprintf(stream, "    -o <output>    Provide output path\n");
    fprintf(stream, "    -t <target>    Output target. Default is `bm`.\n");
    fprintf(stream, "                   Provide `list` to get the list of all available targets.\n");
    fprintf(stream, "    -s <bytes>     Local variables stack size in bytes. (default %zu)\n", (size_t) BANG_DEFAULT_STACK_SIZE);
    fprintf(stream, "    -werror        Treat warnings as errors\n");
    fprintf(stream, "    -h             Print this help to stdout\n");
}

static void usage(FILE *stream)
{
    fprintf(stream, "Usage: bang [SUBCOMMANDS]\n");
    fprintf(stream, "    build   Build the specified source code file.\n");
    fprintf(stream, "    run     Build the specified source code file and run it.\n");
    fprintf(stream, "    help    Print this message to stdout.\n");
}

static char *shift(int *argc, char ***argv)
{
    assert(*argc > 0);
    char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

static void help_subcommand(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    usage(stdout);
    exit(0);
}

static void run_usage(FILE *stream)
{
    fprintf(stream, "Usage: bang run [OPTIONS] <input.bang>\n");
    fprintf(stream, "OPTIONS:\n");
    fprintf(stream, "    -t             Enable trace mode\n");
    fprintf(stream, "    -s <bytes>     Local variables stack size in bytes. (default %zu)\n", (size_t) BANG_DEFAULT_STACK_SIZE);
    fprintf(stream, "    -werror        Treat warnings as errors\n");
    fprintf(stream, "    -h             Print this help to stdout\n");
}

static void run_subcommand(int argc, char **argv)
{
    static Bm   bm   = {0};
    static Basm basm = {0};
    static Bang bang = {0};

    bool trace = false;
    const char *input_file_path = NULL;
    size_t stack_size = BANG_DEFAULT_STACK_SIZE;

    while (argc > 0) {
        const char *flag = shift(&argc, &argv);
        if (strcmp(flag, "-t") == 0) {
            trace = true;
        } else if (strcmp(flag, "-h") == 0) {
            run_usage(stdout);
            exit(0);
        } else if (strcmp(flag, "-s") == 0) {
            if (argc <= 0) {
                run_usage(stderr);
                fprintf(stderr, "ERROR: no value is provided for flag `%s`\n", flag);
                exit(1);
            }
            const char *stack_size_cstr = shift(&argc, &argv);
            char *endptr = NULL;

            stack_size = strtoumax(stack_size_cstr, &endptr, 10);

            if (stack_size_cstr == endptr || *endptr != '\0') {
                run_usage(stderr);
                fprintf(stderr, "ERROR: `%s` is not a valid size of the stack\n",
                        stack_size_cstr);
                exit(1);
            }
        } else if (strcmp(flag, "-werror") == 0) {
            bang.warnings_as_errors = true;
        } else {
            if (input_file_path != NULL) {
                fprintf(stderr, "ERROR: input file path is already specified as `%s`. Multiple file paths are not supported yet\n", input_file_path);
                exit(1);
            }

            input_file_path = flag;
        }
    }

    if (input_file_path == NULL) {
        run_usage(stderr);
        fprintf(stderr, "ERROR: no input file is provided\n");
        exit(1);
    }

    String_View content = {0};
    if (arena_slurp_file(&bang.arena, sv_from_cstr(input_file_path), &content) < 0) {
        fprintf(stderr, "ERROR: could not read file `%s`: %s",
                input_file_path, strerror(errno));
        exit(1);
    }

    Bang_Lexer lexer = bang_lexer_from_sv(content, input_file_path);
    Bang_Module module = parse_bang_module(&basm.arena, &lexer);

    bang.write_id = basm_push_external_native(&basm, SV("write"));
    bang_prepare_var_stack(&bang, &basm, stack_size);

    bang_push_new_scope(&bang);
    {
        compile_bang_module_into_basm(&bang, &basm, module);

        bang_generate_entry_point(&bang, &basm, SV("main"));
        bang_generate_heap_base(&bang, &basm, SV("heap_base"));
        assert(basm.has_entry);
    }
    bang_pop_scope(&bang);

    basm_save_to_bm(&basm, &bm);

    for (size_t i = 0; i < bm.externals_size; ++i) {
        if (strcmp(bm.externals[i].name, "write") == 0) {
            bm.natives[i] = native_write;
        } else {
            fprintf(stderr, "WARNING: unknown native `%s`\n", bm.externals[i].name);
        }
    }

    while (!bm.halt) {
        if (trace) {
            const Inst inst = bm.program[bm.ip];
            const Inst_Def def = get_inst_def(inst.type);
            fprintf(stderr, "%s", def.name);
            if (def.has_operand) {
                fprintf(stderr, " %"PRIu64, inst.operand.as_u64);
            }
            fprintf(stderr, "\n");
        }
        Err err = bm_execute_inst(&bm);
        if (trace) {
            // TODO(#469): `bang run` needs a way to customize trace mode parameters
#define BANG_TRACE_MEMORY_START 0
#define BANG_TRACE_CELL_COUNT 5
#define BANG_TRACE_CELL_SIZE 8
            for (size_t cell = 0; cell < BANG_TRACE_CELL_COUNT; ++cell) {
                const Memory_Addr start = BANG_TRACE_MEMORY_START + cell * BANG_TRACE_CELL_SIZE;
                fprintf(stderr, "  %04"PRIX64":", start);
                for (size_t byte = 0; byte < BANG_TRACE_CELL_SIZE; ++byte) {
                    const Memory_Addr addr = start + byte;
                    fprintf(stderr, " %02X", bm.memory[addr]);
                }
                fprintf(stderr, "\n");
            }
        }

        if (err != ERR_OK) {
            fprintf(stderr, "ERROR: %s\n", err_as_cstr(err));
            exit(1);
        }
    }
}

static void build_subcommand(int argc, char **argv)
{
    static Basm basm = {0};
    static Bang bang = {0};

    const char *input_file_path = NULL;
    const char *output_file_path = NULL;
    size_t stack_size = BANG_DEFAULT_STACK_SIZE;
    Target output_target = TARGET_BM;

    while (argc > 0) {
        const char *flag = shift(&argc, &argv);

        if (strcmp(flag, "-o") == 0) {
            if (argc <= 0) {
                build_usage(stderr);
                fprintf(stderr, "ERROR: no value is provided for flag `%s`\n", flag);
                exit(1);
            }

            output_file_path = shift(&argc, &argv);
        } else if (strcmp(flag, "-t") == 0) {
            if (argc <= 0) {
                build_usage(stderr);
                fprintf(stderr, "ERROR: no value is provided for flag `%s`\n", flag);
                exit(1);
            }
            const char *name = shift(&argc, &argv);

            if (strcmp(name, "list") == 0) {
                printf("Available targets:\n");
                for (Target target = 0; target < COUNT_TARGETS; ++target) {
                    printf("  %s\n", target_name(target));
                }
                exit(0);
            }

            if (!target_by_name(name, &output_target)) {
                build_usage(stderr);
                fprintf(stderr, "ERROR: unknown target: `%s`\n", name);
                exit(1);
            }
        } else if (strcmp(flag, "-s") == 0) {
            if (argc <= 0) {
                build_usage(stderr);
                fprintf(stderr, "ERROR: no value is provided for flag `%s`\n", flag);
                exit(1);
            }
            const char *stack_size_cstr = shift(&argc, &argv);
            char *endptr = NULL;

            stack_size = strtoumax(stack_size_cstr, &endptr, 10);

            if (stack_size_cstr == endptr || *endptr != '\0') {
                build_usage(stderr);
                fprintf(stderr, "ERROR: `%s` is not a valid size of the stack\n",
                        stack_size_cstr);
                exit(1);
            }
        } else if (strcmp(flag, "-h") == 0) {
            build_usage(stdout);
            exit(0);
        } else {
            if (input_file_path != NULL) {
                fprintf(stderr, "ERROR: input file path is already specified as `%s`. Multiple file paths are not supported yet\n", input_file_path);
                exit(1);
            }

            input_file_path = flag;
        }
    }

    if (input_file_path == NULL) {
        build_usage(stderr);
        fprintf(stderr, "ERROR: no input file path was provided\n");
        exit(1);
    }

    if (output_file_path == NULL) {
        const String_View output_file_path_sv =
            SV_CONCAT(&basm.arena,
                      SV("./"),
                      file_name_of_path(input_file_path),
                      sv_from_cstr(target_file_ext(output_target)));
        output_file_path = arena_sv_to_cstr(&basm.arena, output_file_path_sv);
    }

    String_View content = {0};
    if (arena_slurp_file(&basm.arena, sv_from_cstr(input_file_path), &content) < 0) {
        fprintf(stderr, "ERROR: could not read file `%s`: %s",
                input_file_path,
                strerror(errno));
        exit(1);
    }

    Bang_Lexer lexer = bang_lexer_from_sv(content, input_file_path);
    Bang_Module module = parse_bang_module(&basm.arena, &lexer);

    bang.write_id = basm_push_external_native(&basm, SV("write"));
    bang_prepare_var_stack(&bang, &basm, stack_size);

    bang_push_new_scope(&bang);
    {
        compile_bang_module_into_basm(&bang, &basm, module);

        bang_generate_entry_point(&bang, &basm, SV("main"));
        bang_generate_heap_base(&bang, &basm, SV("heap_base"));
        assert(basm.has_entry);
        basm_save_to_file_as_target(&basm, output_file_path, output_target);
    }
    bang_pop_scope(&bang);

    arena_free(&basm.arena);
    arena_free(&bang.arena);
}

int main(int argc, char **argv)
{
    shift(&argc, &argv); // skip the program

    if (argc == 0) {
        usage(stderr);
        fprintf(stderr, "ERROR: subcommands is not specified\n");
        exit(1);
    }

    const char * const subcommand = shift(&argc, &argv);

    if (strcmp(subcommand, "build") == 0) {
        build_subcommand(argc, argv);
    } else if (strcmp(subcommand, "help") == 0) {
        help_subcommand(argc, argv);
    } else if (strcmp(subcommand, "run") == 0) {
        run_subcommand(argc, argv);
    } else {
        usage(stderr);
        fprintf(stderr, "ERROR: unknown subcommand `%s`\n", subcommand);
        exit(1);
    }

    return 0;
}
