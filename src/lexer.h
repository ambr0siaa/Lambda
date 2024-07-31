#ifndef LEXER_H_
#define LEXER_H_

#include "sv.h"
#include "arena.h"
#include "types.h"

typedef enum {
    TK_NONE = 0,
    TK_NIL,
    TK_TEXT,
    TK_NUMBER,
    TK_STRING,
    TK_OPERATOR,
    TK_OPEN_PAREN,
    TK_CLOSE_PAREN,
} Token_Type;

typedef struct {
    size_t row, col;  // Place in line
    Token_Type type;  // Kind of token
    String_View text; // Content
    const char *file; // File placing
} Token;

#define TOKEN_NIL (Token) { .type = TK_NIL }
#define TOKEN_NONE (Token) {0}

typedef struct {
    int status;        // Used for yielding tokens
    size_t linenumber; // Number of current line
    String_View src;   // Source code
    char *linestart;   // Start of current line
    const char *file;  // From what file
} Lexer;

#define LEXSTATUS_OK 1
#define LEXSTATUS_ERR 0
#define LEXSTATUS_EMPTY 2

#define lexstatus_err(L) ((L)->status == LEXSTATUS_ERR)
#define lexempty(L) ((L)->status == LEXSTATUS_EMPTY)

LAM_API Lexer lexer_new(const char *file_path, String_View src);

LAM_API Token lexer_next(Lexer *L);
LAM_API Token lexer_peek(Lexer *L);
LAM_API Token lexer_yield(Lexer *L, Token_Type t);

LAM_API void token_dump(Token tk);
LAM_API void lexer_dump(Lexer lex);

#endif // LEXER_H_
