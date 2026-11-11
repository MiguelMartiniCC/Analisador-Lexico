#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

#define LINE_SIZE (4 * ADDR_SIZE + 32) // uma instrução formatada sempre cabe aqui
#define PATH_SIZE 1024

// Rótulo de função: nome e número da primeira instrução dela.
typedef struct
{
    char name[ADDR_SIZE];
    int firstInstr;
} FunctionLabel;

// quads[0] é a instrução 1, quads[1] é a instrução 2, e assim por diante.
static Quad quads[MAX_QUADS];
static int quadCount = 0;

static int tempCount = 0;

static FunctionLabel labels[MAX_FUNCTIONS];
static int labelCount = 0;

// ---------------------------------------------------------------------
// Erros internos
// ---------------------------------------------------------------------

// Um erro aqui não é culpa do programa-fonte: é um limite do gerador.
// Preferimos parar a gerar código errado ou truncado.
static void codegenFatal(const char *message)
{
    printf("Erro interno do gerador: %s\n", message);
    exit(EXIT_FAILURE);
}

// Copia 'source' para 'dest' (de tamanho 'size'), parando se não couber.
static void copyField(char *dest, size_t size, const char *source)
{
    if (source == NULL)
    {
        source = "";
    }
    if (strlen(source) >= size)
    {
        codegenFatal("campo de instrução maior que o limite (ADDR_SIZE/OP_SIZE).");
    }
    snprintf(dest, size, "%s", source);
}

// ---------------------------------------------------------------------
// Funções do SDT
// ---------------------------------------------------------------------

int nextinstr(void)
{
    // As instruções são numeradas a partir de 1, como nos slides.
    return quadCount + 1;
}

void emit(const char *op, const char *arg1, const char *arg2, const char *result)
{
    if (quadCount >= MAX_QUADS)
    {
        codegenFatal("programa grande demais (limite MAX_QUADS).");
    }

    Quad *quad = &quads[quadCount];
    copyField(quad->op, OP_SIZE, op);
    copyField(quad->arg1, ADDR_SIZE, arg1);
    copyField(quad->arg2, ADDR_SIZE, arg2);
    copyField(quad->result, ADDR_SIZE, result);
    quadCount++;
}

void newTemp(char temp[ADDR_SIZE])
{
    tempCount++;
    snprintf(temp, ADDR_SIZE, "t%d", tempCount);
}

IntList makelist(int instr)
{
    IntList list = emptylist();
    list.items[0] = instr;
    list.count = 1;
    return list;
}

IntList emptylist(void)
{
    IntList list;
    list.count = 0;
    return list;
}

IntList merge(IntList first, IntList second)
{
    if (first.count + second.count > MAX_LIST)
    {
        codegenFatal("lista de desvios grande demais (limite MAX_LIST).");
    }

    IntList result = first;
    for (int i = 0; i < second.count; i++)
    {
        result.items[result.count] = second.items[i];
        result.count++;
    }
    return result;
}

void backpatch(IntList list, int target)
{
    // Cada item é o número de um desvio emitido com destino "_".
    // O destino fica no campo result da quádrupla (Aula 14, slide 19).
    for (int i = 0; i < list.count; i++)
    {
        Quad *jump = &quads[list.items[i] - 1];
        snprintf(jump->result, ADDR_SIZE, "%d", target);
    }
}

// ---------------------------------------------------------------------
// Expressões
// ---------------------------------------------------------------------

Expr makeValue(const char *addr)
{
    Expr expr;
    expr.is_bool = 0;
    copyField(expr.addr, ADDR_SIZE, addr);
    expr.truelist = emptylist();
    expr.falselist = emptylist();
    return expr;
}

Expr emitBinaryOp(const char *op, Expr left, Expr right)
{
    char temp[ADDR_SIZE];
    newTemp(temp);
    emit(op, left.addr, right.addr, temp);
    return makeValue(temp);
}

Expr emitUnaryMinus(Expr operand)
{
    char temp[ADDR_SIZE];
    newTemp(temp);
    emit("minus", operand.addr, NULL, temp);
    return makeValue(temp);
}

// ---------------------------------------------------------------------
// Funções do programa
// ---------------------------------------------------------------------

void codegenBeginFunction(const char *name)
{
    if (labelCount >= MAX_FUNCTIONS)
    {
        codegenFatal("funções demais (limite MAX_FUNCTIONS).");
    }

    // O rótulo não consome número de instrução: só marca onde a função começa.
    copyField(labels[labelCount].name, ADDR_SIZE, name);
    labels[labelCount].firstInstr = nextinstr();
    labelCount++;
}

// Diz se a última instrução emitida na função atual é um return.
static int functionEndsWithReturn(void)
{
    int firstInstr = labels[labelCount - 1].firstInstr;

    if (quadCount < firstInstr)
    {
        return 0; // função sem nenhuma instrução
    }
    return strcmp(quads[quadCount - 1].op, "return") == 0;
}

void codegenEndFunction(IntList nextlist)
{
    // Desvios que ainda apontam para "depois do último comando" precisam
    // de uma instrução de destino; e uma função que não termina em return
    // não pode "cair" no código da próxima função. Nos dois casos,
    // emitimos um return implícito.
    if (nextlist.count > 0 || !functionEndsWithReturn())
    {
        backpatch(nextlist, nextinstr());
        emit("return", NULL, NULL, NULL);
    }
}

// ---------------------------------------------------------------------
// Saída
// ---------------------------------------------------------------------

void initCodegen(void)
{
    quadCount = 0;
    tempCount = 0;
    labelCount = 0;
}

// Desvio é qualquer instrução cujo destino fica em result: goto e if...
static int isJump(const Quad *quad)
{
    return strcmp(quad->op, "goto") == 0 || strncmp(quad->op, "if", 2) == 0;
}

void checkNoPendingJumps(void)
{
    for (int i = 0; i < quadCount; i++)
    {
        if (isJump(&quads[i]) && strcmp(quads[i].result, PENDING_TARGET) == 0)
        {
            printf("Erro interno do gerador: o desvio da instrução %d ficou sem destino.\n", i + 1);
            exit(EXIT_FAILURE);
        }
    }
}

// Desvios: "goto L", "if x goto L" e "if x relop y goto L".
// No relacional, op é "if" seguido do operador (ex.: "if<=").
static void formatJump(const Quad *q, char *line)
{
    if (strcmp(q->op, "goto") == 0)
    {
        snprintf(line, LINE_SIZE, "goto %s", q->result);
    }
    else if (strcmp(q->op, "if") == 0)
    {
        snprintf(line, LINE_SIZE, "if %s goto %s", q->arg1, q->result);
    }
    else
    {
        snprintf(line, LINE_SIZE, "if %s %s %s goto %s", q->arg1, q->op + 2, q->arg2, q->result);
    }
}

// Chamadas: "param x", "call p, n", "y = call p, n", "return" e "return y".
static void formatCall(const Quad *q, char *line)
{
    if (strcmp(q->op, "param") == 0)
    {
        snprintf(line, LINE_SIZE, "param %s", q->arg1);
    }
    else if (strcmp(q->op, "call") == 0 && q->result[0] == '\0')
    {
        snprintf(line, LINE_SIZE, "call %s, %s", q->arg1, q->arg2);
    }
    else if (strcmp(q->op, "call") == 0)
    {
        snprintf(line, LINE_SIZE, "%s = call %s, %s", q->result, q->arg1, q->arg2);
    }
    else if (q->arg1[0] == '\0')
    {
        snprintf(line, LINE_SIZE, "return");
    }
    else
    {
        snprintf(line, LINE_SIZE, "return %s", q->arg1);
    }
}

// Atribuições: cópia, menos unário, vetores e operação binária.
static void formatAssignment(const Quad *q, char *line)
{
    if (strcmp(q->op, "=") == 0)
    {
        snprintf(line, LINE_SIZE, "%s = %s", q->result, q->arg1);
    }
    else if (strcmp(q->op, "minus") == 0)
    {
        snprintf(line, LINE_SIZE, "%s = minus %s", q->result, q->arg1);
    }
    else if (strcmp(q->op, "=[]") == 0)
    {
        snprintf(line, LINE_SIZE, "%s = %s[%s]", q->result, q->arg1, q->arg2);
    }
    else if (strcmp(q->op, "[]=") == 0)
    {
        snprintf(line, LINE_SIZE, "%s[%s] = %s", q->result, q->arg2, q->arg1);
    }
    else
    {
        snprintf(line, LINE_SIZE, "%s = %s %s %s", q->result, q->arg1, q->op, q->arg2);
    }
}

// Escreve em 'line' a instrução no formato dos slides.
static void formatQuad(const Quad *q, char *line)
{
    if (isJump(q))
    {
        formatJump(q, line);
    }
    else if (strcmp(q->op, "param") == 0 || strcmp(q->op, "call") == 0 ||
             strcmp(q->op, "return") == 0)
    {
        formatCall(q, line);
    }
    else
    {
        formatAssignment(q, line);
    }
}

// Imprime os rótulos das funções que começam na instrução 'instr'.
static void printLabels(FILE *output, int instr)
{
    for (int i = 0; i < labelCount; i++)
    {
        if (labels[i].firstInstr == instr)
        {
            fprintf(output, "%s:\n", labels[i].name);
        }
    }
}

void printCode(FILE *output)
{
    char line[LINE_SIZE];

    for (int i = 0; i < quadCount; i++)
    {
        printLabels(output, i + 1);
        formatQuad(&quads[i], line);
        fprintf(output, "%d. %s\n", i + 1, line);
    }
}

// Troca a extensão de 'sourcePath' por ".tac" (ex.: teste.txt -> teste.tac).
static void buildOutputPath(const char *sourcePath, char *outputPath)
{
    size_t length = strlen(sourcePath);
    const char *dot = strrchr(sourcePath, '.');
    const char *slash = strrchr(sourcePath, '/');
    const char *backslash = strrchr(sourcePath, '\\');

    // Só é extensão se o ponto vier depois da última barra.
    if (dot != NULL && (slash == NULL || dot > slash) && (backslash == NULL || dot > backslash))
    {
        length = (size_t)(dot - sourcePath);
    }
    if (length + strlen(".tac") >= PATH_SIZE)
    {
        codegenFatal("caminho do arquivo de saída longo demais.");
    }
    snprintf(outputPath, PATH_SIZE, "%.*s.tac", (int)length, sourcePath);
}

void writeCodeFile(const char *sourcePath)
{
    char outputPath[PATH_SIZE];
    buildOutputPath(sourcePath, outputPath);

    FILE *output = fopen(outputPath, "w");
    if (output == NULL)
    {
        perror("Erro ao gravar o código intermediário");
        exit(EXIT_FAILURE);
    }
    printCode(output);
    fclose(output);

    printf("\nCódigo intermediário gravado em: %s\n", outputPath);
}
