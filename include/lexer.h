#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "token.h"

typedef struct {
    FILE *file;
    int currentLine;
} Lexer;

void initLexer(Lexer *lexer, FILE *file);
Token getNextToken(Lexer *lexer);

#endif