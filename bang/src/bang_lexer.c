#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include "./bang_lexer.h"

typedef struct {
    String_View text;
    Bang_Token_Kind kind;
} Hardcoded_Token;

static const Hardcoded_Token hardcoded_bang_tokens[] = {
    {.kind = BANG_TOKEN_KIND_DOT_DOT,     .text = SV_STATIC("..")},
    {.kind = BANG_TOKEN_KIND_EQ_EQ,       .text = SV_STATIC("==")},
    {.kind = BANG_TOKEN_KIND_OR,          .text = SV_STATIC("||")},
    {.kind = BANG_TOKEN_KIND_AND,         .text = SV_STATIC("&&")},
    {.kind = BANG_TOKEN_KIND_NE,          .text = SV_STATIC("!=")},
    {.kind = BANG_TOKEN_KIND_GE,          .text = SV_STATIC(">=")},
    {.kind = BANG_TOKEN_KIND_OPEN_PAREN,  .text = SV_STATIC("(")},
    {.kind = BANG_TOKEN_KIND_CLOSE_PAREN, .text = SV_STATIC(")")},
    {.kind = BANG_TOKEN_KIND_OPEN_CURLY,  .text = SV_STATIC("{")},
    {.kind = BANG_TOKEN_KIND_CLOSE_CURLY, .text = SV_STATIC("}")},
    {.kind = BANG_TOKEN_KIND_SEMICOLON,   .text = SV_STATIC(";")},
    {.kind = BANG_TOKEN_KIND_COLON,       .text = SV_STATIC(":")},
    {.kind = BANG_TOKEN_KIND_COMMA,       .text = SV_STATIC(",")},
    {.kind = BANG_TOKEN_KIND_EQ,          .text = SV_STATIC("=")},
    {.kind = BANG_TOKEN_KIND_PLUS,        .text = SV_STATIC("+")},
    {.kind = BANG_TOKEN_KIND_MINUS,       .text = SV_STATIC("-")},
    {.kind = BANG_TOKEN_KIND_MULT,        .text = SV_STATIC("*")},
    {.kind = BANG_TOKEN_KIND_LT,          .text = SV_STATIC("<")},
};
// IMPORTANT! Please keep this array sorted by the length of the tokens in the descending order
static const size_t hardcoded_bang_tokens_count = sizeof(hardcoded_bang_tokens) / sizeof(hardcoded_bang_tokens[0]);
static_assert(COUNT_BANG_TOKEN_KINDS == 21, "The amount of token kinds have changed. Make sure you don't need to add anything new to the list of the hardcoded tokens");

static const char *const token_kind_names[COUNT_BANG_TOKEN_KINDS] = {
    [BANG_TOKEN_KIND_NAME]        = "name",
    [BANG_TOKEN_KIND_NUMBER]      = "number",
    [BANG_TOKEN_KIND_OPEN_PAREN]  = "(",
    [BANG_TOKEN_KIND_CLOSE_PAREN] = ")",
    [BANG_TOKEN_KIND_OPEN_CURLY]  = "{",
    [BANG_TOKEN_KIND_CLOSE_CURLY] = "}",
    [BANG_TOKEN_KIND_SEMICOLON]   = ";",
    [BANG_TOKEN_KIND_COLON]       = ":",
    [BANG_TOKEN_KIND_COMMA]       = ",",
    [BANG_TOKEN_KIND_EQ]          = "=",
    [BANG_TOKEN_KIND_EQ_EQ]       = "==",
    [BANG_TOKEN_KIND_PLUS]        = "+",
    [BANG_TOKEN_KIND_MINUS]       = "-",
    [BANG_TOKEN_KIND_MULT]        = "*",
    [BANG_TOKEN_KIND_LT]          = "<",
    [BANG_TOKEN_KIND_GE]          = ">=",
    [BANG_TOKEN_KIND_NE]          = "!=",
    [BANG_TOKEN_KIND_AND]         = "&&",
    [BANG_TOKEN_KIND_OR]          = "||",
    [BANG_TOKEN_KIND_DOT_DOT]     = "..",
    [BANG_TOKEN_KIND_LIT_STR]     = "string literal",
};
static_assert(COUNT_BANG_TOKEN_KINDS == 21, "The amount of token kinds have changed. Please update the table of token kind names. Thanks!");

const char *bang_token_kind_name(Bang_Token_Kind kind)
{
    assert(0 <= kind);
    assert(kind < COUNT_BANG_TOKEN_KINDS);
    return token_kind_names[kind];
}

Bang_Lexer bang_lexer_from_sv(String_View content, const char *file_path)
{
    return (Bang_Lexer) {
        .content = content,
        .file_path = file_path,
    };
}

static void bang_lexer_next_line(Bang_Lexer *lexer)
{
    lexer->line = sv_chop_by_delim(&lexer->content, '\n');
    lexer->row += 1;
    lexer->line_start = lexer->line.data;
}

Bang_Loc bang_lexer_loc(Bang_Lexer *lexer)
{
    Bang_Loc result = {0};
    result.row = lexer->row;
    assert(lexer->line_start <= lexer->line.data);
    result.col = (size_t) (lexer->line.data - lexer->line_start + 1);
    result.file_path = lexer->file_path;
    return result;
}

static Bang_Token bang_lexer_spit_token(Bang_Lexer *lexer, Bang_Token_Kind token_kind, size_t token_size)
{
    assert(token_size <= lexer->line.count);

    Bang_Token token = {0};
    token.kind = token_kind;
    token.loc = bang_lexer_loc(lexer);
    token.text = sv_chop_left(&lexer->line, token_size);
    return token;
}

static bool bang_is_name(char x)
{
    return ('a' <= x && x <= 'z') ||
           ('A' <= x && x <= 'Z') ||
           ('0' <= x && x <= '9') ||
           x == '_';
}

static bool bang_lexer_next_token_bypassing_peek_buffer(Bang_Lexer *lexer, Bang_Token *token)
{
    // Extracting next token
    {
        lexer->line = sv_trim_left(lexer->line);
        while ((lexer->line.count == 0 || *lexer->line.data == '#') && lexer->content.count > 0) {
            bang_lexer_next_line(lexer);
            lexer->line = sv_trim_left(lexer->line);
        }

        if (lexer->line.count == 0) {
            return false;
        }
    }

    static_assert(
        COUNT_BANG_TOKEN_KINDS == 21,
        "The amount of token kinds have changed. "
        "Please update the lexer to take that into account. "
        "If you updated the hardcoded tokens nothing here needs to be changed. "
        "Just update the counter and move on. "
        "Thanks!");

    // Hardcoded Tokens
    for (size_t i = 0; i < hardcoded_bang_tokens_count; ++i) {
        if (sv_starts_with(lexer->line, hardcoded_bang_tokens[i].text)) {
            *token = bang_lexer_spit_token(
                         lexer,
                         hardcoded_bang_tokens[i].kind,
                         hardcoded_bang_tokens[i].text.count);
            return true;
        }
    }

    // Name or Number token
    {
        size_t n = 0;
        while (n < lexer->line.count && bang_is_name(lexer->line.data[n])) {
            n += 1;
        }
        if (n > 0) {
            if (isdigit(*lexer->line.data)) {
                *token = bang_lexer_spit_token(lexer, BANG_TOKEN_KIND_NUMBER, n);
            } else {
                *token = bang_lexer_spit_token(lexer, BANG_TOKEN_KIND_NAME, n);
            }
            return true;
        }
    }

    // String Literal
    if (lexer->line.data[0] == '"') {
        size_t n = 1;
        // TODO(#399): lexer does not support new lines inside of the string literals
        while (n < lexer->line.count && lexer->line.data[n] != '\"') {
            n += 1;
        }

        if (n >= lexer->line.count) {
            bang_diag_msg(bang_lexer_loc(lexer), BANG_DIAG_ERROR,
                          "unclosed string_literal");
            exit(1);
        }

        assert(lexer->line.data[n] == '\"');
        n += 1;

        *token = bang_lexer_spit_token(lexer, BANG_TOKEN_KIND_LIT_STR, n);
        return true;
    }

    // Unknown token
    bang_diag_msg(bang_lexer_loc(lexer), BANG_DIAG_ERROR,
                  "unknown token starts with `%c`",
                  *lexer->line.data);
    exit(1);
}

static void bang_lexer_refill_peek_buffer(Bang_Lexer *lexer)
{
    Bang_Token token = {0};
    while (lexer->peek_buffer.count < BANG_TOKEN_BUFFER_CAPACITY &&
            bang_lexer_next_token_bypassing_peek_buffer(lexer, &token)) {
        bang_token_buffer_nq(&lexer->peek_buffer, token);
    }
}

bool bang_lexer_peek(Bang_Lexer *lexer, Bang_Token *token, size_t offset)
{
    bang_lexer_refill_peek_buffer(lexer);

    assert(offset < BANG_TOKEN_BUFFER_CAPACITY);
    if (offset < lexer->peek_buffer.count) {
        *token = bang_token_buffer_get(&lexer->peek_buffer, offset);
        return true;
    }

    return false;
}

Bang_Token bang_lexer_expect_token(Bang_Lexer *lexer, Bang_Token_Kind expected_kind)
{
    Bang_Token token = {0};

    if (!bang_lexer_next(lexer, &token)) {
        bang_diag_msg(bang_lexer_loc(lexer), BANG_DIAG_ERROR,
                      "expected token `%s` but reached the end of the file",
                      bang_token_kind_name(expected_kind));
        exit(1);
    }

    if (token.kind != expected_kind) {
        bang_diag_msg(token.loc, BANG_DIAG_ERROR,
                      "expected token `%s` but got `%s`",
                      bang_token_kind_name(expected_kind),
                      bang_token_kind_name(token.kind));
        exit(1);
    }

    return token;
}

Bang_Token bang_lexer_expect_keyword(Bang_Lexer *lexer, String_View name)
{
    Bang_Token token = bang_lexer_expect_token(lexer, BANG_TOKEN_KIND_NAME);
    if (!sv_eq(token.text, name)) {
        bang_diag_msg(token.loc, BANG_DIAG_ERROR,
                      "expected keyword `"SV_Fmt"` but got `"SV_Fmt"`",
                      SV_Arg(name),
                      SV_Arg(token.text));
        exit(1);
    }

    return token;
}

bool bang_lexer_next(Bang_Lexer *lexer, Bang_Token *token)
{
    if (!bang_lexer_peek(lexer, token, 0)) {
        return false;
    }

    bang_token_buffer_dq(&lexer->peek_buffer);
    return true;
}

void bang_token_buffer_nq(Bang_Token_Buffer *buffer, Bang_Token token)
{
    assert(buffer->count < BANG_TOKEN_BUFFER_CAPACITY);
    const size_t internal_index = (buffer->begin + buffer->count) % BANG_TOKEN_BUFFER_CAPACITY;
    buffer->tokens[internal_index] = token;
    buffer->count += 1;
}

Bang_Token bang_token_buffer_dq(Bang_Token_Buffer *buffer)
{
    assert(buffer->count > 0);
    const size_t internal_index = (buffer->begin + buffer->count - 1) % BANG_TOKEN_BUFFER_CAPACITY;
    const Bang_Token result = buffer->tokens[internal_index];
    buffer->begin = (buffer->begin + 1) % BANG_TOKEN_BUFFER_CAPACITY;
    buffer->count -= 1;
    return result;
}

Bang_Token bang_token_buffer_get(const Bang_Token_Buffer *buffer, size_t user_index)
{
    assert(user_index < buffer->count);
    const size_t internal_index = (buffer->begin + user_index) % BANG_TOKEN_BUFFER_CAPACITY;
    return buffer->tokens[internal_index];
}
