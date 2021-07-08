#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include "./error.h"
#include "./dynarray.h"
#include "./bang_parser.h"

static Bang_Binary_Op_Def binary_op_defs[COUNT_BANG_BINARY_OP_KINDS] = {
    [BANG_BINARY_OP_KIND_AND] = {
        .kind       = BANG_BINARY_OP_KIND_AND,
        .token_kind = BANG_TOKEN_KIND_AND,
        .prec       = BINARY_OP_PREC0,
    },
    [BANG_BINARY_OP_KIND_OR] = {
        .kind       = BANG_BINARY_OP_KIND_OR,
        .token_kind = BANG_TOKEN_KIND_OR,
        .prec       = BINARY_OP_PREC0,
    },
    [BANG_BINARY_OP_KIND_LT] = {
        .kind       = BANG_BINARY_OP_KIND_LT,
        .token_kind = BANG_TOKEN_KIND_LT,
        .prec       = BINARY_OP_PREC1,
    },
    [BANG_BINARY_OP_KIND_GE] = {
        .kind       = BANG_BINARY_OP_KIND_GE,
        .token_kind = BANG_TOKEN_KIND_GE,
        .prec       = BINARY_OP_PREC1,
    },
    [BANG_BINARY_OP_KIND_NE] = {
        .kind       = BANG_BINARY_OP_KIND_NE,
        .token_kind = BANG_TOKEN_KIND_NE,
        .prec       = BINARY_OP_PREC1,
    },
    [BANG_BINARY_OP_KIND_EQ] = {
        .kind       = BANG_BINARY_OP_KIND_EQ,
        .token_kind = BANG_TOKEN_KIND_EQ_EQ,
        .prec       = BINARY_OP_PREC1,
    },
    [BANG_BINARY_OP_KIND_PLUS] = {
        .kind       = BANG_BINARY_OP_KIND_PLUS,
        .token_kind = BANG_TOKEN_KIND_PLUS,
        .prec       = BINARY_OP_PREC2
    },
    [BANG_BINARY_OP_KIND_MINUS] = {
        .kind       = BANG_BINARY_OP_KIND_MINUS,
        .token_kind = BANG_TOKEN_KIND_MINUS,
        .prec       = BINARY_OP_PREC2
    },
    [BANG_BINARY_OP_KIND_MULT] = {
        .kind       = BANG_BINARY_OP_KIND_MULT,
        .token_kind = BANG_TOKEN_KIND_MULT,
        .prec       = BINARY_OP_PREC3
    },
};
static_assert(
    COUNT_BANG_BINARY_OP_KINDS == 9,
    "The amount of binary operation kinds has changed. "
    "Please update the binary_op_defs table accordingly. "
    "Thanks!");

Bang_Binary_Op_Def bang_binary_op_def(Bang_Binary_Op_Kind kind)
{
    assert(0 <= kind);
    assert(kind < COUNT_BANG_BINARY_OP_KINDS);
    return binary_op_defs[kind];
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

                bang_diag_msg(loc, BANG_DIAG_ERROR,
                              "unfinished string literal escape sequence");
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

                bang_diag_msg(loc, BANG_DIAG_ERROR,
                              "unknown escape character `%c`",
                              lit_str.data[i + 1]);
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

Dynarray_Of_Bang_Funcall_Arg parse_bang_funcall_args(Arena *arena, Bang_Lexer *lexer)
{
    Dynarray_Of_Bang_Funcall_Arg args = {0};

    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_OPEN_PAREN);

    Bang_Token token = {0};

    // First arg
    if (bang_lexer_peek(lexer, &token, 0) && token.kind != BANG_TOKEN_KIND_CLOSE_PAREN) {
        Bang_Funcall_Arg arg = {
            .value = parse_bang_expr(arena, lexer),
        };
        DYNARRAY_PUSH(arena, args, arg);
    }

    // Rest args
    while (bang_lexer_peek(lexer, &token, 0) && token.kind != BANG_TOKEN_KIND_CLOSE_PAREN) {
        bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_COMMA);

        Bang_Funcall_Arg arg = {
            .value = parse_bang_expr(arena, lexer),
        };
        DYNARRAY_PUSH(arena, args, arg);
    }

    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_CLOSE_PAREN);

    return args;
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
        bang_diag_msg(bang_lexer_loc(lexer), BANG_DIAG_ERROR,
                      "expected expression but got end of the file");
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
                bang_diag_msg(ch_loc, BANG_DIAG_ERROR,
                              "incorrect character `%c` inside of the integer literal",
                              ch);
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
    }
    break;

    case BANG_TOKEN_KIND_COMMA:
    case BANG_TOKEN_KIND_PLUS:
    case BANG_TOKEN_KIND_MINUS:
    case BANG_TOKEN_KIND_MULT:
    case BANG_TOKEN_KIND_LT:
    case BANG_TOKEN_KIND_GE:
    case BANG_TOKEN_KIND_NE:
    case BANG_TOKEN_KIND_AND:
    case BANG_TOKEN_KIND_OR:
    case BANG_TOKEN_KIND_CLOSE_PAREN:
    case BANG_TOKEN_KIND_OPEN_CURLY:
    case BANG_TOKEN_KIND_CLOSE_CURLY:
    case BANG_TOKEN_KIND_COLON:
    case BANG_TOKEN_KIND_EQ:
    case BANG_TOKEN_KIND_EQ_EQ:
    case BANG_TOKEN_KIND_DOT_DOT:
    case BANG_TOKEN_KIND_SEMICOLON: {
        bang_diag_msg(token.loc, BANG_DIAG_ERROR,
                      "no primary expression starts with `%s`",
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

static bool bang_binary_op_def_by_token(Bang_Token_Kind token_kind, Bang_Binary_Op_Def *def)
{
    for (Bang_Binary_Op_Kind kind = 0;
            kind < COUNT_BANG_BINARY_OP_KINDS;
            ++kind) {
        if (binary_op_defs[kind].token_kind == token_kind) {
            if (def) {
                *def = binary_op_defs[kind];
            }
            return true;
        }
    }

    return false;
}

static Bang_Expr parse_bang_expr_with_precedence(Arena *arena, Bang_Lexer *lexer, Binary_Op_Prec prec)
{
    if (prec >= COUNT_BINARY_OP_PRECS) {
        return parse_primary_expr(arena, lexer);
    }

    Bang_Expr lhs = parse_bang_expr_with_precedence(arena, lexer, prec + 1);

    Bang_Token token = {0};
    Bang_Binary_Op_Def def = {0};

    while (bang_lexer_peek(lexer, &token, 0)
            && bang_binary_op_def_by_token(token.kind, &def)
            && def.prec == prec) {
        bool ok = bang_lexer_next(lexer, &token);
        assert(ok);

        Bang_Expr expr = {0};
        expr.loc = token.loc;
        expr.kind = BANG_EXPR_KIND_BINARY_OP;
        Bang_Binary_Op *binary_op = arena_alloc(arena, sizeof(*binary_op));
        {
            binary_op->loc  = token.loc;
            binary_op->kind = def.kind;
            binary_op->lhs  = lhs;
            binary_op->rhs  = parse_bang_expr_with_precedence(arena, lexer, prec + 1);
        }
        expr.as.binary_op = binary_op;

        lhs = expr;
    }

    return lhs;
}

Bang_Expr parse_bang_expr(Arena *arena, Bang_Lexer *lexer)
{
    return parse_bang_expr_with_precedence(arena, lexer, 0);
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
    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_EQ);
    var_assign.value = parse_bang_expr(arena, lexer);

    return var_assign;
}

Bang_Range parse_bang_range(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Range result = {0};
    result.low = parse_bang_expr(arena, lexer);
    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_DOT_DOT);
    result.high = parse_bang_expr(arena, lexer);
    return result;
}

Bang_For parse_bang_for(Arena *arena, Bang_Lexer *lexer)
{
    Bang_For result = {0};

    result.loc = bang_lexer_expect_keyword(lexer, SV("for")).loc;
    result.iter_name = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME).text;
    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_COLON);
    result.iter_type_name = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME).text;
    bang_lexer_expect_keyword(lexer, SV("in"));
    result.range = parse_bang_range(arena, lexer);
    result.body = parse_curly_bang_block(arena, lexer);
    return result;
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
        bang_diag_msg(bang_lexer_loc(lexer), BANG_DIAG_ERROR,
                      "expected statement but reached the end of the file");
        exit(1);
    }

    static_assert(
        COUNT_BANG_STMT_KINDS == 6,
        "The amount of statements have changed. "
        "Please update the parse_bang_stmt function to take that into account");

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
        } else if (sv_eq(token.text, SV("for"))) {
            Bang_Stmt stmt = {0};
            stmt.kind = BANG_STMT_KIND_FOR;
            stmt.as.forr = parse_bang_for(arena, lexer);
            return stmt;
        } else if (sv_eq(token.text, SV("var"))) {
            Bang_Stmt stmt = {0};
            stmt.kind = BANG_STMT_KIND_VAR_DEF;
            stmt.as.var_def = parse_bang_var_def(arena, lexer);
            return stmt;
        } else {
            Bang_Token next_token = {0};
            if (bang_lexer_peek(lexer, &next_token, 1) && next_token.kind == BANG_TOKEN_KIND_EQ) {
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
    case BANG_TOKEN_KIND_LT:
    case BANG_TOKEN_KIND_GE:
    case BANG_TOKEN_KIND_NE:
    case BANG_TOKEN_KIND_AND:
    case BANG_TOKEN_KIND_OR:
    case BANG_TOKEN_KIND_OPEN_PAREN:
    case BANG_TOKEN_KIND_CLOSE_PAREN:
    case BANG_TOKEN_KIND_OPEN_CURLY:
    case BANG_TOKEN_KIND_CLOSE_CURLY:
    case BANG_TOKEN_KIND_SEMICOLON:
    case BANG_TOKEN_KIND_COLON:
    case BANG_TOKEN_KIND_EQ:
    case BANG_TOKEN_KIND_EQ_EQ:
    case BANG_TOKEN_KIND_DOT_DOT:
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

Bang_Block parse_curly_bang_block(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Block result = {0};

    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_OPEN_CURLY);

    Bang_Token token = {0};
    while (bang_lexer_peek(lexer, &token, 0) &&
            token.kind != BANG_TOKEN_KIND_CLOSE_CURLY) {

        Bang_Stmt stmt = parse_bang_stmt(arena, lexer);
        DYNARRAY_PUSH(arena, result, stmt);
    }

    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_CLOSE_CURLY);

    return result;
}

Dynarray_Of_Bang_Proc_Param parse_bang_proc_params(Arena *arena, Bang_Lexer *lexer)
{
    Dynarray_Of_Bang_Proc_Param params = {0};

    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_OPEN_PAREN);

    Bang_Token token = {0};
    if (bang_lexer_peek(lexer, &token, 0) && token.kind == BANG_TOKEN_KIND_CLOSE_PAREN) {
        bang_lexer_next(lexer, &token);
        return params;
    }

    {
        Bang_Proc_Param param = {0};
        token = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME);
        param.loc = token.loc;
        param.name = token.text;
        bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_COLON);
        param.type_name = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME).text;

        DYNARRAY_PUSH(arena, params, param);
    }

    if (bang_lexer_peek(lexer, &token, 0) && token.kind == BANG_TOKEN_KIND_CLOSE_PAREN) {
        bang_lexer_next(lexer, &token);
        return params;
    }

    while (bang_lexer_peek(lexer, &token, 0) && token.kind == BANG_TOKEN_KIND_COMMA) {
        bang_lexer_next(lexer, &token);

        Bang_Proc_Param param = {0};
        token = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME);
        param.loc = token.loc;
        param.name = token.text;
        bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_COLON);
        param.type_name = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME).text;
        DYNARRAY_PUSH(arena, params, param);
    }

    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_CLOSE_PAREN);

    return params;
}

Bang_Proc_Def parse_bang_proc_def(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Proc_Def result = {0};

    result.loc = bang_lexer_expect_keyword(lexer, SV("proc")).loc;
    result.name = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME).text;
    result.params = parse_bang_proc_params(arena, lexer);
    result.body = parse_curly_bang_block(arena, lexer);

    return result;
}

Bang_Var_Def parse_bang_var_def(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Var_Def var_def = {0};

    var_def.loc = bang_lexer_expect_keyword(lexer, SV("var")).loc;
    var_def.name = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME).text;
    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_COLON);
    var_def.type_name = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME).text;

    {
        Bang_Token token = {0};
        if (bang_lexer_peek(lexer, &token, 0) && token.kind == BANG_TOKEN_KIND_EQ) {
            bang_lexer_next(lexer, &token);
            var_def.has_init = true;
            var_def.init = parse_bang_expr(arena, lexer);
        }
    }

    bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_SEMICOLON);

    return var_def;
}

Bang_Module parse_bang_module(Arena *arena, Bang_Lexer *lexer)
{
    Bang_Module module = {0};
    Bang_Token token = {0};

    while (bang_lexer_peek(lexer, &token, 0)) {
        if (token.kind != BANG_TOKEN_KIND_NAME) {
            bang_diag_msg(token.loc, BANG_DIAG_ERROR,
                          "expected token `%s` but got `%s`",
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
            top->as.var = parse_bang_var_def(arena, lexer);
        } else {
            static_assert(COUNT_BANG_TOP_KINDS == 2, "The error message below assumes that there is only two top level definition kinds");
            (void) BANG_TOP_KIND_PROC; // The error message below assumes there is a proc top level definition. Please update the message if needed.
            (void) BANG_TOP_KIND_VAR; // The error message below assumes there is a var top level definition. Please update the message if needed.
            bang_diag_msg(token.loc, BANG_DIAG_ERROR,
                          "expected top level module definition (proc or var) but got `"SV_Fmt"`",
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
