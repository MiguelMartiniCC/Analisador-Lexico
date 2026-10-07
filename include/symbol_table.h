#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "token.h"

typedef struct {
    char name[256];
    TokenType type;
} Symbol;

void initSymbolTable(void);
int insertSymbol(const char *name, TokenType type);
void printSymbolTable(void);

#endif