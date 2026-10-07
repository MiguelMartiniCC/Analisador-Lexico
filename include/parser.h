#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "lexer.h"

typedef struct {
    Lexer *lexer;
    Token currentToken;
} Parser;

void initParser(Parser *parser, Lexer *lexer);
void parse(Parser *parser);

#endif