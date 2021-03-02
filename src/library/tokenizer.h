#ifndef TOKENIZER_H_
#define TOKENIZER_H_

#include "sv.h"
#include "fl.h"

typedef enum {
    TOKEN_KIND_STR,
    TOKEN_KIND_CHAR,
    TOKEN_KIND_PLUS,
    TOKEN_KIND_MINUS,
    TOKEN_KIND_MULT,
    TOKEN_KIND_NUMBER,
    TOKEN_KIND_NAME,
    TOKEN_KIND_OPEN_PAREN,
    TOKEN_KIND_CLOSING_PAREN,
    TOKEN_KIND_COMMA,
    TOKEN_KIND_GT,
} Token_Kind;

const char *token_kind_name(Token_Kind kind);

typedef struct {
    Token_Kind kind;
    String_View text;
} Token;

#define Token_Fmt "%s"
#define Token_Arg(token) token_kind_name((token).kind)

typedef struct {
    String_View source;
    Token peek_buffer;
    bool peek_buffer_full;
} Tokenizer;

void expect_no_tokens(Tokenizer *tokenizer, File_Location location);

bool tokenizer_next(Tokenizer *tokenizer, Token *token, File_Location location);
bool tokenizer_peek(Tokenizer *tokenizer, Token *token, File_Location location);
Tokenizer tokenizer_from_sv(String_View source);

#endif // TOKENIZER_H_
