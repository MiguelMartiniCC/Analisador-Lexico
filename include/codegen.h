#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>

/*
 * Gerador de código intermediário (código de três endereços).
 *
 * Segue o esquema de tradução (SDT) com backpatching visto em aula:
 * o parser chama estas funções no ponto exato de cada ação semântica.
 * As instruções ficam guardadas em memória (quádruplas) e só são
 * impressas quando a análise termina sem erros.
 */

#define MAX_QUADS 2048   // número máximo de instruções do programa
#define MAX_LIST 256     // número máximo de desvios numa truelist/falselist/nextlist
#define ADDR_SIZE 256    // tamanho de um endereço (lexema de até 255 caracteres + '\0')
#define OP_SIZE 16       // tamanho do campo op
#define MAX_FUNCTIONS 64 // número máximo de funções (rótulos)

// Destino ainda desconhecido de um desvio (será preenchido por backpatch).
#define PENDING_TARGET "_"

/*
 * Quádrupla: op, arg1, arg2, result (Aula 14, slide 19).
 * Nos desvios, result guarda o número da instrução de destino.
 * Campos não usados ficam com "" (string vazia).
 */
typedef struct
{
    char op[OP_SIZE];
    char arg1[ADDR_SIZE];
    char arg2[ADDR_SIZE];
    char result[ADDR_SIZE];
} Quad;

/*
 * Lista de números de instrução (truelist, falselist, nextlist).
 * Array fixo: count diz quantas posições de items estão em uso.
 */
typedef struct
{
    int items[MAX_LIST];
    int count;
} IntList;

// ---- Funções do SDT (mesmos nomes da aula) ----

// Número que a PRÓXIMA instrução emitida vai receber (começa em 1).
int nextinstr(void);

// Acrescenta uma quádrupla no fim do código. Argumentos NULL viram "".
void emit(const char *op, const char *arg1, const char *arg2, const char *result);

// Escreve em 'temp' o nome de um temporário novo: t1, t2, t3...
void newTemp(char temp[ADDR_SIZE]);

// Lista com um único elemento: a instrução 'instr'.
IntList makelist(int instr);

// Lista vazia (o "null" do SDT).
IntList emptylist(void);

// Concatena duas listas.
IntList merge(IntList first, IntList second);

// Faz todos os desvios de 'list' apontarem para a instrução 'target'.
void backpatch(IntList list, int target);

// ---- Funções do programa ----

// Começa o código de uma função: rótulo "nome:" antes da próxima instrução.
void codegenBeginFunction(const char *name);

// Termina a função: resolve a nextlist que sobrou e, se preciso, emite return.
void codegenEndFunction(IntList nextlist);

// ---- Saída ----

// Zera o código gerado (chamada antes do parser).
void initCodegen(void);

// Erro interno se algum desvio ficou sem destino ("_").
void checkNoPendingJumps(void);

// Imprime todas as instruções numeradas, com os rótulos das funções.
void printCode(FILE *output);

// Grava o código em <arquivo-fonte sem extensão>.tac e informa o caminho.
void writeCodeFile(const char *sourcePath);

#endif
