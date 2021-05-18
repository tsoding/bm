#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "./arena.h"
#include "./path.h"
#include "./tokenizer.h"
#include "./expr.h"
#include "./basm.h"

typedef struct {
    Expr expr;
} Bang_Statement;

typedef struct Bang_Block Bang_Block;

struct Bang_Block {
    Bang_Statement statement;
    Bang_Block *next;
};

typedef struct {
    String_View name;
    Bang_Block *body;
} Bang_Proc_Def;

static Bang_Block *parse_curly_bang_block(Arena *arena, Tokenizer *tokenizer)
{
    File_Location dummy = {0};

    Bang_Block *begin = NULL;
    Bang_Block *end = NULL;

    expect_token_next(tokenizer, TOKEN_KIND_OPEN_CURLY, dummy);

    Token token = {0};
    while (tokenizer_peek(tokenizer, &token, dummy) &&
            token.kind != TOKEN_KIND_CLOSING_CURLY) {
        Bang_Block *node = arena_alloc(arena, sizeof(*node));
        node->statement.expr = parse_expr_from_tokens(arena, tokenizer, dummy);

        if (end) {
            end->next = node;
            end = node;
        } else {
            assert(begin == NULL);
            begin = end = node;
        }

        expect_token_next(tokenizer, TOKEN_KIND_SEMICOLON, dummy);
    }

    expect_token_next(tokenizer, TOKEN_KIND_CLOSING_CURLY, dummy);

    return begin;
}

static Bang_Proc_Def parse_bang_proc_def(Arena *arena, Tokenizer *tokenizer)
{
    const File_Location dummy = {0};
    Bang_Proc_Def result = {0};

    expect_token_next(tokenizer, TOKEN_KIND_PROC, dummy);
    result.name = expect_token_next(tokenizer, TOKEN_KIND_NAME, dummy).text;
    expect_token_next(tokenizer, TOKEN_KIND_OPEN_PAREN, dummy);
    expect_token_next(tokenizer, TOKEN_KIND_CLOSING_PAREN, dummy);
    result.body = parse_curly_bang_block(arena, tokenizer);

    return result;
}

static Native_ID basm_push_external_native(Basm *basm, String_View native_name)
{
    memset(basm->external_natives[basm->external_natives_size].name,
           0,
           NATIVE_NAME_CAPACITY);

    memcpy(basm->external_natives[basm->external_natives_size].name,
           native_name.data,
           native_name.count);

    Native_ID result = basm->external_natives_size++;

    return result;
}

static void basm_push_inst(Basm *basm, Inst_Type inst_type, Word inst_operand)
{
    assert(basm->program_size < BM_PROGRAM_CAPACITY);
    basm->program[basm->program_size].type = inst_type;
    basm->program[basm->program_size].operand = inst_operand;
    basm->program_size += 1;
}

static void compile_expr_into_basm(Basm *basm, Expr expr, Native_ID write_id)
{
    File_Location dummy = {0};

    switch (expr.kind) {
    case EXPR_KIND_LIT_STR: {
        Word str_addr = basm_push_string_to_memory(basm, expr.value.as_lit_str);
        basm_push_inst(basm, INST_PUSH, str_addr);
        basm_push_inst(basm, INST_PUSH, word_u64(expr.value.as_lit_str.count));
    }
    break;

    case EXPR_KIND_FUNCALL: {
        if (sv_eq(expr.value.as_funcall->name, SV("write"))) {
            funcall_expect_arity(expr.value.as_funcall, 1, dummy);
            compile_expr_into_basm(basm, expr.value.as_funcall->args->value, write_id);
            basm_push_inst(basm, INST_NATIVE, word_u64(write_id));
        } else {
            assert(false && "Unknown function");
        }
    }
    break;

    case EXPR_KIND_BINDING:
    case EXPR_KIND_LIT_INT:
    case EXPR_KIND_LIT_FLOAT:
    case EXPR_KIND_LIT_CHAR:
    case EXPR_KIND_BINARY_OP:
        assert(false && "Compilation of this kind of expressions is not supported yet");
        exit(1);
    }
}

static void compile_statement_into_basm(Basm *basm, Bang_Statement statement, Native_ID write_id)
{
    compile_expr_into_basm(basm, statement.expr, write_id);
}

static void compile_block_into_basm(Basm *basm, Bang_Block *block, Native_ID write_id)
{
    while (block) {
        compile_statement_into_basm(basm, block->statement, write_id);
        block = block->next;
    }
}

static void compile_proc_def_into_basm(Basm *basm, Bang_Proc_Def proc_def, Native_ID write_id)
{
    assert(!basm->has_entry);
    basm->entry = basm->program_size;
    compile_block_into_basm(basm, proc_def.body, write_id);
}

static void usage(FILE *stream, const char *program)
{
    fprintf(stream, "Usage: %s [OPTIONS] <input.bang>\n", program);
    fprintf(stream, "OPTIONS:\n");
    fprintf(stream, "    -o <output>    Provide output path\n");
    fprintf(stream, "    -t <target>    Output target. Default is `bm`.\n");
    fprintf(stream, "                   Provide `list` to get the list of all available targets.\n");
    fprintf(stream, "    -h             Print this help to stdout\n");
}

static char *shift(int *argc, char ***argv)
{
    assert(*argc > 0);
    char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

int main(int argc, char **argv)
{
    static Basm basm = {0};

    const char * const program = shift(&argc, &argv);
    const char *input_file_path = NULL;
    const char *output_file_path = NULL;
    Target target = TARGET_BM;

    while (argc > 0) {
        const char *flag = shift(&argc, &argv);

        if (strcmp(flag, "-o") == 0) {
            if (argc <= 0) {
                usage(stderr, program);
                fprintf(stderr, "ERROR: no value is provided for flag `%s`\n", flag);
                exit(1);
            }

            output_file_path = shift(&argc, &argv);
        } else if (strcmp(flag, "-t") == 0) {
            if (argc <= 0) {
                usage(stderr, program);
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

            if (!target_by_name(name, &target)) {
                usage(stderr, program);
                fprintf(stderr, "ERROR: unknown target: `%s`\n", name);
                exit(1);
            }
        } else if (strcmp(flag, "-h") == 0) {
            usage(stdout, program);
            exit(0);
        } else {
            input_file_path = flag;
        }
    }

    if (input_file_path == NULL) {
        usage(stderr, program);
        fprintf(stderr, "ERROR: no input file path was provided\n");
        exit(1);
    }

    if (output_file_path == NULL) {
        const String_View output_file_path_sv =
            SV_CONCAT(&basm.arena,
                      SV("./"),
                      file_name_of_path(input_file_path),
                      sv_from_cstr(target_file_ext(target)));
        output_file_path = arena_sv_to_cstr(&basm.arena, output_file_path_sv);
    }

    String_View content = {0};
    if (arena_slurp_file(&basm.arena, sv_from_cstr(input_file_path), &content) < 0) {
        fprintf(stderr, "ERROR: could not read file `%s`: %s",
                input_file_path,
                strerror(errno));
        exit(1);
    }

    Tokenizer tokenizer = tokenizer_from_sv(content);
    Bang_Proc_Def proc_def = parse_bang_proc_def(&basm.arena, &tokenizer);

    Native_ID write_id = basm_push_external_native(&basm, SV("write"));
    compile_proc_def_into_basm(&basm, proc_def, write_id);
    basm_push_inst(&basm, INST_HALT, word_u64(0));
    basm_save_to_file_as_target(&basm, output_file_path, target);

    arena_free(&basm.arena);

    return 0;
}
