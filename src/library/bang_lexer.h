#ifndef BANG_LEXER_H_
#define BANG_LEXER_H_

#include <stdlib.h>

#include "./sv.h"

typedef enum {
    BANG_TOKEN_TYPE_NAME,
    BANG_TOKEN_TYPE_OPEN_PAREN,
    BANG_TOKEN_TYPE_CLOSE_PAREN,
    BANG_TOKEN_TYPE_OPEN_CURLY,
    BANG_TOKEN_TYPE_CLOSE_CURLY,
    BANG_TOKEN_TYPE_SEMICOLON,
    BANG_TOKEN_TYPE_LIT_STR,
} Bang_Token_Type;

const char *bang_token_type_name(Bang_Token_Type type);

typedef struct {
    size_t row;
    size_t col;
    const char *file_path;
} Bang_Loc;

#define Bang_Loc_Fmt "%s:%zu:%zu"
#define Bang_Loc_Arg(loc) (loc).file_path, (loc).row, (loc).col

typedef struct {
    Bang_Token_Type type;
    String_View text;
    Bang_Loc loc;
} Bang_Token;

typedef struct {
    String_View content;
    const char *line_start;
    String_View line;
    const char *file_path;
    size_t row;
} Bang_Lexer;

Bang_Lexer bang_lexer_from_sv(String_View content, const char *file_path);
// TODO: lexer_peek is not implement
void bang_lexer_next_line(Bang_Lexer *lexer);
Bang_Loc bang_lexer_loc(Bang_Lexer *lexer);
Bang_Token bang_lexer_spit_token(Bang_Lexer *lexer, Bang_Token_Type token_type, size_t token_size);
bool bang_is_name(char x);
bool bang_lexer_next(Bang_Lexer *lexer, Bang_Token *token);

#endif // BANG_LEXER_H_
