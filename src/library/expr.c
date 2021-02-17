#include <assert.h>
#include <inttypes.h>
#include <ctype.h>
#include "./expr.h"

void dump_binary_op(FILE *stream, Binary_Op binary_op, int level)
{
    fprintf(stream, "%*sLeft:\n", level * 2, "");
    dump_expr(stream, binary_op.left, level + 1);

    fprintf(stream, "%*sRight:\n", level * 2, "");
    dump_expr(stream, binary_op.right, level + 1);
}

void dump_funcall_args(FILE *stream, Funcall_Arg *args, int level)
{
    while (args != NULL) {
        fprintf(stream, "%*sArg:\n", level * 2, "");
        dump_expr(stream, args->value, level + 1);
        args = args->next;
    }
}

Funcall_Arg *parse_funcall_args(Arena *arena, Tokens_View *tokens, File_Location location)
{
    if (tokens->count < 1 || tokens->elems->kind != TOKEN_KIND_OPEN_PAREN) {
        fprintf(stderr, FL_Fmt": ERROR: expected %s\n",
                FL_Arg(location),
                token_kind_name(TOKEN_KIND_OPEN_PAREN));
        exit(1);
    }
    tv_chop_left(tokens, 1);

    if (tokens->count > 0 && tokens->elems->kind == TOKEN_KIND_CLOSING_PAREN) {
        return NULL;
    }

    Funcall_Arg *first = NULL;
    Funcall_Arg *last = NULL;

    // , b, c)
    Token token = {0};
    do {
        Funcall_Arg *arg = arena_alloc(arena, sizeof(Funcall_Arg));
        arg->value = parse_expr_from_tokens(arena, tokens, location);

        if (first == NULL) {
            first = arg;
            last = arg;
        } else {
            last->next = arg;
            last = arg;
        }

        if (tokens->count == 0) {
            fprintf(stderr, FL_Fmt": ERROR: expected %s or %s\n",
                    FL_Arg(location),
                    token_kind_name(TOKEN_KIND_CLOSING_PAREN),
                    token_kind_name(TOKEN_KIND_COMMA));
            exit(1);
        }

        token = tv_chop_left(tokens, 1).elems[0];
    } while (token.kind == TOKEN_KIND_COMMA);

    if (token.kind != TOKEN_KIND_CLOSING_PAREN) {
        fprintf(stderr, FL_Fmt": ERROR: expected %s\n",
                FL_Arg(location),
                token_kind_name(TOKEN_KIND_CLOSING_PAREN));
        exit(1);
    }

    return first;
}

Expr parse_gt_from_tokens(Arena *arena, Tokens_View *tokens, File_Location location)
{
    Expr left = parse_sum_from_tokens(arena, tokens, location);

    if (tokens->count != 0 && tokens->elems->kind == TOKEN_KIND_GT) {
        tv_chop_left(tokens, 1);

        Expr right = parse_gt_from_tokens(arena, tokens, location);

        Binary_Op *binary_op = arena_alloc(arena, sizeof(Binary_Op));
        binary_op->kind = BINARY_OP_GT;
        binary_op->left = left;
        binary_op->right = right;

        Expr result = {
            .kind = EXPR_KIND_BINARY_OP,
            .value = {
                .as_binary_op = binary_op,
            }
        };

        return result;
    } else {
        return left;
    }
}

Expr parse_sum_from_tokens(Arena *arena, Tokens_View *tokens, File_Location location)
{
    Expr left = parse_primary_from_tokens(arena, tokens, location);

    if (tokens->count != 0 && tokens->elems->kind == TOKEN_KIND_PLUS) {
        tv_chop_left(tokens, 1);

        Expr right = parse_sum_from_tokens(arena, tokens, location);

        Binary_Op *binary_op = arena_alloc(arena, sizeof(Binary_Op));
        binary_op->kind = BINARY_OP_PLUS;
        binary_op->left = left;
        binary_op->right = right;

        Expr result = {
            .kind = EXPR_KIND_BINARY_OP,
            .value = {
                .as_binary_op = binary_op,
            }
        };

        return result;
    } else {
        return left;
    }
}

Expr parse_expr_from_tokens(Arena *arena, Tokens_View *tokens, File_Location location)
{
    return parse_gt_from_tokens(arena, tokens, location);
}

Expr parse_expr_from_sv(Arena *arena, String_View source, File_Location location)
{
    Tokens tokens = {0};
    tokenize(source, &tokens, location);

    Tokens_View tv = tokens_as_view(&tokens);
    return parse_expr_from_tokens(arena, &tv, location);
}

void dump_expr(FILE *stream, Expr expr, int level)
{
    fprintf(stream, "%*s", level * 2, "");

    switch(expr.kind) {
    case EXPR_KIND_BINDING:
        fprintf(stream, "Binding: "SV_Fmt"\n",
                SV_Arg(expr.value.as_binding));
        break;
    case EXPR_KIND_LIT_INT:
        fprintf(stream, "Int Literal: %"PRIu64"\n", expr.value.as_lit_int);
        break;
    case EXPR_KIND_LIT_FLOAT:
        fprintf(stream, "Float Literal: %lf\n", expr.value.as_lit_float);
        break;
    case EXPR_KIND_LIT_CHAR:
        fprintf(stream, "Char Literal: '%c'\n", expr.value.as_lit_char);
        break;
    case EXPR_KIND_LIT_STR:
        fprintf(stream, "String Literal: \""SV_Fmt"\"\n",
                SV_Arg(expr.value.as_lit_str));
        break;
    case EXPR_KIND_BINARY_OP:
        fprintf(stream, "Binary Op: %s\n",
                binary_op_kind_name(expr.value.as_binary_op->kind));
        dump_binary_op(stream, *expr.value.as_binary_op, level + 1);
        break;
    case EXPR_KIND_FUNCALL:
        fprintf(stream, "Funcall: "SV_Fmt"\n",
                SV_Arg(expr.value.as_funcall->name));
        dump_funcall_args(stream, expr.value.as_funcall->args, level + 1);
        break;
    }
}

static int dump_expr_as_dot_edges(FILE *stream, Expr expr, int *counter)
{
    int id = (*counter)++;

    switch (expr.kind) {
    case EXPR_KIND_BINDING: {
        fprintf(stream, "Expr_%d [shape=box label=\""SV_Fmt"\"]\n",
                id, SV_Arg(expr.value.as_binding));
    }
    break;
    case EXPR_KIND_LIT_INT: {
        fprintf(stream, "Expr_%d [shape=circle label=\"%"PRIu64"\"]\n",
                id, expr.value.as_lit_int);
    }
    break;
    case EXPR_KIND_LIT_FLOAT: {
        fprintf(stream, "Expr_%d [shape=circle label=\"%lf\"]\n",
                id, expr.value.as_lit_float);
    }
    break;
    case EXPR_KIND_LIT_CHAR: {
        fprintf(stream, "Expr_%d [shape=circle label=\"'%c'\"]\n",
                id, expr.value.as_lit_char);
    }
    break;
    case EXPR_KIND_LIT_STR: {
        fprintf(stream, "Expr_%d [shape=circle label=\""SV_Fmt"\"]\n",
                id, SV_Arg(expr.value.as_lit_str));
    }
    break;
    case EXPR_KIND_BINARY_OP: {
        fprintf(stream, "Expr_%d [shape=diamond label=\"%s\"]\n",
                id, binary_op_kind_name(expr.value.as_binary_op->kind));
        int left_id = dump_expr_as_dot_edges(stream, expr.value.as_binary_op->left, counter);
        int right_id = dump_expr_as_dot_edges(stream, expr.value.as_binary_op->right, counter);
        fprintf(stream, "Expr_%d -> Expr_%d\n", id, left_id);
        fprintf(stream, "Expr_%d -> Expr_%d\n", id, right_id);
    }
    break;
    case EXPR_KIND_FUNCALL: {
        fprintf(stream, "Expr_%d [shape=diamond label=\""SV_Fmt"\"]\n",
                id, SV_Arg(expr.value.as_funcall->name));

        for (Funcall_Arg *arg = expr.value.as_funcall->args;
                arg != NULL;
                arg = arg->next) {
            int child_id = dump_expr_as_dot_edges(stream, arg->value, counter);
            fprintf(stream, "Expr_%d -> Expr_%d\n", id, child_id);
        }
    }
    break;
    }

    return id;
}

void dump_expr_as_dot(FILE *stream, Expr expr)
{
    fprintf(stream, "digraph Expr {\n");
    int counter = 0;
    dump_expr_as_dot_edges(stream, expr, &counter);
    fprintf(stream, "}\n");
}

size_t funcall_args_len(Funcall_Arg *args)
{
    size_t result = 0;
    while (args != NULL) {
        result += 1;
        args = args->next;
    }
    return result;
}

static Expr parse_number_from_tokens(Arena *arena, Tokens_View *tokens, File_Location location)
{
    if (tokens->count == 0) {
        fprintf(stderr, FL_Fmt": ERROR: Cannot parse empty expression\n",
                FL_Arg(location));
        exit(1);
    }

    Expr result = {0};

    if (tokens->elems->kind == TOKEN_KIND_NUMBER) {
        String_View text = tokens->elems->text;

        const char *cstr = arena_sv_to_cstr(arena, text);
        char *endptr = 0;

        if (sv_has_prefix(text, sv_from_cstr("0x"))) {
            result.value.as_lit_int = strtoull(cstr, &endptr, 16);
            if ((size_t) (endptr - cstr) != text.count) {
                fprintf(stderr, FL_Fmt": ERROR: `"SV_Fmt"` is not a hex literal\n",
                        FL_Arg(location), SV_Arg(text));
                exit(1);
            }

            result.kind = EXPR_KIND_LIT_INT;
            tv_chop_left(tokens, 1);
        } else {
            result.value.as_lit_int = strtoull(cstr, &endptr, 10);
            if ((size_t) (endptr - cstr) != text.count) {
                result.value.as_lit_float = strtod(cstr, &endptr);
                if ((size_t) (endptr - cstr) != text.count) {
                    fprintf(stderr, FL_Fmt": ERROR: `"SV_Fmt"` is not a number literal\n",
                            FL_Arg(location), SV_Arg(text));
                } else {
                    result.kind = EXPR_KIND_LIT_FLOAT;
                }
            } else {
                result.kind = EXPR_KIND_LIT_INT;
            }

            tv_chop_left(tokens, 1);
        }
    } else {
        fprintf(stderr, FL_Fmt": ERROR: expected %s but got %s",
                FL_Arg(location),
                token_kind_name(TOKEN_KIND_NUMBER),
                token_kind_name(tokens->elems->kind));
        exit(1);
    }

    return result;
}

// TODO(#199): parse_primary_from_tokens does not support parens
Expr parse_primary_from_tokens(Arena *arena, Tokens_View *tokens, File_Location location)
{
    if (tokens->count == 0) {
        fprintf(stderr, FL_Fmt": ERROR: Cannot parse empty expression\n",
                FL_Arg(location));
        exit(1);
    }

    Expr result = {0};

    switch (tokens->elems->kind) {
    case TOKEN_KIND_STR: {
        // TODO(#66): string literals don't support escaped characters
        result.kind = EXPR_KIND_LIT_STR;
        result.value.as_lit_str = tokens->elems->text;
        tv_chop_left(tokens, 1);
    }
    break;

    case TOKEN_KIND_CHAR: {
        if (tokens->elems->text.count != 1) {
            // TODO(#179): char literals don't support escaped characters
            fprintf(stderr, FL_Fmt": ERROR: the length of char literal has to be exactly one\n",
                    FL_Arg(location));
            exit(1);
        }

        result.kind = EXPR_KIND_LIT_CHAR;
        result.value.as_lit_char = tokens->elems->text.data[0];
        tv_chop_left(tokens, 1);
    }
    break;

    case TOKEN_KIND_NAME: {
        if (tokens->count > 1 && tokens->elems[1].kind == TOKEN_KIND_OPEN_PAREN) {
            result.kind = EXPR_KIND_FUNCALL;
            result.value.as_funcall = arena_alloc(arena, sizeof(Funcall));
            result.value.as_funcall->name = tokens->elems->text;
            tv_chop_left(tokens, 1);
            result.value.as_funcall->args = parse_funcall_args(arena, tokens, location);
        } else {
            result.value.as_binding = tokens->elems->text;
            result.kind = EXPR_KIND_BINDING;
            tv_chop_left(tokens, 1);
        }
    }
    break;

    case TOKEN_KIND_NUMBER: {
        return parse_number_from_tokens(arena, tokens, location);
    }
    break;

    case TOKEN_KIND_MINUS: {
        tv_chop_left(tokens, 1);
        Expr expr = parse_number_from_tokens(arena, tokens, location);

        if (expr.kind == EXPR_KIND_LIT_INT) {
            // TODO(#184): more cross-platform way to negate integer literals
            // what if somewhere the numbers are not two's complement
            expr.value.as_lit_int = (~expr.value.as_lit_int + 1);
        } else if (expr.kind == EXPR_KIND_LIT_FLOAT) {
            expr.value.as_lit_float = -expr.value.as_lit_float;
        } else {
            assert(false && "parse_primary_from_tokens: unreachable");
        }

        return expr;
    }
    break;

    case TOKEN_KIND_GT:
    case TOKEN_KIND_OPEN_PAREN:
    case TOKEN_KIND_COMMA:
    case TOKEN_KIND_CLOSING_PAREN:
    case TOKEN_KIND_PLUS: {
        fprintf(stderr, FL_Fmt": ERROR: expected primary expression but found %s\n",
                FL_Arg(location), token_kind_name(tokens->elems->kind));
        exit(1);
    }
    break;

    default: {
        assert(false && "parse_primary_from_tokens: unreachable");
        exit(1);
    }
    }

    return result;
}

const char *binary_op_kind_name(Binary_Op_Kind kind)
{
    switch (kind) {
    case BINARY_OP_PLUS:
        return "+";
    case BINARY_OP_GT:
        return ">";
    default:
        assert(false && "binary_op_kind_name: unreachable");
        exit(1);
    }
}

static bool is_name(char x)
{
    return isalnum(x) || x == '_';
}

static bool is_number(char x)
{
    return isalnum(x) || x == '.';
}

const char *token_kind_name(Token_Kind kind)
{
    switch (kind) {
    case TOKEN_KIND_STR:
        return "string";
    case TOKEN_KIND_CHAR:
        return "character";
    case TOKEN_KIND_PLUS:
        return "plus";
    case TOKEN_KIND_MINUS:
        return "minus";
    case TOKEN_KIND_NUMBER:
        return "number";
    case TOKEN_KIND_NAME:
        return "name";
    case TOKEN_KIND_OPEN_PAREN:
        return "open paren";
    case TOKEN_KIND_CLOSING_PAREN:
        return "closing paren";
    case TOKEN_KIND_COMMA:
        return "comma";
    case TOKEN_KIND_GT:
        return ">";
    default: {
        assert(false && "token_kind_name: unreachable");
        exit(1);
    }
    }
}

void tokens_push(Tokens *tokens, Token token)
{
    assert(tokens->count < TOKENS_CAPACITY);
    tokens->elems[tokens->count++] = token;
}

void tokenize(String_View source, Tokens *tokens, File_Location location)
{
    source = sv_trim_left(source);
    while (source.count > 0) {
        switch (*source.data) {
        case '(': {
            tokens_push(tokens, (Token) {
                .kind = TOKEN_KIND_OPEN_PAREN,
                .text = sv_chop_left(&source, 1)
            });
        }
        break;

        case ')': {
            tokens_push(tokens, (Token) {
                .kind = TOKEN_KIND_CLOSING_PAREN,
                .text = sv_chop_left(&source, 1)
            });
        }
        break;

        case ',': {
            tokens_push(tokens, (Token) {
                .kind = TOKEN_KIND_COMMA,
                .text = sv_chop_left(&source, 1)
            });
        }
        break;

        case '>': {
            tokens_push(tokens, (Token) {
                .kind = TOKEN_KIND_GT,
                .text = sv_chop_left(&source, 1)
            });
        }
        break;

        case '+': {
            tokens_push(tokens, (Token) {
                .kind = TOKEN_KIND_PLUS,
                .text = sv_chop_left(&source, 1)
            });
        }
        break;

        case '-': {
            tokens_push(tokens, (Token) {
                .kind = TOKEN_KIND_MINUS,
                .text = sv_chop_left(&source, 1)
            });
        }
        break;

        case '"': {
            sv_chop_left(&source, 1);

            size_t index = 0;

            if (sv_index_of(source, '"', &index)) {
                String_View text = sv_chop_left(&source, index);
                sv_chop_left(&source, 1);
                tokens_push(tokens, (Token) {
                    .kind = TOKEN_KIND_STR, .text = text
                });
            } else {
                fprintf(stderr, FL_Fmt": ERROR: Could not find closing \"\n",
                        FL_Arg(location));
                exit(1);
            }
        }
        break;

        case '\'': {
            sv_chop_left(&source, 1);

            size_t index = 0;

            if (sv_index_of(source, '\'', &index)) {
                String_View text = sv_chop_left(&source, index);
                sv_chop_left(&source, 1);
                tokens_push(tokens, (Token) {
                    .kind = TOKEN_KIND_CHAR, .text = text
                });
            } else {
                fprintf(stderr, FL_Fmt": ERROR: Could not find closing \'\n",
                        FL_Arg(location));
                exit(1);
            }
        }
        break;

        default: {
            if (isalpha(*source.data)) {
                tokens_push(tokens, (Token) {
                    .kind = TOKEN_KIND_NAME,
                    .text = sv_chop_left_while(&source, is_name)
                });
            } else if (isdigit(*source.data)) {
                tokens_push(tokens, (Token) {
                    .kind = TOKEN_KIND_NUMBER,
                    .text = sv_chop_left_while(&source, is_number)
                });
            } else {
                fprintf(stderr, FL_Fmt": ERROR: Unknown token starts with %c\n",
                        FL_Arg(location), *source.data);
                exit(1);
            }
        }
        }

        source = sv_trim_left(source);
    }
}

Tokens_View tokens_as_view(const Tokens *tokens)
{
    return (Tokens_View) {
        .elems = tokens->elems,
        .count = tokens->count,
    };
}

Tokens_View tv_chop_left(Tokens_View *tv, size_t n)
{
    if (n > tv->count) {
        n = tv->count;
    }

    Tokens_View result = {
        .elems = tv->elems,
        .count = n,
    };

    tv->elems += n;
    tv->count -= n;

    return result;
}
