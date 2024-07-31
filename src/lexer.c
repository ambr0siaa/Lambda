#include "lexer.h"

Lexer lexer_new(const char *file_path, String_View src)
{
    Lexer L = {0};
    
    L.src = src;
    L.file = file_path;
    L.linestart = src.data;
    L.linenumber = 1;
    L.status = LEXSTATUS_OK;

    return L; 
}

static inline void lexer_comments(String_View *src)
{
    size_t i = 0;
    sv_cut_left(src, 1);

    while (i < src->count &&
            src->data[i] != '\n') ++i;
    
    sv_cut_left(src, i);
}

static inline String_View lexer_string(String_View *src)
{
    size_t i = 0;
    sv_cut_left(src, 1);

    while ((src->data[i] != '"' && src->data[i] != '\'') &&
           (i < src->count)) ++i;
    
    String_View result = sv_from_parts(src->data, i);
    sv_cut_left(src, i + 1);

    return result;
}

static inline void lexer_space(Lexer *L)
{
    size_t i = 0;

    while (i < L->src.count && isspace(L->src.data[i])) {
        if (L->src.data[i] == '\n') {
            L->linestart = L->src.data + 1;
            L->linenumber += 1;
        }
        ++i;
    }

    sv_cut_left(&L->src, i);
}

static inline int lexer_char(String_View *src, Token *tk)
{
    int status = 1;
    size_t shift = 1;

    switch (src->data[0]) {
        case '+':
        case '-':
        case '*':
        case '/':
        case '^':
        case '%': {
            tk->type = TK_OPERATOR;
            break;
        }

        case ';': {
            lexer_comments(src);
            status = -1;
            shift = 0;
            break;
        }

        case '(': {
            tk->type = TK_OPEN_PAREN;
            break;
        }

        case ')': {
            tk->type = TK_CLOSE_PAREN;
            break;
        }

        case '\'':
        case '"': {
            tk->type = TK_STRING;
            tk->text = lexer_string(src);
            shift = 0;
            break;
        }

        case '\000': {
            status = -1;
            shift = 0;
            break;
        }

        default: {
            status = 0;
            shift = 0;
            break;
        }
    }
    
    if (shift == 1) {
        tk->text = sv_from_parts(src->data, 1);
        sv_cut_left(src, shift);
    }

    return status;
}

Token lexer_next(Lexer *L)
{
    Token tk = TOKEN_NONE;
    if (L->src.count <= 0) {
        L->status = LEXSTATUS_EMPTY;
        goto defer;
    }

    lexer_space(L);
    
    tk.row = L->linenumber;
    tk.col = (size_t)(L->src.data - L->linestart) + 1;

    if (L->src.count > 0) {
        if (isdigit(L->src.data[0])) {
            tk.text = sv_cut_value(&L->src);
            tk.type = TK_NUMBER;

        } else {
            if (!lexer_char(&L->src, &tk)) {
                tk.text = sv_cut_txt(&L->src);
                tk.type = TK_TEXT;
            }
        }
    }

defer:
    return tk;
}

Token lexer_peek(Lexer *L)
{
    Lexer copy = *L;
    Token tk = lexer_next(L);
    *L = copy;
    return tk;
}

Token lexer_yield(Lexer *L, Token_Type t)
{
    Token tk = lexer_next(L);

    if (tk.type != t) {
        fprintf(stderr, "Expected %u, but provided %u\n", t, tk.type);
        L->status = LEXSTATUS_ERR;
    }

    return tk;
}

void token_dump(Token tk)
{
    printf("[row: %zu, col: %zu] ", tk.row, tk.col);

    switch (tk.type) {
        case TK_NUMBER: {
            printf("number ["SV_Fmt"]", SV_Args(tk.text));
            break;
        }

        case TK_TEXT: {
            printf("text ["SV_Fmt"]", SV_Args(tk.text));
            break;
        }

        case TK_STRING: {
            printf("string ["SV_Fmt"]", SV_Args(tk.text));
            break;
        }

        case TK_OPERATOR:
        case TK_OPEN_PAREN:
        case TK_CLOSE_PAREN: {
            printf("char ["SV_Fmt"]", SV_Args(tk.text));
            break;
        }

        case TK_NIL: {
            printf("nil");
            break;
        }

        default: {
            fprintf(stderr, "Unknown token type %u\n", tk.type);
            exit(1);
        }
    }

    printf("\n");
}

void lexer_dump(Lexer lex)
{
    while (1) {
        Token tk = lexer_next(&lex);
        if (lexempty(&lex)) break;
        token_dump(tk);
    }
}
