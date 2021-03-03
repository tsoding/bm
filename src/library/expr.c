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

Funcall_Arg *parse_funcall_args(Arena *arena, Tokenizer *tokenizer, File_Location location)
{
    Token token = {0};

    if (!tokenizer_next(tokenizer, &token, location) || token.kind != TOKEN_KIND_OPEN_PAREN) {
        fprintf(stderr, FL_Fmt": ERROR: expected %s\n",
                FL_Arg(location),
                token_kind_name(TOKEN_KIND_OPEN_PAREN));
        exit(1);
    }

    if (tokenizer_peek(tokenizer, &token, location) && token.kind == TOKEN_KIND_CLOSING_PAREN) {
        tokenizer_next(tokenizer, NULL, location);
        return NULL;
    }

    Funcall_Arg *first = NULL;
    Funcall_Arg *last = NULL;

    do {
        Funcall_Arg *arg = arena_alloc(arena, sizeof(Funcall_Arg));
        arg->value = parse_expr_from_tokens(arena, tokenizer, location);

        if (first == NULL) {
            first = arg;
            last = arg;
        } else {
            last->next = arg;
            last = arg;
        }

        if (!tokenizer_next(tokenizer, &token, location)) {
            fprintf(stderr, FL_Fmt": ERROR: expected %s or %s\n",
                    FL_Arg(location),
                    token_kind_name(TOKEN_KIND_CLOSING_PAREN),
                    token_kind_name(TOKEN_KIND_COMMA));
            exit(1);
        }
    } while (token.kind == TOKEN_KIND_COMMA);

    if (token.kind != TOKEN_KIND_CLOSING_PAREN) {
        fprintf(stderr, FL_Fmt": ERROR: expected %s\n",
                FL_Arg(location),
                token_kind_name(TOKEN_KIND_CLOSING_PAREN));
        exit(1);
    }

    return first;
}

bool token_kind_as_binary_op_kind(Token_Kind token_kind, Binary_Op_Kind *binary_op_kind)
{
    if (token_kind == TOKEN_KIND_PLUS) {
        if (binary_op_kind) *binary_op_kind = BINARY_OP_PLUS;
        return true;
    }

    if (token_kind == TOKEN_KIND_MULT) {
        if (binary_op_kind) *binary_op_kind = BINARY_OP_MULT;
        return true;
    }

    if (token_kind == TOKEN_KIND_GT) {
        if (binary_op_kind) *binary_op_kind = BINARY_OP_GT;
        return true;
    }

    return false;
}

Expr parse_binary_op_from_tokens(Arena *arena, Tokenizer *tokenizer,
                                 File_Location location, size_t precedence)
{
    if (precedence > MAX_PRECEDENCE) {
        return parse_primary_from_tokens(arena, tokenizer, location);
    }

    Expr left = parse_binary_op_from_tokens(arena, tokenizer, location, precedence + 1);

    Binary_Op_Kind binary_op_kind;
    Token token = {0};
    while (tokenizer_peek(tokenizer, &token, location) &&
            token_kind_as_binary_op_kind(token.kind, &binary_op_kind) &&
            binary_op_kind_precedence(binary_op_kind) == precedence) {
        tokenizer_next(tokenizer, NULL, location);

        Expr right = parse_binary_op_from_tokens(arena, tokenizer, location, precedence + 1);

        Binary_Op *binary_op = arena_alloc(arena, sizeof(Binary_Op));
        binary_op->kind = binary_op_kind;
        binary_op->left = left;
        binary_op->right = right;

        left = (Expr) {
            .kind = EXPR_KIND_BINARY_OP,
            .value = {
                .as_binary_op = binary_op,
            }
        };
    }

    return left;
}

Expr parse_expr_from_tokens(Arena *arena, Tokenizer *tokenizer, File_Location location)
{
    return parse_binary_op_from_tokens(arena, tokenizer, location, 0);
}

Expr parse_expr_from_sv(Arena *arena, String_View source, File_Location location)
{
    Tokenizer tokenizer = tokenizer_from_sv(source);
    Expr result = parse_expr_from_tokens(arena, &tokenizer, location);
    expect_no_tokens(&tokenizer, location);

    return result;
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

int dump_expr_as_dot_edges(FILE *stream, Expr expr, int *counter)
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

static Expr parse_number_from_tokens(Arena *arena, Tokenizer *tokenizer, File_Location location)
{
    Token token = {0};

    if (!tokenizer_next(tokenizer, &token, location)) {
        fprintf(stderr, FL_Fmt": ERROR: Cannot parse empty expression\n",
                FL_Arg(location));
        exit(1);
    }

    Expr result = {0};

    if (token.kind == TOKEN_KIND_NUMBER) {
        String_View text = token.text;

        const char *cstr = arena_sv_to_cstr(arena, text);
        char *endptr = 0;

        if (sv_starts_with(text, sv_from_cstr("0x"))) {
            result.value.as_lit_int = strtoull(cstr, &endptr, 16);
            if ((size_t) (endptr - cstr) != text.count) {
                fprintf(stderr, FL_Fmt": ERROR: `"SV_Fmt"` is not a hex literal\n",
                        FL_Arg(location), SV_Arg(text));
                exit(1);
            }

            result.kind = EXPR_KIND_LIT_INT;
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
        }
    } else {
        fprintf(stderr, FL_Fmt": ERROR: expected %s but got %s",
                FL_Arg(location),
                token_kind_name(TOKEN_KIND_NUMBER),
                token_kind_name(token.kind));
        exit(1);
    }

    return result;
}

String_View parse_lit_str_from_tokens(Tokenizer *tokenizer, File_Location location)
{
    Token token = {0};
    if (!tokenizer_next(tokenizer, &token, location) || token.kind != TOKEN_KIND_STR) {
        fprintf(stderr, FL_Fmt": ERROR: expected token %s\n",
                FL_Arg(location), token_kind_name(token.kind));
        exit(1);
    }

    return token.text;
}

Expr parse_primary_from_tokens(Arena *arena, Tokenizer *tokenizer, File_Location location)
{
    Token token = {0};

    if (!tokenizer_peek(tokenizer, &token, location)) {
        fprintf(stderr, FL_Fmt": ERROR: Cannot parse empty expression\n",
                FL_Arg(location));
        exit(1);
    }

    Expr result = {0};

    switch (token.kind) {
    case TOKEN_KIND_STR: {
        result.kind = EXPR_KIND_LIT_STR;
        result.value.as_lit_str = parse_lit_str_from_tokens(tokenizer, location);
    }
    break;

    case TOKEN_KIND_CHAR: {
        tokenizer_next(tokenizer, NULL, location);

        if (token.text.count != 1) {
            // TODO(#179): char literals don't support escaped characters
            fprintf(stderr, FL_Fmt": ERROR: the length of char literal has to be exactly one\n",
                    FL_Arg(location));
            exit(1);
        }

        result.kind = EXPR_KIND_LIT_CHAR;
        result.value.as_lit_char = token.text.data[0];
    }
    break;

    case TOKEN_KIND_NAME: {
        tokenizer_next(tokenizer, NULL, location);

        Token next = {0};
        if (tokenizer_peek(tokenizer, &next, location) && next.kind == TOKEN_KIND_OPEN_PAREN) {
            result.kind = EXPR_KIND_FUNCALL;
            result.value.as_funcall = arena_alloc(arena, sizeof(Funcall));
            result.value.as_funcall->name = token.text;
            result.value.as_funcall->args = parse_funcall_args(arena, tokenizer, location);
        } else {
            result.value.as_binding = token.text;
            result.kind = EXPR_KIND_BINDING;
        }
    }
    break;

    case TOKEN_KIND_NUMBER: {
        return parse_number_from_tokens(arena, tokenizer, location);
    }
    break;

    case TOKEN_KIND_MINUS: {
        tokenizer_next(tokenizer, NULL, location);
        Expr expr = parse_number_from_tokens(arena, tokenizer, location);

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

    case TOKEN_KIND_OPEN_PAREN: {
        tokenizer_next(tokenizer, NULL, location);
        Expr expr = parse_expr_from_tokens(arena, tokenizer, location);

        if (!tokenizer_next(tokenizer, &token, location) || token.kind != TOKEN_KIND_CLOSING_PAREN) {
            fprintf(stderr, FL_Fmt": ERROR: expected `%s`\n",
                    FL_Arg(location), token_kind_name(TOKEN_KIND_CLOSING_PAREN));
            exit(1);
        }
        return expr;
    }
    break;

    case TOKEN_KIND_MULT:
    case TOKEN_KIND_GT:
    case TOKEN_KIND_COMMA:
    case TOKEN_KIND_CLOSING_PAREN:
    case TOKEN_KIND_PLUS: {
        fprintf(stderr, FL_Fmt": ERROR: expected primary expression but found %s\n",
                FL_Arg(location), token_kind_name(token.kind));
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

size_t binary_op_kind_precedence(Binary_Op_Kind kind)
{
    switch (kind) {
    case BINARY_OP_GT:
        return 0;
    case BINARY_OP_PLUS:
        return 1;
    case BINARY_OP_MULT:
        return 2;
    default:
        assert(false && "binary_op_kind_precedence: unreachable");
        exit(1);
    }
}

const char *binary_op_kind_name(Binary_Op_Kind kind)
{
    switch (kind) {
    case BINARY_OP_PLUS:
        return "+";
    case BINARY_OP_GT:
        return ">";
    case BINARY_OP_MULT:
        return "*";
    default:
        assert(false && "binary_op_kind_name: unreachable");
        exit(1);
    }
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
    case TOKEN_KIND_MULT:
        return "multiply";
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

