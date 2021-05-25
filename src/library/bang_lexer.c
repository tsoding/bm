#include <assert.h>
#include <stdio.h>
#include "./bang_lexer.h"

typedef struct {
    String_View text;
    Bang_Token_Type type;
} Hardcoded_Token;

static const Hardcoded_Token hardcoded_bang_tokens[] = {
    {.type = BANG_TOKEN_TYPE_OPEN_PAREN,  .text = SV_STATIC("(")},
    {.type = BANG_TOKEN_TYPE_CLOSE_PAREN, .text = SV_STATIC(")")},
    {.type = BANG_TOKEN_TYPE_OPEN_CURLY,  .text = SV_STATIC("{")},
    {.type = BANG_TOKEN_TYPE_CLOSE_CURLY, .text = SV_STATIC("}")},
    {.type = BANG_TOKEN_TYPE_SEMICOLON,   .text = SV_STATIC(";")},
};
static const size_t hardcoded_bang_tokens_count = sizeof(hardcoded_bang_tokens) / sizeof(hardcoded_bang_tokens[0]);

const char *bang_token_type_name(Bang_Token_Type type)
{
    switch (type) {
    case BANG_TOKEN_TYPE_NAME:
        return "BANG_TOKEN_TYPE_NAME";
    case BANG_TOKEN_TYPE_OPEN_PAREN:
        return "BANG_TOKEN_TYPE_OPEN_PAREN";
    case BANG_TOKEN_TYPE_CLOSE_PAREN:
        return "BANG_TOKEN_TYPE_CLOSE_PAREN";
    case BANG_TOKEN_TYPE_OPEN_CURLY:
        return "BANG_TOKEN_TYPE_OPEN_CURLY";
    case BANG_TOKEN_TYPE_CLOSE_CURLY:
        return "BANG_TOKEN_TYPE_CLOSE_CURLY";
    case BANG_TOKEN_TYPE_SEMICOLON:
        return "BANG_TOKEN_TYPE_SEMICOLON";
    case BANG_TOKEN_TYPE_LIT_STR:
        return "BANG_TOKEN_TYPE_LIT_STR";
    default:
        assert(false && "token_type_name: unreachable");
        exit(1);
    }
}

Bang_Lexer bang_lexer_from_sv(String_View content, const char *file_path)
{
    return (Bang_Lexer) {
        .content = content,
        .file_path = file_path,
    };
}

void bang_lexer_next_line(Bang_Lexer *lexer)
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

Bang_Token bang_lexer_spit_token(Bang_Lexer *lexer, Bang_Token_Type token_type, size_t token_size)
{
    assert(token_size <= lexer->line.count);
    Bang_Token token = {0};
    token.type = token_type;
    token.loc = bang_lexer_loc(lexer);
    token.text = sv_chop_left(&lexer->line, token_size);
    return token;
}

bool bang_is_name(char x)
{
    return ('a' <= x && x <= 'z') ||
           ('A' <= x && x <= 'Z') ||
           ('0' <= x && x <= '9') ||
           x == '_';
}

bool bang_lexer_next(Bang_Lexer *lexer, Bang_Token *token)
{
    lexer->line = sv_trim_left(lexer->line);
    while (lexer->line.count == 0 && lexer->content.count > 0) {
        bang_lexer_next_line(lexer);
        lexer->line = sv_trim_left(lexer->line);
    }

    if (lexer->line.count == 0) {
        return false;
    }

    // Hardcoded Tokens
    for (size_t i = 0; i < hardcoded_bang_tokens_count; ++i) {
        if (sv_starts_with(lexer->line, hardcoded_bang_tokens[i].text)) {
            *token = bang_lexer_spit_token(
                         lexer,
                         hardcoded_bang_tokens[i].type,
                         hardcoded_bang_tokens[i].text.count);
            return true;
        }
    }

    // Name token
    {
        size_t n = 0;
        while (n < lexer->line.count && bang_is_name(lexer->line.data[n])) {
            n += 1;
        }
        if (n > 0) {
            *token = bang_lexer_spit_token(lexer, BANG_TOKEN_TYPE_NAME, n);
            return true;
        }
    }

    // String Literal
    if (lexer->line.data[0] == '"') {
        size_t n = 1;
        // TODO: lexer does not support new lines inside of the string literals
        while (n < lexer->line.count && lexer->line.data[n] != '\"') {
            n += 1;
        }

        if (n >= lexer->line.count) {
            const Bang_Loc unclosed_loc = bang_lexer_loc(lexer);
            fprintf(stderr, Bang_Loc_Fmt": ERROR: unclosed string literal",
                    Bang_Loc_Arg(unclosed_loc));
            exit(1);
        }

        assert(lexer->line.data[n] == '\"');
        n += 1;

        *token = bang_lexer_spit_token(lexer, BANG_TOKEN_TYPE_LIT_STR, n);
        return true;
    }

    const Bang_Loc unknown_loc = bang_lexer_loc(lexer);
    fprintf(stderr, Bang_Loc_Fmt": ERROR: unknown token starts with `%c`",
            Bang_Loc_Arg(unknown_loc), *lexer->line.data);
    exit(1);
}
