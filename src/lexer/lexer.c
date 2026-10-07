#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "lexer.h"
#include "symbol_table.h"

const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TOKEN_KEYWORD:  return "PALAVRA_RESERVADA";
        case TOKEN_ID:       return "IDENTIFICADOR";
        case TOKEN_INT:      return "INTEIRO";
        case TOKEN_OP_ARITH: return "OP_ARITMETICO";
        case TOKEN_OP_REL:   return "OP_RELACIONAL";
        case TOKEN_ASSIGN:   return "ATRIBUICAO";
        case TOKEN_PUNCT:    return "PONTUACAO";
        case TOKEN_EOF:      return "FIM_DE_FICHEIRO";
        default:             return "ERRO";
    }
}

void initLexer(Lexer *lexer, FILE *file) {
    lexer->file = file;
    lexer->currentLine = 1;
}

Token getNextToken(Lexer *lexer) {
    Token token;
    int c = fgetc(lexer->file);

    // Ignorar espaços em branco e quebras de linha
    while (c != EOF && isspace(c)) {
        if (c == '\n') {
            lexer->currentLine++;
        }
        c = fgetc(lexer->file);
    }

    if (c == EOF) {
        token.type = TOKEN_EOF;
        strcpy(token.lexeme, "EOF");
        token.line = lexer->currentLine;
        return token;
    }

    // Identificadores e Palavras Reservadas
    if (isalpha(c) || c == '_') {
        int i = 0;
        while (isalnum(c) || c == '_') {
            if (i < 63) token.lexeme[i++] = c;
            c = fgetc(lexer->file);
        }
        token.lexeme[i] = '\0';
        ungetc(c, lexer->file);

        if (strcmp(token.lexeme, "if") == 0 || 
            strcmp(token.lexeme, "else") == 0 || 
            strcmp(token.lexeme, "int") == 0 ||
            strcmp(token.lexeme, "while") == 0) {
            token.type = TOKEN_KEYWORD;
        } else {
            token.type = TOKEN_ID;
            insertSymbol(token.lexeme, TOKEN_ID);
        }
        token.line = lexer->currentLine;
        return token;
    }

    // Números Inteiros
    if (isdigit(c)) {
        int i = 0;
        while (isdigit(c)) {
            if (i < 63) token.lexeme[i++] = c;
            c = fgetc(lexer->file);
        }
        token.lexeme[i] = '\0';
        ungetc(c, lexer->file);

        token.type = TOKEN_INT;
        token.line = lexer->currentLine;
        return token;
    }

    // Operadores Relacionais e Atribuição
    if (c == '=') {
        int next = fgetc(lexer->file);
        if (next == '=') {
            strcpy(token.lexeme, "==");
            token.type = TOKEN_OP_REL;
        } else {
            ungetc(next, lexer->file);
            strcpy(token.lexeme, "=");
            token.type = TOKEN_ASSIGN;
        }
        token.line = lexer->currentLine;
        return token;
    }

    // Operadores Aritméticos
    if (strchr("+-*/", c)) {
        token.lexeme[0] = c;
        token.lexeme[1] = '\0';
        token.type = TOKEN_OP_ARITH;
        token.line = lexer->currentLine;
        return token;
    }

    // Pontuação
    if (strchr(";,(){}", c)) {
        token.lexeme[0] = c;
        token.lexeme[1] = '\0';
        token.type = TOKEN_PUNCT;
        token.line = lexer->currentLine;
        return token;
    }

    // Caractere não reconhecido
    token.lexeme[0] = c;
    token.lexeme[1] = '\0';
    token.type = TOKEN_ERROR;
    token.line = lexer->currentLine;
    return token;
}