#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include "./tokenizer.h"

static bool is_name(char x)
{
    return isalnum(x) || x == '_';
}

static bool is_number(char x)
{
    return isalnum(x) || x == '.';
}

bool tokenizer_peek(Tokenizer *tokenizer, Token *output, File_Location location)
{
    if (tokenizer->peek_buffer_full) {
        if (output) {
            *output = tokenizer->peek_buffer;
        }
        return true;
    }

    tokenizer->source = sv_trim_left(tokenizer->source);

    if (tokenizer->source.count == 0) {
        return false;
    }

    Token token = {0};
    switch (*tokenizer->source.data) {
    case '(': {
        token.kind = TOKEN_KIND_OPEN_PAREN;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case ')': {
        token.kind = TOKEN_KIND_CLOSING_PAREN;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case ',': {
        token.kind = TOKEN_KIND_COMMA;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case '>': {
        token.kind = TOKEN_KIND_GT;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case '*': {
        token.kind = TOKEN_KIND_MULT;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case '+': {
        token.kind = TOKEN_KIND_PLUS;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case '-': {
        token.kind = TOKEN_KIND_MINUS;
        token.text = sv_chop_left(&tokenizer->source, 1);
    }
    break;

    case '=': {
        if (tokenizer->source.count <= 1 || tokenizer->source.data[1] != '=') {
            fprintf(stderr, FL_Fmt": ERROR: Unknown token starts with =\n",
                    FL_Arg(location));
            exit(1);
        }
        token.kind = TOKEN_KIND_EE;
        token.text = sv_chop_left(&tokenizer->source, 2);
    }
    break;

    case '"': {
        sv_chop_left(&tokenizer->source, 1);

        size_t index = 0;

        if (sv_index_of(tokenizer->source, '"', &index)) {
            // TODO(#66): string literals don't support escaped characters
            String_View text = sv_chop_left(&tokenizer->source, index);
            sv_chop_left(&tokenizer->source, 1);
            token.kind = TOKEN_KIND_STR;
            token.text = text;
        } else {
            fprintf(stderr, FL_Fmt": ERROR: Could not find closing \"\n",
                    FL_Arg(location));
            exit(1);
        }
    }
    break;

    case '\'': {
        sv_chop_left(&tokenizer->source, 1);

        size_t index = 0;

        if (sv_index_of(tokenizer->source, '\'', &index)) {
            String_View text = sv_chop_left(&tokenizer->source, index);
            sv_chop_left(&tokenizer->source, 1);
            token.kind = TOKEN_KIND_CHAR;
            token.text = text;
        } else {
            fprintf(stderr, FL_Fmt": ERROR: Could not find closing \'\n",
                    FL_Arg(location));
            exit(1);
        }
    }
    break;

    default: {
        if (isalpha(*tokenizer->source.data)) {
            token.kind = TOKEN_KIND_NAME;
            token.text = sv_chop_left_while(&tokenizer->source, is_name);
        } else if (isdigit(*tokenizer->source.data)) {
            token.kind = TOKEN_KIND_NUMBER;
            token.text = sv_chop_left_while(&tokenizer->source, is_number);
        } else {
            fprintf(stderr, FL_Fmt": ERROR: Unknown token starts with %c\n",
                    FL_Arg(location), *tokenizer->source.data);
            exit(1);
        }
    }
    }

    tokenizer->peek_buffer_full = true;
    tokenizer->peek_buffer = token;

    if (output) {
        *output = token;
    }

    return true;
}

bool tokenizer_next(Tokenizer *tokenizer, Token *token, File_Location location)
{
    if (tokenizer_peek(tokenizer, token, location)) {
        tokenizer->peek_buffer_full = false;
        return true;
    }

    return false;
}

Tokenizer tokenizer_from_sv(String_View source)
{
    return (Tokenizer) {
        .source = source
    };
}

void expect_no_tokens(Tokenizer *tokenizer, File_Location location)
{
    Token token = {0};
    if (tokenizer_next(tokenizer, &token, location)) {
        fprintf(stderr, FL_Fmt": ERROR: unexpected token `"SV_Fmt"`\n",
                FL_Arg(location), SV_Arg(token.text));
        exit(1);
    }
}
