#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "lexer.h"
#include "symbol_table.h"

#define MAX_LEXEME 255

// Tabela de palavras reservadas: lexema -> tipo do token 
typedef struct {
    const char *word;
    TokenType type;
} Keyword;

static const Keyword keywords[] = {
    {"int",     TOKEN_INT},
    {"float",   TOKEN_FLOAT},
    {"char",    TOKEN_CHAR},
    {"boolean", TOKEN_BOOLEAN},
    {"void",    TOKEN_VOID},
    {"if",      TOKEN_IF},
    {"else",    TOKEN_ELSE},
    {"for",     TOKEN_FOR},
    {"while",   TOKEN_WHILE},
    {"scanf",   TOKEN_SCANF},
    {"println", TOKEN_PRINTLN},
    {"main",    TOKEN_MAIN},
    {"return",  TOKEN_RETURN}
};

static const int keywordCount = sizeof(keywords) / sizeof(keywords[0]);

const char* tokenTypeToString(TokenType type) {
    switch (type) {
       case TOKEN_ID:            return "ID";
        case TOKEN_NUM_INT:       return "NUM_INT";
        case TOKEN_NUM_DEC:       return "NUM_DEC";
        case TOKEN_TEXT:          return "TEXTO";
        case TOKEN_INT:           return "INT";
        case TOKEN_FLOAT:         return "FLOAT";
        case TOKEN_CHAR:          return "CHAR";
        case TOKEN_BOOLEAN:       return "BOOLEAN";
        case TOKEN_VOID:          return "VOID";
        case TOKEN_IF:            return "IF";
        case TOKEN_ELSE:          return "ELSE";
        case TOKEN_FOR:           return "FOR";
        case TOKEN_WHILE:         return "WHILE";
        case TOKEN_SCANF:         return "SCANF";
        case TOKEN_PRINTLN:       return "PRINTLN";
        case TOKEN_MAIN:          return "MAIN";
        case TOKEN_RETURN:        return "RETURN";
        case TOKEN_ATRIB:         return "=";
        case TOKEN_SUM:           return "+";
        case TOKEN_SUB:           return "-";
        case TOKEN_MULT:          return "*";
        case TOKEN_DIV:           return "/";
        case TOKEN_MOD:           return "%";
        case TOKEN_AND:           return "&&";
        case TOKEN_OR:            return "||";
        case TOKEN_NOT:           return "!";
        case TOKEN_COMP:          return "COMP";
        case TOKEN_OPEN_PAREN:    return "(";
        case TOKEN_CLOSE_PAREN:   return ")";
        case TOKEN_OPEN_BRACKET:  return "[";
        case TOKEN_CLOSE_BRACKET: return "]";
        case TOKEN_OPEN_BRACE:    return "{";
        case TOKEN_CLOSE_BRACE:   return "}";
        case TOKEN_COMMA:         return ",";
        case TOKEN_SEMICOLON:     return ";";
        case TOKEN_EOF:           return "FIM_ARQUIVO";
        default:                  return "ERRO";
    }
}

void initLexer(Lexer *lexer, FILE *file) {
    lexer->file = file;
    lexer->currentLine = 1;
}

// Monta um token pronto a partir de tipo, lexema e linha 
static Token makeToken(TokenType type, const char *lexeme, int line) {
    Token token;
    token.type = type;
    strncpy(token.lexeme, lexeme, sizeof(token.lexeme) - 1);
    token.lexeme[sizeof(token.lexeme) - 1] = '\0';
    token.line = line;
    return token;
}

// Devolve o tipo da palavra reservada, ou TOKEN_ID se não for uma
static TokenType lookupKeyword(const char *lexeme) {
    for (int i = 0; i < keywordCount; i++) {
        if (strcmp(lexeme, keywords[i].word) == 0) {
            return keywords[i].type;
        }
    }
    return TOKEN_ID;
}

Token getNextToken(Lexer *lexer) {
    FILE *f = lexer->file;
    int c;

    // Ignorar espaços, quebras de linha e comentários 
    for (;;) {
        c = fgetc(f);

        while (c != EOF && isspace(c)) {
            if (c == '\n') lexer->currentLine++;
            c = fgetc(f);
        }

        if (c != '/') break;

        int next = fgetc(f);
        if (next != '/') {          /* era só uma divisão */
            ungetc(next, f);
            break;
        }

        // comentário: consome até o fim da linha
        while (c != EOF && c != '\n') c = fgetc(f);
        if (c == '\n') lexer->currentLine++;
    }

    int line = lexer->currentLine;

    if (c == EOF) {
        return makeToken(TOKEN_EOF, "EOF", line);
    }

    // 2) ID e palavras reservadas: L(L|D)*
    if (isalpha(c) || c == '_') {
        char buf[MAX_LEXEME + 1];
        int i = 0;
        while (isalnum(c) || c == '_') {
            if (i < MAX_LEXEME) buf[i++] = (char)c;
            c = fgetc(f);
        }
        buf[i] = '\0';
        ungetc(c, f);

        TokenType type = lookupKeyword(buf);
        if (type == TOKEN_ID) {
            insertSymbol(buf, TOKEN_ID);
        }
        return makeToken(type, buf, line);
    }

    // NUM_INT: D+   e   NUM_DEC: D+\.D+ 
    if (isdigit(c)) {
        char buf[MAX_LEXEME + 1];
        int i = 0;
        while (isdigit(c)) {
            if (i < MAX_LEXEME) buf[i++] = (char)c;
            c = fgetc(f);
        }

        if (c == '.') {
            int next = fgetc(f);
            if (isdigit(next)) {
                if (i < MAX_LEXEME) buf[i++] = '.';
                c = next;
                while (isdigit(c)) {
                    if (i < MAX_LEXEME) buf[i++] = (char)c;
                    c = fgetc(f);
                }
                buf[i] = '\0';
                ungetc(c, f);
                return makeToken(TOKEN_NUM_DEC, buf, line);
            }
            /* "3." sem dígitos depois: número malformado */
            ungetc(next, f);
            if (i < MAX_LEXEME) buf[i++] = '.';
            buf[i] = '\0';
            return makeToken(TOKEN_ERROR, buf, line);
        }

        buf[i] = '\0';
        ungetc(c, f);
        return makeToken(TOKEN_NUM_INT, buf, line);
    }

    // TEXT: "..." (não pode atravessar linha)
    if (c == '"') {
        char buf[MAX_LEXEME + 1];
        int i = 0;
        buf[i++] = '"';
        c = fgetc(f);
        while (c != '"' && c != '\n' && c != EOF) {
            if (i < MAX_LEXEME - 1) buf[i++] = (char)c;
            c = fgetc(f);
        }
        if (c == '"') {
            buf[i++] = '"';
            buf[i] = '\0';
            return makeToken(TOKEN_TEXT, buf, line);
        }
        /* chegou no fim da linha/arquivo sem fechar as aspas */
        buf[i] = '\0';
        ungetc(c, f);
        return makeToken(TOKEN_ERROR, buf, line);
    }

    // Operadores e símbolos
    switch (c) {
        case '=': {
            int next = fgetc(f);
            if (next == '=') return makeToken(TOKEN_COMP, "==", line);
            ungetc(next, f);
            return makeToken(TOKEN_ATRIB, "=", line);
        }
        case '!': {
            int next = fgetc(f);
            if (next == '=') return makeToken(TOKEN_COMP, "!=", line);
            ungetc(next, f);
            return makeToken(TOKEN_NOT, "!", line);
        }
        case '<': {
            int next = fgetc(f);
            if (next == '=') return makeToken(TOKEN_COMP, "<=", line);
            ungetc(next, f);
            return makeToken(TOKEN_COMP, "<", line);
        }
        case '>': {
            int next = fgetc(f);
            if (next == '=') return makeToken(TOKEN_COMP, ">=", line);
            ungetc(next, f);
            return makeToken(TOKEN_COMP, ">", line);
        }
        case '&': {
            int next = fgetc(f);
            if (next == '&') return makeToken(TOKEN_AND, "&&", line);
            ungetc(next, f);
            return makeToken(TOKEN_ERROR, "&", line);   /* & sozinho não existe */
        }
        case '|': {
            int next = fgetc(f);
            if (next == '|') return makeToken(TOKEN_OR, "||", line);
            ungetc(next, f);
            return makeToken(TOKEN_ERROR, "|", line);   /* | sozinho não existe */
        }
        case '+': return makeToken(TOKEN_SUM,           "+", line);
        case '-': return makeToken(TOKEN_SUB,           "-", line);
        case '*': return makeToken(TOKEN_MULT,          "*", line);
        case '/': return makeToken(TOKEN_DIV,           "/", line);
        case '%': return makeToken(TOKEN_MOD,           "%", line);
        case '(': return makeToken(TOKEN_OPEN_PAREN,    "(", line);
        case ')': return makeToken(TOKEN_CLOSE_PAREN,   ")", line);
        case '[': return makeToken(TOKEN_OPEN_BRACKET,  "[", line);
        case ']': return makeToken(TOKEN_CLOSE_BRACKET, "]", line);
        case '{': return makeToken(TOKEN_OPEN_BRACE,    "{", line);
        case '}': return makeToken(TOKEN_CLOSE_BRACE,   "}", line);
        case ',': return makeToken(TOKEN_COMMA,         ",", line);
        case ';': return makeToken(TOKEN_SEMICOLON,     ";", line);
    }

    // Qualquer outro caractere: erro léxico
    char bad[2] = { (char)c, '\0' };
    return makeToken(TOKEN_ERROR, bad, line);
}