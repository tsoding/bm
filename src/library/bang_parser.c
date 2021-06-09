#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include "./bang_parser.h"

#define APPEND_LINKED_LIST(begin, end, node) \
    do {                                     \
        if ((end) == NULL) {                 \
            assert((begin) == NULL);         \
            (begin) = (end) = (node);        \
        } else {                             \
            assert((begin) != NULL);         \
            (end)->next = node;              \
            (end) = node;                    \
        }                                    \
    } while (0)

static Bang_Token_Kind binary_op_tokens[COUNT_BANG_BINARY_OP_KINDS] = {
    [BANG_BINARY_OP_KIND_PLUS]  = BANG_TOKEN_KIND_PLUS,
    [BANG_BINARY_OP_KIND_MINUS] = BANG_TOKEN_KIND_MINUS,
    [BANG_BINARY_OP_KIND_MULT]  = BANG_TOKEN_KIND_MULT,
    [BANG_BINARY_OP_KIND_LESS]  = BANG_TOKEN_KIND_LESS,
};
static_assert(
    COUNT_BANG_BINARY_OP_KINDS == 4,
    "The amount of binary operation kinds has changed. "
    "Please update the binary_op_tokens table accordingly. "
    "Thanks!");

Bang_Token_Kind bang_binary_op_token(Bang_Binary_Op_Kind kind)
{
    assert(0 <= kind);
    assert(kind < COUNT_BANG_BINARY_OP_KINDS);
    return binary_op_tokens[kind];
}

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
    Bang_Funcall_Arg *begin = NULL;
    Bang_Funcall_Arg *end = NULL;

    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_OPEN_PAREN);

    Bang_Token token = {0};

    // First arg
    if (bang_lexer_peek(lexer, &token, 0) && token.kind != BANG_TOKEN_KIND_CLOSE_PAREN) {
        Bang_Funcall_Arg *node = arena_alloc(arena, sizeof(*node));
        node->value = parse_bang_expr(arena, lexer);
        APPEND_LINKED_LIST(begin, end, node);
    }

    // Rest args
    while (bang_lexer_peek(lexer, &token, 0) && token.kind != BANG_TOKEN_KIND_CLOSE_PAREN) {
        bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_COMMA);

        Bang_Funcall_Arg *node = arena_alloc(arena, sizeof(*node));
        node->value = parse_bang_expr(arena, lexer);
        APPEND_LINKED_LIST(begin, end, node);
    }

    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_CLOSE_PAREN);

    return begin;
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

Bang_Var_Read parse_var_read(Bang_Lexer *lexer)
{
    const Bang_Token token = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME);
    Bang_Var_Read var_read = {0};
    var_read.loc = token.loc;
    var_read.name = token.text;
    return var_read;
}

static Bang_Expr parse_primary_expr(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Token token = {0};

    if (!bang_lexer_peek(lexer, &token, 0)) {
        const Bang_Loc eof_loc = bang_lexer_loc(lexer);
        fprintf(stderr, Bang_Loc_Fmt": ERROR: expected expression but got end of the file\n",
                Bang_Loc_Arg(eof_loc));
        exit(1);
    }

    static_assert(
        COUNT_BANG_EXPR_KINDS == 6,
        "The amount of the expression kinds have changed. "
        "Please update the parser to take that into account. "
        "Thanks!");

    switch (token.kind) {
    case BANG_TOKEN_KIND_NAME: {
        if (sv_eq(token.text, SV("true"))) {
            bang_lexer_next(lexer, &token);
            Bang_Expr expr = {0};
            expr.loc = token.loc;
            expr.kind = BANG_EXPR_KIND_LIT_BOOL;
            expr.as.boolean = true;
            return expr;
        } else if (sv_eq(token.text, SV("false"))) {
            bang_lexer_next(lexer, &token);
            Bang_Expr expr = {0};
            expr.loc = token.loc;
            expr.kind = BANG_EXPR_KIND_LIT_BOOL;
            expr.as.boolean = false;
            return expr;
        } else {
            Bang_Token next_token = {0};
            if (bang_lexer_peek(lexer, &next_token, 1) && next_token.kind == BANG_TOKEN_KIND_OPEN_PAREN) {
                Bang_Expr expr = {0};
                expr.loc = token.loc;
                expr.kind = BANG_EXPR_KIND_FUNCALL;
                expr.as.funcall = parse_bang_funcall(arena, lexer);
                return expr;
            } else {
                Bang_Expr expr = {0};
                expr.loc = token.loc;
                expr.kind = BANG_EXPR_KIND_VAR_READ;
                expr.as.var_read = parse_var_read(lexer);
                return expr;
            }
        }
    }

    case BANG_TOKEN_KIND_NUMBER: {
        bang_lexer_next(lexer, &token);

        int64_t result = 0;
        for (size_t i = 0; i < token.text.count; ++i) {
            const char ch = token.text.data[i];
            if (isdigit(ch)) {
                result = result * 10 + ch - '0';
            } else {
                Bang_Loc ch_loc = token.loc;
                ch_loc.col += i;
                fprintf(stderr, Bang_Loc_Fmt": ERROR: incorrect character `%c` inside of the integer literal\n",
                        Bang_Loc_Arg(ch_loc), ch);
                exit(1);
            }
        }

        Bang_Expr expr = {0};
        expr.loc = token.loc;
        expr.kind = BANG_EXPR_KIND_LIT_INT;
        expr.as.lit_int = result;
        return expr;
    }
    break;

    case BANG_TOKEN_KIND_LIT_STR: {
        Bang_Expr expr = {0};
        expr.loc = token.loc;
        expr.kind = BANG_EXPR_KIND_LIT_STR;
        expr.as.lit_str = parse_bang_lit_str(arena, lexer);
        return expr;
    }

    case BANG_TOKEN_KIND_OPEN_PAREN: {
        bang_lexer_next(lexer, &token);
        Bang_Expr expr = parse_bang_expr(arena, lexer);
        bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_CLOSE_PAREN);
        return expr;
    } break;

    case BANG_TOKEN_KIND_COMMA:
    case BANG_TOKEN_KIND_PLUS:
    case BANG_TOKEN_KIND_MINUS:
    case BANG_TOKEN_KIND_MULT:
    case BANG_TOKEN_KIND_LESS:
    case BANG_TOKEN_KIND_CLOSE_PAREN:
    case BANG_TOKEN_KIND_OPEN_CURLY:
    case BANG_TOKEN_KIND_CLOSE_CURLY:
    case BANG_TOKEN_KIND_COLON:
    case BANG_TOKEN_KIND_EQUALS:
    case BANG_TOKEN_KIND_SEMICOLON: {
        fprintf(stderr, Bang_Loc_Fmt": ERROR: no primary expression starts with `%s`\n",
                Bang_Loc_Arg(token.loc),
                bang_token_kind_name(token.kind));
        exit(1);
    }
    break;

    case COUNT_BANG_TOKEN_KINDS:
    default: {
        assert(false && "parse_primary_expr: unreachable");
        exit(1);
    }
    }
}

Bang_Expr parse_bang_expr(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Expr *lhs = arena_alloc(arena, sizeof(*lhs));
    *lhs = parse_primary_expr(arena, lexer);

    Bang_Token token = {0};
    if (bang_lexer_peek(lexer, &token, 0)) {
        for (Bang_Binary_Op_Kind kind = 0;
                kind < COUNT_BANG_BINARY_OP_KINDS;
                ++kind) {
            if (binary_op_tokens[kind] == token.kind) {
                bool ok = bang_lexer_next(lexer, &token);
                assert(ok);

                Bang_Binary_Op binary_op = {0};
                {
                    binary_op.loc = token.loc;
                    binary_op.kind = kind;
                    binary_op.lhs = lhs;

                    binary_op.rhs = arena_alloc(arena, sizeof(*binary_op.rhs));
                    *binary_op.rhs = parse_bang_expr(arena, lexer);
                }

                Bang_Expr expr = {0};
                expr.loc = token.loc;
                expr.kind = BANG_EXPR_KIND_BINARY_OP;
                expr.as.binary_op = binary_op;
                return expr;
            }
        }
    }

    return *lhs;
}

Bang_If parse_bang_if(Arena *arena, Bang_Lexer *lexer)
{
    Bang_If eef = {0};

    // then
    {
        Bang_Token token = bang_lexer_expect_keyword(lexer, SV("if"));
        eef.loc = token.loc;
        eef.condition = parse_bang_expr(arena, lexer);
        eef.then = parse_curly_bang_block(arena, lexer);
    }

    // else
    {
        Bang_Token token = {0};
        if (bang_lexer_peek(lexer, &token, 0) &&
                token.kind == BANG_TOKEN_KIND_NAME &&
                sv_eq(token.text, SV("else"))) {
            bang_lexer_next(lexer, &token);
            eef.elze = parse_curly_bang_block(arena, lexer);
        }
    }

    return eef;
}

Bang_Var_Assign parse_bang_var_assign(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Var_Assign var_assign = {0};

    {
        Bang_Token token = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME);
        var_assign.loc = token.loc;
        var_assign.name = token.text;
    }
    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_EQUALS);
    var_assign.value = parse_bang_expr(arena, lexer);

    return var_assign;
}

Bang_While parse_bang_while(Arena *arena, Bang_Lexer *lexer)
{
    Bang_While result = {0};
    result.loc = bang_lexer_expect_keyword(lexer, SV("while")).loc;
    result.condition = parse_bang_expr(arena, lexer);
    result.body = parse_curly_bang_block(arena, lexer);
    return result;
}

Bang_Stmt parse_bang_stmt(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Token token = {0};
    if (!bang_lexer_peek(lexer, &token, 0)) {
        const Bang_Loc eof_loc = bang_lexer_loc(lexer);
        fprintf(stderr, Bang_Loc_Fmt": ERROR: expected statement but reached the end of the file\n",
                Bang_Loc_Arg(eof_loc));
        exit(1);
    }

    static_assert(COUNT_BANG_STMT_KINDS == 4, "The amount of statements have changed. Please update the parse_bang_stmt function to take that into account");

    switch (token.kind) {
    case BANG_TOKEN_KIND_NAME: {
        if (sv_eq(token.text, SV("if"))) {
            Bang_Stmt stmt = {0};
            stmt.kind = BANG_STMT_KIND_IF;
            stmt.as.eef = parse_bang_if(arena, lexer);
            return stmt;
        } else if (sv_eq(token.text, SV("while"))) {
            Bang_Stmt stmt = {0};
            stmt.kind = BANG_STMT_KIND_WHILE;
            stmt.as.hwile = parse_bang_while(arena, lexer);
            return stmt;
        } else {
            Bang_Token next_token = {0};
            if (bang_lexer_peek(lexer, &next_token, 1) && next_token.kind == BANG_TOKEN_KIND_EQUALS) {
                Bang_Stmt stmt = {0};
                stmt.kind = BANG_STMT_KIND_VAR_ASSIGN;
                stmt.as.var_assign = parse_bang_var_assign(arena, lexer);
                bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_SEMICOLON);
                return stmt;
            }
        }
    }
    break;

    case BANG_TOKEN_KIND_COMMA:
    case BANG_TOKEN_KIND_PLUS:
    case BANG_TOKEN_KIND_MINUS:
    case BANG_TOKEN_KIND_MULT:
    case BANG_TOKEN_KIND_LESS:
    case BANG_TOKEN_KIND_OPEN_PAREN:
    case BANG_TOKEN_KIND_CLOSE_PAREN:
    case BANG_TOKEN_KIND_OPEN_CURLY:
    case BANG_TOKEN_KIND_CLOSE_CURLY:
    case BANG_TOKEN_KIND_SEMICOLON:
    case BANG_TOKEN_KIND_COLON:
    case BANG_TOKEN_KIND_EQUALS:
    case BANG_TOKEN_KIND_NUMBER:
    case BANG_TOKEN_KIND_LIT_STR: {
        // This is probably an expression, let's just fall through the entire switch construction and try to parse it as the expression
    } break;


    case COUNT_BANG_TOKEN_KINDS:
    default: {
        assert(false && "parse_bang_stmt: unreachable");
        exit(1);
    }
    }

    {
        Bang_Stmt stmt = {0};
        stmt.kind = BANG_STMT_KIND_EXPR;
        stmt.as.expr = parse_bang_expr(arena, lexer);
        bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_SEMICOLON);
        return stmt;
    }
}

Bang_Block *parse_curly_bang_block(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Block *begin = NULL;
    Bang_Block *end = NULL;

    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_OPEN_CURLY);

    Bang_Token token = {0};
    while (bang_lexer_peek(lexer, &token, 0) &&
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

    result.loc = bang_lexer_expect_keyword(lexer, SV("proc")).loc;
    result.name = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME).text;
    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_OPEN_PAREN);
    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_CLOSE_PAREN);
    result.body = parse_curly_bang_block(arena, lexer);

    return result;
}

Bang_Var_Def parse_bang_var_def(Bang_Lexer *lexer)
{
    Bang_Var_Def var_def = {0};

    var_def.loc = bang_lexer_expect_keyword(lexer, SV("var")).loc;
    var_def.name = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME).text;
    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_COLON);
    var_def.type_name = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME).text;
    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_SEMICOLON);

    return var_def;
}

Bang_Module parse_bang_module(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Module module = {0};
    Bang_Token token = {0};

    while (bang_lexer_peek(lexer, &token, 0)) {
        if (token.kind != BANG_TOKEN_KIND_NAME) {
            fprintf(stderr, Bang_Loc_Fmt": ERROR: expected token `%s` but got `%s`",
                    Bang_Loc_Arg(token.loc),
                    bang_token_kind_name(BANG_TOKEN_KIND_NAME),
                    bang_token_kind_name(token.kind));
            exit(1);
        }

        Bang_Top *top = arena_alloc(arena, sizeof(*top));

        if (sv_eq(token.text, SV("proc"))) {
            top->kind = BANG_TOP_KIND_PROC;
            top->as.proc = parse_bang_proc_def(arena, lexer);
        } else if (sv_eq(token.text, SV("var"))) {
            top->kind = BANG_TOP_KIND_VAR;
            top->as.var = parse_bang_var_def(lexer);
        } else {
            static_assert(COUNT_BANG_TOP_KINDS == 2, "The error message below assumes that there is only two top level definition kinds");
            (void) BANG_TOP_KIND_PROC; // The error message below assumes there is a proc top level definition. Please update the message if needed.
            (void) BANG_TOP_KIND_VAR; // The error message below assumes there is a var top level definition. Please update the message if needed.
            fprintf(stderr, Bang_Loc_Fmt": ERROR: expected top level module definition (proc or var) but got `"SV_Fmt"`",
                    Bang_Loc_Arg(token.loc),
                    SV_Arg(token.text));
            exit(1);
        }

        bang_module_push_top(&module, top);
    }

    return module;
}

void bang_module_push_top(Bang_Module *module, Bang_Top *top)
{
    if (module->tops_end == NULL) {
        assert(module->tops_begin == NULL);
        module->tops_begin = top;
        module->tops_end = top;
    } else {
        assert(module->tops_begin != NULL);
        module->tops_end->next = top;
        module->tops_end = top;
    }
}
