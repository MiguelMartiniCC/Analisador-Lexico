#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "semantic.h"
#include "codegen.h"

static void advance(Parser *parser)
{
    parser->currentToken = getNextToken(parser->lexer);
}

static void match(Parser *parser, TokenType expectedType)
{
    if (parser->currentToken.type == expectedType)
    {
        advance(parser);
    }
    else
    {
        printf("Erro Sintático [Linha %d]: Esperado '%s', mas foi encontrado '%s' ('%s')\n",
               parser->currentToken.line,
               tokenTypeToString(expectedType),
               parser->currentToken.lexeme,
               tokenTypeToString(parser->currentToken.type));
        exit(EXIT_FAILURE);
    }
}

/*
 * CODEGEN: atributos sintetizados de uma expressão.
 * As funções de expressão devolvem os dois resultados juntos:
 * 'type' para o analisador semântico (E.type) e 'code' para o
 * gerador de código (E.addr, B.truelist, B.falselist).
 */
typedef struct
{
    SemType type;
    Expr code;
} Attr;

static Attr makeAttr(SemType type, Expr code)
{
    Attr attr;
    attr.type = type;
    attr.code = code;
    return attr;
}

static void program(Parser *parser);
static void functionList(Parser *parser);
static void functionDecl(Parser *parser);
static SemType returnType(Parser *parser);
static SemType type(Parser *parser);
static BaseType baseType(Parser *parser);
static int dimension(Parser *parser);
static void parameters(Parser *parser);
static void parameterList(Parser *parser);
static void mainFunction(Parser *parser);
static void block(Parser *parser);
static void varList(Parser *parser);
static void idList(Parser *parser, SemType varType);
static void statementList(Parser *parser);
static void statement(Parser *parser);
static void readStatement(Parser *parser);
static void readTerm(Parser *parser);
static void newReadTerm(Parser *parser);
static int dimension2(Parser *parser);
static void writeStatement(Parser *parser);
static void writeTerm(Parser *parser);
static void newWriteTerm(Parser *parser);
static void ifStatement(Parser *parser);
static void elseClause(Parser *parser);
static void whileStatement(Parser *parser);
static void assignmentStatement(Parser *parser);
static SemType complement(Parser *parser, const char *target);
static SemType functionCall(Parser *parser);
static int arguments(Parser *parser, const char *functionName);
static int newArgument(Parser *parser, const char *functionName, int position);
static void returnStatement(Parser *parser);
static Attr expression(Parser *parser);
static Attr orExpr(Parser *parser);
static Attr orExpr2(Parser *parser, Attr left);
static Attr andExpr(Parser *parser);
static Attr andExpr2(Parser *parser, Attr left);
static Attr relationalExpr(Parser *parser);
static Attr relationalExpr2(Parser *parser, Attr left);
static Attr additiveExpr(Parser *parser);
static Attr additiveExpr2(Parser *parser, Attr left);
static void additiveOp(Parser *parser);
static Attr multiplicativeExpr(Parser *parser);
static Attr multiplicativeExpr2(Parser *parser, Attr left);
static void multiplicativeOp(Parser *parser);
static Attr factor(Parser *parser);
static Attr signedTerm(Parser *parser);
static Attr term(Parser *parser);
static int sign(Parser *parser);
static SemType constant(Parser *parser);

// PROGRAMA → LISTAFUNÇÕES PRINCIPAL
static void program(Parser *parser)
{
    functionList(parser);
    mainFunction(parser);
}

// LISTAFUNÇÕES → DECFUNÇÃO LISTAFUNÇÕES | ε
static void functionList(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_INT ||
        parser->currentToken.type == TOKEN_FLOAT ||
        parser->currentToken.type == TOKEN_CHAR ||
        parser->currentToken.type == TOKEN_BOOLEAN ||
        parser->currentToken.type == TOKEN_VOID)
    {
        if (parser->currentToken.type == TOKEN_MAIN)
        {
            return; // ε (epsilon)
        }

        functionDecl(parser);
        functionList(parser);
    }
}

// DECFUNÇÃO → TIPORETORNO id ( PARÂMETROS ) BLOCO
static void functionDecl(Parser *parser)
{
    SemType functionType = returnType(parser);

    Token id = parser->currentToken;
    match(parser, TOKEN_ID);
    beginFunction(id.lexeme, functionType, id.line);
    codegenBeginFunction(id.lexeme); // CODEGEN: rótulo "nome:" da função

    match(parser, TOKEN_OPEN_PAREN);
    parameters(parser);
    match(parser, TOKEN_CLOSE_PAREN);
    block(parser);

    codegenEndFunction(emptylist()); // CODEGEN: return implícito se preciso
    endFunction();
}

// TIPORETORNO → TIPO | void
static SemType returnType(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_VOID)
    {
        match(parser, TOKEN_VOID);
        return makeType(TYPE_VOID, 0);
    }

    return type(parser);
}

// TIPO → TIPOBASE DIMENSÃO
static SemType type(Parser *parser)
{
    BaseType base = baseType(parser);
    int dimensions = dimension(parser);
    return makeType(base, dimensions);
}

// TIPOBASE → char | float | int | boolean
static BaseType baseType(Parser *parser)
{
    switch (parser->currentToken.type)
    {
    case TOKEN_CHAR:
        match(parser, TOKEN_CHAR);
        return TYPE_CHAR;
    case TOKEN_FLOAT:
        match(parser, TOKEN_FLOAT);
        return TYPE_FLOAT;
    case TOKEN_INT:
        match(parser, TOKEN_INT);
        return TYPE_INT;
    case TOKEN_BOOLEAN:
        match(parser, TOKEN_BOOLEAN);
        return TYPE_BOOLEAN;
    default:
        printf("Erro Sintático [Linha %d]: Esperado tipo base, mas foi encontrado '%s' ('%s')\n",
               parser->currentToken.line,
               parser->currentToken.lexeme,
               tokenTypeToString(parser->currentToken.type));
        exit(EXIT_FAILURE);
    }
}

// DIMENSÃO → [ num_int ] DIMENSÃO | ε
// Devolve quantas dimensões foram declaradas.
static int dimension(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_OPEN_BRACKET)
    {
        match(parser, TOKEN_OPEN_BRACKET);
        match(parser, TOKEN_NUM_INT);
        match(parser, TOKEN_CLOSE_BRACKET);
        return 1 + dimension(parser);
    }
    return 0;
}

// PARÂMETROS → TIPO id LISTAPARAMETROS | ε
static void parameters(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_INT ||
        parser->currentToken.type == TOKEN_FLOAT ||
        parser->currentToken.type == TOKEN_CHAR ||
        parser->currentToken.type == TOKEN_BOOLEAN)
    {
        SemType paramType = type(parser);
        Token id = parser->currentToken;
        match(parser, TOKEN_ID);
        declareParameter(id.lexeme, paramType, id.line);
        parameterList(parser);
    }
}

// LISTAPARAMETROS → , TIPO id LISTAPARAMETROS | ε
static void parameterList(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_COMMA)
    {
        match(parser, TOKEN_COMMA);
        SemType paramType = type(parser);
        Token id = parser->currentToken;
        match(parser, TOKEN_ID);
        declareParameter(id.lexeme, paramType, id.line);
        parameterList(parser);
    }
}

// PRINCIPAL → main ( ) BLOCO
static void mainFunction(Parser *parser)
{
    Token mainToken = parser->currentToken;
    match(parser, TOKEN_MAIN);

    // main devolve int (por isso "return 0;" é válido)
    beginFunction(mainToken.lexeme, makeType(TYPE_INT, 0), mainToken.line);
    codegenBeginFunction(mainToken.lexeme); // CODEGEN: rótulo "main:"

    match(parser, TOKEN_OPEN_PAREN);
    match(parser, TOKEN_CLOSE_PAREN);
    block(parser);

    codegenEndFunction(emptylist()); // CODEGEN: return implícito se preciso
    endFunction();
}

// BLOCO → { LISTAVARIÁVEIS COMANDOS }
static void block(Parser *parser)
{
    match(parser, TOKEN_OPEN_BRACE);
    varList(parser);
    statementList(parser);
    match(parser, TOKEN_CLOSE_BRACE);
}

// LISTAVARIÁVEIS → TIPO id LISTAID ; LISTAVARIÁVEIS | ε
static void varList(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_INT ||
        parser->currentToken.type == TOKEN_FLOAT ||
        parser->currentToken.type == TOKEN_CHAR ||
        parser->currentToken.type == TOKEN_BOOLEAN)
    {
        SemType varType = type(parser);
        Token id = parser->currentToken;
        match(parser, TOKEN_ID);
        declareVariable(id.lexeme, varType, id.line);
        idList(parser, varType);
        match(parser, TOKEN_SEMICOLON);
        varList(parser);
    }
}

// LISTAID → , id LISTAID | ε
static void idList(Parser *parser, SemType varType)
{
    if (parser->currentToken.type == TOKEN_COMMA)
    {
        match(parser, TOKEN_COMMA);
        Token id = parser->currentToken;
        match(parser, TOKEN_ID);
        declareVariable(id.lexeme, varType, id.line);
        idList(parser, varType);
    }
}

// COMANDOS → COMANDO COMANDOS | ε
static void statementList(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_SCANF ||
        parser->currentToken.type == TOKEN_PRINTLN ||
        parser->currentToken.type == TOKEN_IF ||
        parser->currentToken.type == TOKEN_WHILE ||
        parser->currentToken.type == TOKEN_RETURN ||
        parser->currentToken.type == TOKEN_FUNC ||
        parser->currentToken.type == TOKEN_ID)
    {
        statement(parser);
        statementList(parser);
    }
}

// COMANDO → LEITURA | ESCRITA | ATRIBUIÇÃO | FUNÇÃO | SELEÇÃO | ENQUANTO | RETORNO
static void statement(Parser *parser)
{
    switch (parser->currentToken.type)
    {
    case TOKEN_SCANF:
        readStatement(parser);
        break;
    case TOKEN_PRINTLN:
        writeStatement(parser);
        break;
    case TOKEN_IF:
        ifStatement(parser);
        break;
    case TOKEN_FUNC:
        functionCall(parser);
        break;
    case TOKEN_WHILE:
        whileStatement(parser);
        break;
    case TOKEN_RETURN:
        returnStatement(parser);
        break;
    case TOKEN_ID:
        assignmentStatement(parser);
        break;
    default:
        printf("Erro Sintático [Linha %d]: Esperado comando, mas foi encontrado '%s' ('%s')\n",
               parser->currentToken.line,
               parser->currentToken.lexeme,
               tokenTypeToString(parser->currentToken.type));
        exit(EXIT_FAILURE);
    }
}

// LEITURA → scanf ( TERMOLEITURA NOVOTERMOLEITURA) ;
static void readStatement(Parser *parser)
{
    match(parser, TOKEN_SCANF);
    match(parser, TOKEN_OPEN_PAREN);
    readTerm(parser);
    newReadTerm(parser);
    match(parser, TOKEN_CLOSE_PAREN);
    match(parser, TOKEN_SEMICOLON);
}

// TERMOLEITURA → id DIMENSAO2
static void readTerm(Parser *parser)
{
    Token id = parser->currentToken;
    match(parser, TOKEN_ID);
    SemType varType = requireVariable(id.lexeme, id.line);
    int indexCount = dimension2(parser);
    indexedType(varType, indexCount, id.lexeme, id.line);
}

// NOVOTERMOLEITURA → , TERMOLEITURA NOVOTERMOLEITURA | ε
static void newReadTerm(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_COMMA)
    {
        match(parser, TOKEN_COMMA);
        readTerm(parser);
        newReadTerm(parser);
    }
}

// DIMENSAO2 → [ EXPR_ADITIVA ] DIMENSAO2 | ε
// Confere que cada índice é int e devolve quantos índices foram usados.
static int dimension2(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_OPEN_BRACKET)
    {
        match(parser, TOKEN_OPEN_BRACKET);
        int line = parser->currentToken.line;
        Attr index = additiveExpr(parser);
        requireIntIndex(index.type, line);
        match(parser, TOKEN_CLOSE_BRACKET);
        return 1 + dimension2(parser);
    }
    return 0;
}

// ESCRITA → println ( TERMOESCRITA NOVOTERMOESCRITA ) ;
static void writeStatement(Parser *parser)
{
    match(parser, TOKEN_PRINTLN);
    match(parser, TOKEN_OPEN_PAREN);
    writeTerm(parser);
    newWriteTerm(parser);
    match(parser, TOKEN_CLOSE_PAREN);
    match(parser, TOKEN_SEMICOLON);
}

// TERMOESCRITA → id DIMENSAO2 | CONSTANTE | texto
static void writeTerm(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_ID)
    {
        Token id = parser->currentToken;
        match(parser, TOKEN_ID);
        SemType varType = requireVariable(id.lexeme, id.line);
        int indexCount = dimension2(parser);
        indexedType(varType, indexCount, id.lexeme, id.line);
    }
    else if (parser->currentToken.type == TOKEN_TEXT)
    {
        match(parser, TOKEN_TEXT);
    }
    else
    {
        constant(parser);
    }
}

// NOVOTERMOESCRITA → , TERMOESCRITA NOVOTERMOESCRITA | ε
static void newWriteTerm(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_COMMA)
    {
        match(parser, TOKEN_COMMA);
        writeTerm(parser);
        newWriteTerm(parser);
    }
}

// SELEÇÃO → if ( EXPRESSÃO ) BLOCO SENÃO
static void ifStatement(Parser *parser)
{
    match(parser, TOKEN_IF);
    match(parser, TOKEN_OPEN_PAREN);
    int line = parser->currentToken.line;
    Attr condition = expression(parser);
    requireCondition(condition.type, "if", line);
    match(parser, TOKEN_CLOSE_PAREN);
    block(parser);
    elseClause(parser);
}

// SENÃO → else BLOCO | ε
static void elseClause(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_ELSE)
    {
        match(parser, TOKEN_ELSE);
        block(parser);
    }
}

// ENQUANTO → while ( EXPRESSÃO ) BLOCO
static void whileStatement(Parser *parser)
{
    match(parser, TOKEN_WHILE);
    match(parser, TOKEN_OPEN_PAREN);
    int line = parser->currentToken.line;
    Attr condition = expression(parser);
    requireCondition(condition.type, "while", line);
    match(parser, TOKEN_CLOSE_PAREN);
    block(parser);
}

// ATRIBUIÇÃO → id = COMPLEMENTO ;
static void assignmentStatement(Parser *parser)
{
    Token id = parser->currentToken;
    match(parser, TOKEN_ID);
    SemType target = requireVariable(id.lexeme, id.line);
    match(parser, TOKEN_ATRIB);
    SemType value = complement(parser, id.lexeme);
    checkAssignment(id.lexeme, target, value, id.line);
    match(parser, TOKEN_SEMICOLON);
}

// COMPLEMENTO → EXPRESSÃO | FUNÇÃO]
// Devolve o tipo do valor: o retorno da função ou o tipo da expressão.
// 'target' é o id que recebe o valor (o lado esquerdo da atribuição).
// SDT: A → id = E   { emit(top.get(id.lexeme) '=' E.addr); }
static SemType complement(Parser *parser, const char *target)
{
    if (parser->currentToken.type == TOKEN_FUNC)
    {
        return functionCall(parser);
    }

    Attr value = expression(parser);
    emit("=", value.code.addr, NULL, target); // CODEGEN: id = E.addr
    return value.type;
}

// FUNÇÃO → func id ( ARGUMENTOS ) IMPORTANTE: ADICIONAR O FUNC NO TOKEN.H E NA LEITURA DO LEXER.
// Devolve o tipo de retorno da função chamada.
static SemType functionCall(Parser *parser)
{
    match(parser, TOKEN_FUNC);
    Token id = parser->currentToken;
    match(parser, TOKEN_ID);
    SemType result = requireFunction(id.lexeme, id.line);
    match(parser, TOKEN_OPEN_PAREN);
    int argumentCount = arguments(parser, id.lexeme);
    match(parser, TOKEN_CLOSE_PAREN);
    checkArgumentCount(id.lexeme, argumentCount, id.line);
    return result;
}

// ARGUMENTOS → EXPRESSÃO NOVO_ARGUMENTO | ε
// Confere o tipo de cada argumento e devolve quantos argumentos a chamada tem.
static int arguments(Parser *parser, const char *functionName)
{
    if (parser->currentToken.type == TOKEN_SUM ||
        parser->currentToken.type == TOKEN_SUB ||
        parser->currentToken.type == TOKEN_NOT ||
        parser->currentToken.type == TOKEN_OPEN_PAREN ||
        parser->currentToken.type == TOKEN_ID ||
        parser->currentToken.type == TOKEN_NUM_INT ||
        parser->currentToken.type == TOKEN_NUM_DEC ||
        parser->currentToken.type == TOKEN_TEXT)
    {
        int line = parser->currentToken.line;
        Attr argument = expression(parser);
        checkArgument(functionName, 0, argument.type, line);
        return 1 + newArgument(parser, functionName, 1);
    }
    return 0;
}

// NOVO_ARGUMENTO → , EXPRESSÃO NOVO_ARGUMENTO | ε
// 'position' é a posição (a partir de 0) do próximo argumento.
static int newArgument(Parser *parser, const char *functionName, int position)
{
    if (parser->currentToken.type == TOKEN_COMMA)
    {
        match(parser, TOKEN_COMMA);
        int line = parser->currentToken.line;
        Attr argument = expression(parser);
        checkArgument(functionName, position, argument.type, line);
        return 1 + newArgument(parser, functionName, position + 1);
    }
    return 0;
}

// RETORNO → return EXPRESSÃO ;
static void returnStatement(Parser *parser)
{
    Token returnToken = parser->currentToken;
    match(parser, TOKEN_RETURN);
    Attr value = expression(parser);
    checkReturn(value.type, returnToken.line);
    match(parser, TOKEN_SEMICOLON);
}

// EXPRESSÃO → EXPR_OU
// As funções de expressão devolvem o tipo (semântico) e o código (gerador).
static Attr expression(Parser *parser)
{
    return orExpr(parser);
}

// EXPR_OU → EXPR_E EXPR_OU2
static Attr orExpr(Parser *parser)
{
    Attr left = andExpr(parser);
    return orExpr2(parser, left);
}

// EXPR_OU2 → || EXPR_E EXPR_OU2 | ε
// 'left' é o que já foi lido à esquerda do operador.
static Attr orExpr2(Parser *parser, Attr left)
{
    if (parser->currentToken.type == TOKEN_OR)
    {
        Token op = parser->currentToken;
        match(parser, TOKEN_OR);
        Attr right = andExpr(parser);
        SemType result = logicalType(left.type, right.type, op.lexeme, op.line);
        return orExpr2(parser, makeAttr(result, left.code));
    }
    return left;
}

// EXPR_E → EXPR_RELACIONAL EXPR_E2
static Attr andExpr(Parser *parser)
{
    Attr left = relationalExpr(parser);
    return andExpr2(parser, left);
}

// EXPR_E2 → && EXPR_RELACIONAL EXPR_E2 | ε
static Attr andExpr2(Parser *parser, Attr left)
{
    if (parser->currentToken.type == TOKEN_AND)
    {
        Token op = parser->currentToken;
        match(parser, TOKEN_AND);
        Attr right = relationalExpr(parser);
        SemType result = logicalType(left.type, right.type, op.lexeme, op.line);
        return andExpr2(parser, makeAttr(result, left.code));
    }
    return left;
}

// EXPR_RELACIONAL → EXPR_ADITIVA EXPR_ RELACIONAL2
static Attr relationalExpr(Parser *parser)
{
    Attr left = additiveExpr(parser);
    return relationalExpr2(parser, left);
}

// EXPR_RELACIONAL2 → comp EXPR_ADITIVA | ε
static Attr relationalExpr2(Parser *parser, Attr left)
{
    if (parser->currentToken.type == TOKEN_COMP)
    {
        Token op = parser->currentToken;
        match(parser, TOKEN_COMP);
        Attr right = additiveExpr(parser);
        SemType result = comparisonType(left.type, right.type, op.lexeme, op.line);
        return makeAttr(result, left.code);
    }
    return left;
}

// EXPR_ADITIVA → EXPR_MULTIPLICATIVA EXPR_ADITIVA2
static Attr additiveExpr(Parser *parser)
{
    Attr left = multiplicativeExpr(parser);
    return additiveExpr2(parser, left);
}

// EXPR_ADITIVA2 → OP_ADITIVO EXPR_MULTIPLICATIVA EXPR_ADITIVA2 | ε
// SDT: E → E1 + E2  { E.addr = new Temp(); emit(E.addr '=' E1.addr '+' E2.addr); }  (idem para -)
// 'left' faz o papel de E1: assim a - b - c vira (a - b) - c (associatividade à esquerda).
static Attr additiveExpr2(Parser *parser, Attr left)
{
    if (parser->currentToken.type == TOKEN_SUM ||
        parser->currentToken.type == TOKEN_SUB)
    {
        Token op = parser->currentToken;
        additiveOp(parser);
        Attr right = multiplicativeExpr(parser);
        SemType result = arithmeticType(left.type, right.type, op.lexeme, op.line);
        Expr code = emitBinaryOp(op.lexeme, left.code, right.code); // CODEGEN: t = E1 op E2
        return additiveExpr2(parser, makeAttr(result, code));
    }
    return left;
}

// OP_ADITIVO → + | -
static void additiveOp(Parser *parser)
{
    switch (parser->currentToken.type)
    {
    case TOKEN_SUM:
        match(parser, TOKEN_SUM);
        break;
    case TOKEN_SUB:
        match(parser, TOKEN_SUB);
        break;
    default:
        printf("Erro Sintático [Linha %d]: Esperado operador aditivo, mas foi encontrado '%s' ('%s')\n",
               parser->currentToken.line,
               parser->currentToken.lexeme,
               tokenTypeToString(parser->currentToken.type));
        exit(EXIT_FAILURE);
    }
}

// EXPR_MULTIPLICATIVA → FATOR EXPR_MULTIPLICATIVA2
static Attr multiplicativeExpr(Parser *parser)
{
    Attr left = factor(parser);
    return multiplicativeExpr2(parser, left);
}

// EXPR_MULTIPLICATIVA2 → OP_MULTIPLICATIVO FATOR EXPR_MULTIPLICATIVA2 | ε
// SDT: mesmo padrão de E → E1 + E2, com * / %.
static Attr multiplicativeExpr2(Parser *parser, Attr left)
{
    if (parser->currentToken.type == TOKEN_MULT ||
        parser->currentToken.type == TOKEN_DIV ||
        parser->currentToken.type == TOKEN_MOD)
    {
        Token op = parser->currentToken;
        multiplicativeOp(parser);
        Attr right = factor(parser);
        SemType result = arithmeticType(left.type, right.type, op.lexeme, op.line);
        Expr code = emitBinaryOp(op.lexeme, left.code, right.code); // CODEGEN: t = E1 op E2
        return multiplicativeExpr2(parser, makeAttr(result, code));
    }
    return left;
}

// OP_MULTIPLICATIVO → * | / | %
static void multiplicativeOp(Parser *parser)
{
    switch (parser->currentToken.type)
    {
    case TOKEN_MULT:
        match(parser, TOKEN_MULT);
        break;
    case TOKEN_DIV:
        match(parser, TOKEN_DIV);
        break;
    case TOKEN_MOD:
        match(parser, TOKEN_MOD);
        break;
    default:
        printf("Erro Sintático [Linha %d]: Esperado operador multiplicativo, mas foi encontrado '%s' ('%s')\n",
               parser->currentToken.line,
               parser->currentToken.lexeme,
               tokenTypeToString(parser->currentToken.type));
        exit(EXIT_FAILURE);
    }
}

// FATOR → SINAL TERMO | texto | ! FATOR | ( EXPRESSÃO )
static Attr factor(Parser *parser)
{
    switch (parser->currentToken.type)
    {
    case TOKEN_TEXT:
    {
        Token text = parser->currentToken;
        match(parser, TOKEN_TEXT);
        // CODEGEN: a constante texto é o próprio endereço (com as aspas)
        return makeAttr(makeType(TYPE_TEXT, 0), makeValue(text.lexeme));
    }
    case TOKEN_NOT:
    {
        Token op = parser->currentToken;
        match(parser, TOKEN_NOT);
        Attr operand = factor(parser);
        return makeAttr(notType(operand.type, op.line), operand.code);
    }
    case TOKEN_OPEN_PAREN:
    {
        // SDT: E → ( E1 )  { E.addr = E1.addr; }  — não emite nada
        match(parser, TOKEN_OPEN_PAREN);
        Attr inner = expression(parser);
        match(parser, TOKEN_CLOSE_PAREN);
        return inner;
    }
    default:
        return signedTerm(parser);
    }
}

// SINAL TERMO (o caso padrão de FATOR).
// SDT: E → - E1  { E.addr = new Temp(); emit(E.addr '=' 'minus' E1.addr); }
// O + unário não muda o valor, então não gera instrução.
static Attr signedTerm(Parser *parser)
{
    Token signToken = parser->currentToken;
    int hasSign = sign(parser);
    Attr value = term(parser);

    if (!hasSign)
    {
        return value;
    }

    SemType result = signType(value.type, signToken.lexeme, signToken.line);
    if (signToken.type == TOKEN_SUB)
    {
        return makeAttr(result, emitUnaryMinus(value.code)); // CODEGEN: t = minus E1
    }
    return makeAttr(result, value.code);
}

// TERMO → id DIMENSAO2 | CONSTANTE
// SDT: E → id  { E.addr = top.get(id.lexeme); }   E → num  { E.addr = num.lexeme; }
static Attr term(Parser *parser)
{
    Token current = parser->currentToken;

    if (current.type == TOKEN_ID)
    {
        match(parser, TOKEN_ID);
        SemType varType = requireVariable(current.lexeme, current.line);
        int indexCount = dimension2(parser);
        SemType result = indexedType(varType, indexCount, current.lexeme, current.line);
        return makeAttr(result, makeValue(current.lexeme)); // CODEGEN: E.addr = id
    }

    SemType constType = constant(parser);
    return makeAttr(constType, makeValue(current.lexeme)); // CODEGEN: E.addr = num
}

// SINAL → + | - | ε
// Devolve 1 se havia um sinal (e ele foi consumido), 0 caso contrário.
static int sign(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_SUM)
    {
        match(parser, TOKEN_SUM);
        return 1;
    }
    if (parser->currentToken.type == TOKEN_SUB)
    {
        match(parser, TOKEN_SUB);
        return 1;
    }
    return 0;
}

// CONSTANTE → num_int | num_dec
static SemType constant(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_NUM_INT)
    {
        match(parser, TOKEN_NUM_INT);
        return makeType(TYPE_INT, 0);
    }
    if (parser->currentToken.type == TOKEN_NUM_DEC)
    {
        match(parser, TOKEN_NUM_DEC);
        return makeType(TYPE_FLOAT, 0);
    }

    printf("Erro Sintático [Linha %d]: Esperado constante, mas foi encontrado '%s' ('%s')\n",
           parser->currentToken.line,
           parser->currentToken.lexeme,
           tokenTypeToString(parser->currentToken.type));
    exit(EXIT_FAILURE);
}

void initParser(Parser *parser, Lexer *lexer)
{
    parser->lexer = lexer;
}

void parse(Parser *parser)
{
    advance(parser);
    program(parser);

    if (parser->currentToken.type == TOKEN_EOF)
    {
        printf("\n=========================================\n");
        printf(" SUCCESSO: Análise Sintática Completa! \n");
        printf("=========================================\n");
    }
    else
    {
        printf("Erro Sintático [Linha %d]: Token inesperado '%s' após o fim do programa ('%s')\n",
               parser->currentToken.line,
               parser->currentToken.lexeme,
               tokenTypeToString(parser->currentToken.type));
        exit(EXIT_FAILURE);
    }
}
