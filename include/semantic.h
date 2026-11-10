#ifndef SEMANTIC_H
#define SEMANTIC_H

/*
 * Analisador semântico.
 *
 * Roda junto com o parser: cada função do parser chama as funções deste
 * módulo para declarar nomes, calcular o tipo das expressões e conferir as
 * regras da linguagem. No primeiro erro, a mensagem é impressa e o programa
 * termina (mesmo comportamento do parser).
 */

// Tipos base da linguagem. TYPE_TEXT é o tipo de uma constante "texto".
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_BOOLEAN,
    TYPE_VOID,
    TYPE_TEXT
} BaseType;

// Tipo completo: tipo base + número de dimensões (0 = valor simples).
// Exemplo: char[10] nome  ->  { TYPE_CHAR, 1 }
typedef struct {
    BaseType base;
    int dimensions;
} SemType;

SemType makeType(BaseType base, int dimensions);

// Escreve o tipo como texto em 'buffer' (ex.: "int", "char[]"). Use um buffer de 32 chars.
void typeToString(SemType type, char *buffer);

// Inicia (ou reinicia) a tabela semântica.
void initSemantic(void);

// Imprime "Erro Semântico [Linha N]: ..." e encerra o programa.
// Aceita o mesmo formato do printf.
void semanticError(int line, const char *format, ...);

// ---- Declarações e escopo ----

// Começa uma função: registra o nome e passa a usá-la como escopo atual.
void beginFunction(const char *name, SemType returnType, int line);
// Termina a função atual (volta ao escopo global).
void endFunction(void);
void declareParameter(const char *name, SemType type, int line);
void declareVariable(const char *name, SemType type, int line);

// Busca uma variável/parâmetro da função atual. Erro se não existir.
SemType requireVariable(const char *name, int line);
// Busca uma função declarada. Erro se não existir. Devolve o tipo de retorno.
SemType requireFunction(const char *name, int line);

// ---- Expressões ----

// Tipo de 'name[i][j]...' com 'indexCount' índices.
SemType indexedType(SemType type, int indexCount, const char *name, int line);
void requireIntIndex(SemType index, int line);

SemType arithmeticType(SemType left, SemType right, const char *op, int line);
SemType comparisonType(SemType left, SemType right, const char *op, int line);
SemType logicalType(SemType left, SemType right, const char *op, int line);
SemType notType(SemType operand, int line);
SemType signType(SemType operand, const char *op, int line);

// ---- Comandos ----

// Condição de if/while: precisa ser boolean ou int.
void requireCondition(SemType condition, const char *statementName, int line);
void checkAssignment(const char *name, SemType target, SemType value, int line);
void checkReturn(SemType value, int line);

// ---- Chamada de função ----

// 'position' começa em 0.
void checkArgument(const char *functionName, int position, SemType argument, int line);
void checkArgumentCount(const char *functionName, int count, int line);

// Imprime a tabela semântica e a mensagem de sucesso.
void finishSemanticAnalysis(void);

#endif
