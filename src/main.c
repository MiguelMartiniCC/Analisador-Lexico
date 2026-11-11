#include <stdio.h>
#include <stdlib.h>

#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "symbol_table.h"

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        printf("Uso: %s <ficheiro_fonte>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");

    if (!file)
    {
        perror("Erro ao abrir o ficheiro");
        return 1;
    }

    /*
     * Inicializa a Tabela de Símbolos antes
     * de começar a análise léxica.
     */
    initSymbolTable();

    /*
     * Inicializa a tabela da análise semântica
     * (separada da Tabela de Símbolos do lexer).
     */
    initSemantic();

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
    do
    {
        t = getNextToken(&lexer);

        printf(
            "[Linha %02d] %-18s : %s\n",
            t.line,
            tokenTypeToString(t.type),
            t.lexeme);

    } while (t.type != TOKEN_EOF);

    /*
     * Ao final da análise léxica,
     * exibe a Tabela de Símbolos.
     */

    printSymbolTable();

    /*
     * Volta o ponteiro do ficheiro ao início para permitir
     * que o Parser releia os tokens do zero.
     */

    fseek(file, 0, SEEK_SET);
    initLexer(&lexer, file);

    printf("\n=== ANÁLISE SINTÁTICA E SEMÂNTICA ===\n");

    /*
     * Inicializa e executa o Parser.
     * As verificações semânticas acontecem durante a análise sintática:
     * o parser chama o analisador semântico a cada declaração, expressão
     * e comando. Se houver erro (sintático ou semântico), o programa
     * termina dentro de parse().
     */

    /*
     * O gerador de código intermediário também roda dentro de parse():
     * as instruções ficam em memória e só são mostradas no final.
     */
    initCodegen();

    Parser parser;
    initParser(&parser, &lexer);
    parse(&parser);

    /*
     * Se chegou aqui, o programa está correto nas duas análises.
     * Mostra a tabela semântica e a mensagem de sucesso.
     */
    finishSemanticAnalysis();

    /*
     * Só um programa sem erros chega aqui, então nunca imprimimos
     * código parcial. Antes, confere que todo desvio recebeu destino.
     */
    checkNoPendingJumps();

    printf("\n=== CÓDIGO INTERMEDIÁRIO ===\n");
    printCode(stdout);
    writeCodeFile(argv[1]);

    fclose(file);

    return 0;
}