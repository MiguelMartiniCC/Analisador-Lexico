#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    TOKEN_KEYWORD,
    TOKEN_ID,
    TOKEN_INT,
    TOKEN_OP_ARITH,
    TOKEN_OP_REL,
    TOKEN_ASSIGN,
    TOKEN_PUNCT,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[64];
    int line;
} Token;

const char* tokenTypeToString(TokenType type);

#endif