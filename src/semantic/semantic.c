#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "semantic.h"

#define MAX_DECLARATIONS 200
#define MAX_PARAMS 10
#define NAME_SIZE 256
#define GLOBAL_SCOPE -1
#define NOT_FOUND -1

typedef enum {
    DECL_VARIABLE,
    DECL_PARAMETER,
    DECL_FUNCTION
} DeclKind;

// Uma linha da tabela semântica.
typedef struct {
    char name[NAME_SIZE];
    DeclKind kind;
    SemType type;                // tipo da variável/parâmetro, ou tipo de retorno da função
    int owner;                   // índice da função dona; GLOBAL_SCOPE para as próprias funções
    int paramCount;              // usado só quando kind == DECL_FUNCTION
    SemType params[MAX_PARAMS];  // tipos dos parâmetros, na ordem
} Declaration;

static Declaration table[MAX_DECLARATIONS];
static int declarationCount = 0;

// Índice (na tabela) da função que está sendo analisada.
static int currentFunction = GLOBAL_SCOPE;

// ---------------------------------------------------------------------
// Tipos
// ---------------------------------------------------------------------

SemType makeType(BaseType base, int dimensions)
{
    SemType type;
    type.base = base;
    type.dimensions = dimensions;
    return type;
}

static const char *baseTypeName(BaseType base)
{
    switch (base)
    {
    case TYPE_INT:
        return "int";
    case TYPE_FLOAT:
        return "float";
    case TYPE_CHAR:
        return "char";
    case TYPE_BOOLEAN:
        return "boolean";
    case TYPE_VOID:
        return "void";
    default:
        return "texto";
    }
}

void typeToString(SemType type, char *buffer)
{
    strcpy(buffer, baseTypeName(type.base));
    for (int i = 0; i < type.dimensions; i++)
    {
        strcat(buffer, "[]");
    }
}

// int ou float, sem dimensões.
static int isNumeric(SemType type)
{
    return type.dimensions == 0 &&
           (type.base == TYPE_INT || type.base == TYPE_FLOAT);
}

// boolean ou int, sem dimensões (o que pode ser usado como condição).
static int isConditionType(SemType type)
{
    return type.dimensions == 0 &&
           (type.base == TYPE_BOOLEAN || type.base == TYPE_INT);
}

// Diz se um valor do tipo 'value' pode ser guardado em algo do tipo 'target'.
static int isAssignable(SemType target, SemType value)
{
    // Vetores: precisam ter exatamente o mesmo tipo (o tamanho não é comparado).
    if (target.dimensions > 0 || value.dimensions > 0)
    {
        return target.dimensions == value.dimensions &&
               target.base == value.base;
    }

    // Texto e void nunca são atribuíveis a variáveis simples.
    if (value.base == TYPE_TEXT || value.base == TYPE_VOID || target.base == TYPE_VOID)
    {
        return 0;
    }

    if (target.base == value.base)
    {
        return 1;
    }

    // int é aceito onde se espera float (conversão implícita)
    if (target.base == TYPE_FLOAT && value.base == TYPE_INT)
    {
        return 1;
    }

    // A linguagem não tem true/false, então 0 e 1 (int) servem para boolean
    if (target.base == TYPE_BOOLEAN && value.base == TYPE_INT)
    {
        return 1;
    }

    return 0;
}

// ---------------------------------------------------------------------
// Erros
// ---------------------------------------------------------------------

void semanticError(int line, const char *format, ...)
{
    va_list args;

    printf("Erro Semântico [Linha %d]: ", line);

    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
    exit(EXIT_FAILURE);
}

// ---------------------------------------------------------------------
// Tabela semântica
// ---------------------------------------------------------------------

void initSemantic(void)
{
    declarationCount = 0;
    currentFunction = GLOBAL_SCOPE;
}

// Procura uma função pelo nome. Devolve o índice ou NOT_FOUND.
static int findFunction(const char *name)
{
    for (int i = 0; i < declarationCount; i++)
    {
        if (table[i].kind == DECL_FUNCTION && strcmp(table[i].name, name) == 0)
        {
            return i;
        }
    }
    return NOT_FOUND;
}

// Procura uma variável ou parâmetro da função atual. Devolve o índice ou NOT_FOUND.
static int findLocal(const char *name)
{
    for (int i = 0; i < declarationCount; i++)
    {
        if (table[i].kind != DECL_FUNCTION &&
            table[i].owner == currentFunction &&
            strcmp(table[i].name, name) == 0)
        {
            return i;
        }
    }
    return NOT_FOUND;
}

// Acrescenta uma linha na tabela e devolve o índice dela.
static int addDeclaration(const char *name, DeclKind kind, SemType type, int owner, int line)
{
    if (declarationCount >= MAX_DECLARATIONS)
    {
        semanticError(line, "Tabela semântica cheia (máximo de %d declarações).", MAX_DECLARATIONS);
    }

    Declaration *decl = &table[declarationCount];
    snprintf(decl->name, NAME_SIZE, "%s", name);
    decl->kind = kind;
    decl->type = type;
    decl->owner = owner;
    decl->paramCount = 0;

    declarationCount++;
    return declarationCount - 1;
}

void beginFunction(const char *name, SemType returnType, int line)
{
    if (findFunction(name) != NOT_FOUND)
    {
        semanticError(line, "A função '%s' já foi declarada.", name);
    }

    // A função entra na tabela antes do corpo, então ela pode chamar a si mesma.
    currentFunction = addDeclaration(name, DECL_FUNCTION, returnType, GLOBAL_SCOPE, line);
}

void endFunction(void)
{
    currentFunction = GLOBAL_SCOPE;
}

void declareParameter(const char *name, SemType type, int line)
{
    Declaration *function = &table[currentFunction];

    if (findLocal(name) != NOT_FOUND)
    {
        semanticError(line, "O parâmetro '%s' já foi declarado na função '%s'.",
                      name, function->name);
    }

    if (function->paramCount >= MAX_PARAMS)
    {
        semanticError(line, "A função '%s' tem parâmetros demais (máximo de %d).",
                      function->name, MAX_PARAMS);
    }

    // Guarda o tipo na lista de parâmetros da função (usada para conferir as chamadas)
    function->params[function->paramCount] = type;
    function->paramCount++;

    addDeclaration(name, DECL_PARAMETER, type, currentFunction, line);
}

void declareVariable(const char *name, SemType type, int line)
{
    if (findLocal(name) != NOT_FOUND)
    {
        semanticError(line, "'%s' já foi declarado na função '%s'.",
                      name, table[currentFunction].name);
    }

    addDeclaration(name, DECL_VARIABLE, type, currentFunction, line);
}

SemType requireVariable(const char *name, int line)
{
    int index = findLocal(name);

    if (index == NOT_FOUND)
    {
        semanticError(line, "A variável '%s' não foi declarada.", name);
    }

    return table[index].type;
}

SemType requireFunction(const char *name, int line)
{
    int index = findFunction(name);

    if (index == NOT_FOUND)
    {
        semanticError(line, "A função '%s' não foi declarada (ela precisa ser declarada antes da chamada).", name);
    }

    return table[index].type;
}

// ---------------------------------------------------------------------
// Expressões
// ---------------------------------------------------------------------

SemType indexedType(SemType type, int indexCount, const char *name, int line)
{
    if (indexCount == 0)
    {
        return type;
    }

    if (type.dimensions == 0)
    {
        semanticError(line, "'%s' não é um vetor e não pode receber índice.", name);
    }

    if (indexCount > type.dimensions)
    {
        semanticError(line, "'%s' tem %d dimensão(ões), mas foi indexado com %d índice(s).",
                      name, type.dimensions, indexCount);
    }

    // Com menos índices que dimensões, o resultado ainda é um vetor.
    return makeType(type.base, type.dimensions - indexCount);
}

void requireIntIndex(SemType index, int line)
{
    if (index.dimensions != 0 || index.base != TYPE_INT)
    {
        char name[32];
        typeToString(index, name);
        semanticError(line, "O índice de um vetor deve ser int, mas foi encontrado '%s'.", name);
    }
}

SemType arithmeticType(SemType left, SemType right, const char *op, int line)
{
    char leftName[32];
    char rightName[32];
    typeToString(left, leftName);
    typeToString(right, rightName);

    if (!isNumeric(left) || !isNumeric(right))
    {
        semanticError(line, "O operador '%s' exige operandos int ou float, mas foi encontrado '%s' e '%s'.",
                      op, leftName, rightName);
    }

    // O resto da divisão só existe para inteiros
    if (strcmp(op, "%") == 0 &&
        (left.base != TYPE_INT || right.base != TYPE_INT))
    {
        semanticError(line, "O operador '%%' exige operandos int, mas foi encontrado '%s' e '%s'.",
                      leftName, rightName);
    }

    if (left.base == TYPE_FLOAT || right.base == TYPE_FLOAT)
    {
        return makeType(TYPE_FLOAT, 0);
    }
    return makeType(TYPE_INT, 0);
}

SemType comparisonType(SemType left, SemType right, const char *op, int line)
{
    if (!isNumeric(left) || !isNumeric(right))
    {
        char leftName[32];
        char rightName[32];
        typeToString(left, leftName);
        typeToString(right, rightName);
        semanticError(line, "O operador '%s' exige operandos int ou float, mas foi encontrado '%s' e '%s'.",
                      op, leftName, rightName);
    }

    return makeType(TYPE_BOOLEAN, 0);
}

SemType logicalType(SemType left, SemType right, const char *op, int line)
{
    if (!isConditionType(left) || !isConditionType(right))
    {
        char leftName[32];
        char rightName[32];
        typeToString(left, leftName);
        typeToString(right, rightName);
        semanticError(line, "O operador '%s' exige operandos boolean ou int, mas foi encontrado '%s' e '%s'.",
                      op, leftName, rightName);
    }

    return makeType(TYPE_BOOLEAN, 0);
}

SemType notType(SemType operand, int line)
{
    if (!isConditionType(operand))
    {
        char name[32];
        typeToString(operand, name);
        semanticError(line, "O operador '!' exige um operando boolean ou int, mas foi encontrado '%s'.", name);
    }

    return makeType(TYPE_BOOLEAN, 0);
}

SemType signType(SemType operand, const char *op, int line)
{
    if (!isNumeric(operand))
    {
        char name[32];
        typeToString(operand, name);
        semanticError(line, "O sinal '%s' exige um operando int ou float, mas foi encontrado '%s'.", op, name);
    }

    return operand;
}

// ---------------------------------------------------------------------
// Comandos
// ---------------------------------------------------------------------

void requireCondition(SemType condition, const char *statementName, int line)
{
    if (!isConditionType(condition))
    {
        char name[32];
        typeToString(condition, name);
        semanticError(line, "A condição do '%s' deve ser boolean ou int, mas foi encontrado '%s'.",
                      statementName, name);
    }
}

void checkAssignment(const char *name, SemType target, SemType value, int line)
{
    if (value.base == TYPE_VOID)
    {
        semanticError(line, "Uma função void não retorna valor, então não pode ser atribuída a '%s'.", name);
    }

    if (!isAssignable(target, value))
    {
        char targetName[32];
        char valueName[32];
        typeToString(target, targetName);
        typeToString(value, valueName);
        semanticError(line, "Atribuição incompatível: '%s' é '%s', mas o valor é '%s'.",
                      name, targetName, valueName);
    }
}

void checkReturn(SemType value, int line)
{
    Declaration *function = &table[currentFunction];

    if (function->type.base == TYPE_VOID)
    {
        semanticError(line, "A função '%s' é void e não pode retornar valor.", function->name);
    }

    if (!isAssignable(function->type, value))
    {
        char expectedName[32];
        char valueName[32];
        typeToString(function->type, expectedName);
        typeToString(value, valueName);
        semanticError(line, "Retorno incompatível: a função '%s' retorna '%s', mas o valor é '%s'.",
                      function->name, expectedName, valueName);
    }
}

// ---------------------------------------------------------------------
// Chamada de função
// ---------------------------------------------------------------------

void checkArgument(const char *functionName, int position, SemType argument, int line)
{
    Declaration *function = &table[findFunction(functionName)];

    // Argumentos além do esperado são reportados por checkArgumentCount.
    if (position >= function->paramCount)
    {
        return;
    }

    SemType expected = function->params[position];

    // Uma constante "texto" só pode ser passada para parâmetro char.
    int textForChar = argument.base == TYPE_TEXT && expected.base == TYPE_CHAR;

    if (!textForChar && !isAssignable(expected, argument))
    {
        char expectedName[32];
        char argumentName[32];
        typeToString(expected, expectedName);
        typeToString(argument, argumentName);
        semanticError(line, "Argumento %d da chamada de '%s': esperado '%s', mas foi encontrado '%s'.",
                      position + 1, functionName, expectedName, argumentName);
    }
}

void checkArgumentCount(const char *functionName, int count, int line)
{
    Declaration *function = &table[findFunction(functionName)];

    if (count != function->paramCount)
    {
        semanticError(line, "A função '%s' espera %d argumento(s), mas a chamada tem %d.",
                      functionName, function->paramCount, count);
    }
}

// ---------------------------------------------------------------------
// Resultado
// ---------------------------------------------------------------------

static const char *kindName(DeclKind kind)
{
    switch (kind)
    {
    case DECL_VARIABLE:
        return "variavel";
    case DECL_PARAMETER:
        return "parametro";
    default:
        return "funcao";
    }
}

void finishSemanticAnalysis(void)
{
    printf("\n=== TABELA SEMÂNTICA ===\n");

    for (int i = 0; i < declarationCount; i++)
    {
        char typeName[32];
        typeToString(table[i].type, typeName);

        const char *scope = "global";
        if (table[i].owner != GLOBAL_SCOPE)
        {
            scope = table[table[i].owner].name;
        }

        printf("[%d] Nome: %-15s | Categoria: %-9s | Tipo: %-10s | Escopo: %s\n",
               i, table[i].name, kindName(table[i].kind), typeName, scope);
    }

    printf("\n=========================================\n");
    printf(" SUCESSO: Análise Semântica Completa! \n");
    printf("=========================================\n");
}
