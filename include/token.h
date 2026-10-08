#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    //Identificadores
    TOKEN_ID,
    TOKEN_NUM_INT,
    TOKEN_NUM_DEC,
    TOKEN_TEXT,

    //Palavras reservadas

    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_CHAR,
    TOKEN_BOOLEAN,
    TOKEN_VOID,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_SCANF,
    TOKEN_PRINTLN,
    TOKEN_MAIN,
    TOKEN_RETURN,
    TOKEN_FUNC,
    
    //Operadores

    TOKEN_ATRIB,
    TOKEN_SUM,
    TOKEN_SUB,
    TOKEN_MULT,
    TOKEN_DIV,
    TOKEN_MOD,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_COMP,

    //Simbolos especiais
    TOKEN_OPEN_PAREN, 
    TOKEN_CLOSE_PAREN,
    TOKEN_OPEN_BRACKET, //[
    TOKEN_CLOSE_BRACKET, //]
    TOKEN_OPEN_BRACE,   //{
    TOKEN_CLOSE_BRACE,  //}
    TOKEN_COMMA,    // ,
    TOKEN_SEMICOLON, // ;

    //Controle

    TOKEN_EOF,
    TOKEN_ERROR

} TokenType;

typedef struct {
    TokenType type;
    char lexeme[256];
    int line;
} Token;

const char* tokenTypeToString(TokenType type);

#endif