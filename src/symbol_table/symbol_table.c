#include <stdio.h>
#include <string.h>
#include "symbol_table.h"

#define MAX_SYMBOLS 100

static Symbol symbolTable[MAX_SYMBOLS];
static int symbolCount = 0;

void initSymbolTable(void) {
    symbolCount = 0;
}

int insertSymbol(const char *name, TokenType type) {
    for (int i = 0; i < symbolCount; i++) {
        if (strcmp(symbolTable[i].name, name) == 0) {
            return i; // Já existe na tabela
        }
    }
    if (symbolCount < MAX_SYMBOLS) {
        strcpy(symbolTable[symbolCount].name, name);
        symbolTable[symbolCount].type = type;
        symbolCount++;
        return symbolCount - 1;
    }
    return -1; // Tabela cheia
}

void printSymbolTable(void) {
    printf("\n=== TABELA DE SÍMBOLOS ===\n");
    for (int i = 0; i < symbolCount; i++) {
        printf("[%d] Nome: %-15s | Tipo: %s\n", i, symbolTable[i].name, tokenTypeToString(symbolTable[i].type));
    }
}