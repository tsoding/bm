#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "./arena.h"
#include "./path.h"
#include "./expr.h"
#include "./basm.h"
#include "./bang_lexer.h"

typedef enum {
    BANG_EXPR_KIND_LIT_STR,
    BANG_EXPR_KIND_FUNCALL,
} Bang_Expr_Kind;

typedef struct Bang_Funcall_Arg Bang_Funcall_Arg;

typedef struct {
    Bang_Loc loc;
    String_View name;
    Bang_Funcall_Arg *args;
} Bang_Funcall;

typedef union {
    String_View as_lit_str;
    Bang_Funcall as_funcall;
} Bang_Expr_Value;

typedef struct {
    Bang_Expr_Kind kind;
    Bang_Expr_Value value;
} Bang_Expr;

typedef struct Bang_Funcall_Arg Bang_Funcall_Arg;

struct Bang_Funcall_Arg {
    Bang_Expr value;
    Bang_Funcall_Arg *next;
};

typedef struct {
    Bang_Expr expr;
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

static String_View parse_bang_lit_str(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Token token = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_LIT_STR);
    String_View lit_str = token.text;
    assert(lit_str.count >= 2);
    sv_chop_left(&lit_str, 1);
    sv_chop_right(&lit_str, 1);

    char *unescaped = arena_alloc(arena, lit_str.count);
    size_t unescaped_size = 0;

    for (size_t i = 0; i < lit_str.count; ) {
        if (lit_str.data[i] == '\\') {
            if (i + 1 >= lit_str.count) {
                Bang_Loc loc = token.loc;
                loc.col += i + 1;

                fprintf(stderr, Bang_Loc_Fmt": ERROR: unfinished string literal escape sequence\n",
                        Bang_Loc_Arg(loc));
                exit(1);
            }

            switch (lit_str.data[i + 1]) {
            case '0':
                assert(unescaped_size < lit_str.count);
                unescaped[unescaped_size++] = '\0';
                break;

            case 'n':
                assert(unescaped_size < lit_str.count);
                unescaped[unescaped_size++] = '\n';
                break;

            default: {
                Bang_Loc loc = token.loc;
                loc.col += i + 2;

                fprintf(stderr, Bang_Loc_Fmt": ERROR: unknown escape character `%c`",
                        Bang_Loc_Arg(loc), lit_str.data[i + 1]);
                exit(1);
            }
            }

            i += 2;
        } else {
            assert(unescaped_size < lit_str.count);
            unescaped[unescaped_size++] = lit_str.data[i];
            i += 1;
        }
    }

    return (String_View) {
        .count = unescaped_size,
        .data = unescaped,
    };
}

static Bang_Expr parse_bang_expr(Arena *arena, Bang_Lexer *lexer);

static Bang_Funcall_Arg *parse_bang_funcall_args(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Funcall_Arg *result = NULL;

    // TODO: parse_bang_funcall_args only parses a single argument

    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_OPEN_PAREN);
    result = arena_alloc(arena, sizeof(*result));
    result->value = parse_bang_expr(arena, lexer);
    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_CLOSE_PAREN);

    return result;
}

static Bang_Funcall parse_bang_funcall(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Funcall funcall = {0};
    Bang_Token token = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME);
    funcall.name = token.text;
    funcall.loc  = token.loc;
    funcall.args = parse_bang_funcall_args(arena, lexer);
    return funcall;
}

static Bang_Expr parse_bang_expr(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Token token = {0};

    if (!bang_lexer_peek(lexer, &token)) {
        const Bang_Loc eof_loc = bang_lexer_loc(lexer);
        fprintf(stderr, Bang_Loc_Fmt": ERROR: expected expression but got end of the file\n",
                Bang_Loc_Arg(eof_loc));
        exit(1);
    }

    switch (token.kind) {
    case BANG_TOKEN_KIND_NAME: {
        Bang_Expr expr = {0};
        expr.kind = BANG_EXPR_KIND_FUNCALL;
        expr.value.as_funcall = parse_bang_funcall(arena, lexer);
        return expr;
    }

    case BANG_TOKEN_KIND_LIT_STR: {
        Bang_Expr expr = {0};
        expr.kind = BANG_EXPR_KIND_LIT_STR;
        expr.value.as_lit_str = parse_bang_lit_str(arena, lexer);
        return expr;
    }

    case BANG_TOKEN_KIND_OPEN_PAREN:
    case BANG_TOKEN_KIND_CLOSE_PAREN:
    case BANG_TOKEN_KIND_OPEN_CURLY:
    case BANG_TOKEN_KIND_CLOSE_CURLY:
    case BANG_TOKEN_KIND_SEMICOLON: {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: no expression starts with `%s`\n",
                Bang_Loc_Arg(token.loc),
                bang_token_kind_name(token.kind));
        exit(1);
    }
    break;

    default: {
        assert(false && "parse_bang_expr: unreachable");
        exit(1);
    }
    }
}

static Bang_Block *parse_curly_bang_block(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Block *begin = NULL;
    Bang_Block *end = NULL;

    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_OPEN_CURLY);

    Bang_Token token = {0};
    while (bang_lexer_peek(lexer, &token) &&
            token.kind != BANG_TOKEN_KIND_CLOSE_CURLY) {
        Bang_Block *node = arena_alloc(arena, sizeof(*node));
        node->statement.expr = parse_bang_expr(arena, lexer);

        if (end) {
            end->next = node;
            end = node;
        } else {
            assert(begin == NULL);
            begin = end = node;
        }

        bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_SEMICOLON);
    }

    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_CLOSE_CURLY);

    return begin;
}

static Bang_Proc_Def parse_bang_proc_def(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Proc_Def result = {0};

    bang_lexer_expect_keyword(lexer, SV("proc"));
    result.name = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME).text;
    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_OPEN_PAREN);
    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_CLOSE_PAREN);
    result.body = parse_curly_bang_block(arena, lexer);

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

static void bang_funcall_expect_arity(Bang_Funcall funcall, size_t expected_arity)
{
    size_t actual_arity = 0;

    {
        Bang_Funcall_Arg *args = funcall.args;
        while (args != NULL) {
            actual_arity += 1;
            args = args->next;
        }
    }

    if (expected_arity != actual_arity) {
        fprintf(stderr, Bang_Loc_Fmt"ERROR: function `"SV_Fmt"` expectes %zu amoutn of arguments but provided %zu\n",
                Bang_Loc_Arg(funcall.loc),
                SV_Arg(funcall.name),
                expected_arity,
                actual_arity);
        exit(1);
    }
}

static void compile_bang_expr_into_basm(Basm *basm, Bang_Expr expr, Native_ID write_id)
{
    switch (expr.kind) {
    case BANG_EXPR_KIND_LIT_STR: {
        Word str_addr = basm_push_string_to_memory(basm, expr.value.as_lit_str);
        basm_push_inst(basm, INST_PUSH, str_addr);
        basm_push_inst(basm, INST_PUSH, word_u64(expr.value.as_lit_str.count));
    }
    break;

    case BANG_EXPR_KIND_FUNCALL: {
        if (sv_eq(expr.value.as_funcall.name, SV("write"))) {
            bang_funcall_expect_arity(expr.value.as_funcall, 1);
            compile_bang_expr_into_basm(basm, expr.value.as_funcall.args->value, write_id);
            basm_push_inst(basm, INST_NATIVE, word_u64(write_id));
        } else {
            fprintf(stderr, Bang_Loc_Fmt": ERROR: unknown function `"SV_Fmt"`\n",
                    Bang_Loc_Arg(expr.value.as_funcall.loc),
                    SV_Arg(expr.value.as_funcall.name));
            exit(1);
        }
    }
    break;

    default:
        assert(false && "compile_bang_expr_into_basm: unreachable");
        exit(1);
    }
}

static void compile_statement_into_basm(Basm *basm, Bang_Statement statement, Native_ID write_id)
{
    compile_bang_expr_into_basm(basm, statement.expr, write_id);
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
    Target output_target = TARGET_BM;

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

            if (!target_by_name(name, &output_target)) {
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
    Bang_Proc_Def proc_def = parse_bang_proc_def(&basm.arena, &lexer);

    Native_ID write_id = basm_push_external_native(&basm, SV("write"));
    compile_proc_def_into_basm(&basm, proc_def, write_id);
    basm_push_inst(&basm, INST_HALT, word_u64(0));
    basm_save_to_file_as_target(&basm, output_file_path, output_target);

    arena_free(&basm.arena);

    return 0;
}
