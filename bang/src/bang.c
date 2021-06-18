#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "./arena.h"
#include "./path.h"
// #include "./basm.h"
#include "./bang_compiler.h"

static void build_usage(FILE *stream)
{
    fprintf(stream, "Usage: bang build [OPTIONS] <input.bang>\n");
    fprintf(stream, "OPTIONS:\n");
    fprintf(stream, "    -o <output>    Provide output path\n");
    fprintf(stream, "    -t <target>    Output target. Default is `bm`.\n");
    fprintf(stream, "                   Provide `list` to get the list of all available targets.\n");
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

static void build_subcommand(int argc, char **argv)
{
    static Basm basm = {0};
    static Bang bang = {0};

    const char *input_file_path = NULL;
    const char *output_file_path = NULL;
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
        } else if (strcmp(flag, "-h") == 0) {
            build_usage(stdout);
            exit(0);
        } else {
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
    bang_prepare_var_stack(&bang, &basm);

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
        assert(false && "TODO: run subcommand is not implemented yet");
    } else {
        usage(stderr);
        fprintf(stderr, "ERROR: unknown subcommand `%s`\n", subcommand);
        exit(1);
    }

    return 0;
}
