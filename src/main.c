#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "symbol_table.h"

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Uso: %s <ficheiro_fonte>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");

    if (!file) {
        perror("Erro ao abrir o ficheiro");
        return 1;
    }

    /*
     * Inicializa a Tabela de Símbolos antes
     * de começar a análise léxica.
     */
    initSymbolTable();

    /*
     * Inicializa o Lexer com o arquivo fonte.
     */
    Lexer lexer;
    initLexer(&lexer, file);

    printf("=== LISTA DE TOKENS ===\n");

    Token t;

    /*
     * Executa a análise léxica.
     * O lexer lê o arquivo e produz os tokens.
     */
    do {
        t = getNextToken(&lexer);

        printf(
            "[Linha %02d] %-18s : %s\n",
            t.line,
            tokenTypeToString(t.type),
            t.lexeme
        );

    } while (t.type != TOKEN_EOF);

    /*
     * Ao final da análise léxica,
     * exibe a Tabela de Símbolos.
     */
    printf("\n=== TABELA DE SÍMBOLOS ===\n");
    printSymbolTable();

    fclose(file);

    return 0;
}