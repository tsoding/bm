#include <stdio.h>
#include <assert.h>
#include "./bang_parser.h"

String_View parse_bang_lit_str(Arena *arena, Bang_Lexer *lexer)
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

Bang_Funcall_Arg *parse_bang_funcall_args(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Funcall_Arg *result = NULL;

    // TODO: parse_bang_funcall_args only parses a single argument

    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_OPEN_PAREN);
    result = arena_alloc(arena, sizeof(*result));
    result->value = parse_bang_expr(arena, lexer);
    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_CLOSE_PAREN);

    return result;
}

Bang_Funcall parse_bang_funcall(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Funcall funcall = {0};
    Bang_Token token = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME);
    funcall.name = token.text;
    funcall.loc  = token.loc;
    funcall.args = parse_bang_funcall_args(arena, lexer);
    return funcall;
}

Bang_Expr parse_bang_expr(Arena *arena, Bang_Lexer *lexer)
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

Bang_Stmt parse_bang_stmt(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Stmt stmt = {0};
    stmt.kind = BANG_STMT_KIND_EXPR;
    stmt.as.expr = parse_bang_expr(arena, lexer);
    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_SEMICOLON);
    return stmt;
}

Bang_Block *parse_curly_bang_block(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Block *begin = NULL;
    Bang_Block *end = NULL;

    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_OPEN_CURLY);

    Bang_Token token = {0};
    while (bang_lexer_peek(lexer, &token) &&
            token.kind != BANG_TOKEN_KIND_CLOSE_CURLY) {
        Bang_Block *node = arena_alloc(arena, sizeof(*node));
        node->stmt = parse_bang_stmt(arena, lexer);

        if (end) {
            end->next = node;
            end = node;
        } else {
            assert(begin == NULL);
            begin = end = node;
        }
    }

    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_CLOSE_CURLY);

    return begin;
}

Bang_Proc_Def parse_bang_proc_def(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Proc_Def result = {0};

    bang_lexer_expect_keyword(lexer, SV("proc"));
    result.name = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME).text;
    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_OPEN_PAREN);
    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_CLOSE_PAREN);
    result.body = parse_curly_bang_block(arena, lexer);

    return result;
}
