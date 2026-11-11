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

/*
 * Resultado da tradução de uma expressão (atributos do SDT).
 * - Valor (is_bool = 0): o resultado está no endereço addr (E.addr),
 *   que pode ser um nome, uma constante ou um temporário.
 * - Booleano em código de desvio (is_bool = 1): não há endereço; o
 *   resultado são os desvios em truelist e falselist (B.truelist, B.falselist).
 */
typedef struct
{
    int is_bool;
    char addr[ADDR_SIZE];
    IntList truelist;
    IntList falselist;
} Expr;

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

// ---- Expressões ----

// Expressão cujo valor já está em 'addr' (E → id, E → num): não emite nada.
Expr makeValue(const char *addr);

// E → E1 op E2: E.addr = new Temp(); emit(E.addr '=' E1.addr op E2.addr).
// Vale para + - * / %.
Expr emitBinaryOp(const char *op, Expr left, Expr right);

// E → - E1: E.addr = new Temp(); emit(E.addr '=' 'minus' E1.addr).
Expr emitUnaryMinus(Expr operand);

// ---- Condições (código de desvio, com curto-circuito) ----

// Expressão booleana dada pelas suas listas de desvio.
Expr makeBool(IntList truelist, IntList falselist);

// B → E1 rel E2: B.truelist = makelist(nextinstr); B.falselist = makelist(nextinstr + 1);
// emit('if' E1.addr rel E2.addr 'goto _'); emit('goto _').
Expr emitRelational(const char *relop, Expr left, Expr right);

// Garante código de desvio. Um valor x (ex.: "if (ok)") vira
// "if x goto _" e "goto _"; uma expressão já booleana volta igual.
Expr toCondition(Expr expr);

// Garante um endereço (booleano usado como valor, ex.: "ok = a < b").
// Uma expressão em código de desvio vira um temporário com 1 ou 0:
//   truelist  -> k:   t = 1
//                k+1: goto k+3
//   falselist -> k+2: t = 0
// Um valor volta igual.
Expr toValue(Expr expr);

// N → ε: N.nextlist = makelist(nextinstr); emit('goto _').
IntList emitGotoPlaceholder(void);

// emit('goto' target), com destino já conhecido (o "goto M1.instr" do while).
void emitGoto(int target);

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
