#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "semantic.h"

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
static SemType complement(Parser *parser);
static SemType functionCall(Parser *parser);
static int arguments(Parser *parser, const char *functionName);
static int newArgument(Parser *parser, const char *functionName, int position);
static void returnStatement(Parser *parser);
static SemType expression(Parser *parser);
static SemType orExpr(Parser *parser);
static SemType orExpr2(Parser *parser, SemType left);
static SemType andExpr(Parser *parser);
static SemType andExpr2(Parser *parser, SemType left);
static SemType relationalExpr(Parser *parser);
static SemType relationalExpr2(Parser *parser, SemType left);
static SemType additiveExpr(Parser *parser);
static SemType additiveExpr2(Parser *parser, SemType left);
static void additiveOp(Parser *parser);
static SemType multiplicativeExpr(Parser *parser);
static SemType multiplicativeExpr2(Parser *parser, SemType left);
static void multiplicativeOp(Parser *parser);
static SemType factor(Parser *parser);
static SemType term(Parser *parser);
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

    match(parser, TOKEN_OPEN_PAREN);
    parameters(parser);
    match(parser, TOKEN_CLOSE_PAREN);
    block(parser);

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

    match(parser, TOKEN_OPEN_PAREN);
    match(parser, TOKEN_CLOSE_PAREN);
    block(parser);

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
        SemType index = additiveExpr(parser);
        requireIntIndex(index, line);
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
    SemType condition = expression(parser);
    requireCondition(condition, "if", line);
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
    SemType condition = expression(parser);
    requireCondition(condition, "while", line);
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
    SemType value = complement(parser);
    checkAssignment(id.lexeme, target, value, id.line);
    match(parser, TOKEN_SEMICOLON);
}

// COMPLEMENTO → EXPRESSÃO | FUNÇÃO]
// Devolve o tipo do valor: o retorno da função ou o tipo da expressão.
static SemType complement(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_FUNC)
    {
        return functionCall(parser);
    }

    return expression(parser);
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
        SemType argument = expression(parser);
        checkArgument(functionName, 0, argument, line);
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
        SemType argument = expression(parser);
        checkArgument(functionName, position, argument, line);
        return 1 + newArgument(parser, functionName, position + 1);
    }
    return 0;
}

// RETORNO → return EXPRESSÃO ;
static void returnStatement(Parser *parser)
{
    Token returnToken = parser->currentToken;
    match(parser, TOKEN_RETURN);
    SemType value = expression(parser);
    checkReturn(value, returnToken.line);
    match(parser, TOKEN_SEMICOLON);
}

// EXPRESSÃO → EXPR_OU
// As funções de expressão devolvem o tipo do que reconheceram.
static SemType expression(Parser *parser)
{
    return orExpr(parser);
}

// EXPR_OU → EXPR_E EXPR_OU2
static SemType orExpr(Parser *parser)
{
    SemType left = andExpr(parser);
    return orExpr2(parser, left);
}

// EXPR_OU2 → || EXPR_E EXPR_OU2 | ε
// 'left' é o tipo do que já foi lido à esquerda do operador.
static SemType orExpr2(Parser *parser, SemType left)
{
    if (parser->currentToken.type == TOKEN_OR)
    {
        Token op = parser->currentToken;
        match(parser, TOKEN_OR);
        SemType right = andExpr(parser);
        SemType result = logicalType(left, right, op.lexeme, op.line);
        return orExpr2(parser, result);
    }
    return left;
}

// EXPR_E → EXPR_RELACIONAL EXPR_E2
static SemType andExpr(Parser *parser)
{
    SemType left = relationalExpr(parser);
    return andExpr2(parser, left);
}

// EXPR_E2 → && EXPR_RELACIONAL EXPR_E2 | ε
static SemType andExpr2(Parser *parser, SemType left)
{
    if (parser->currentToken.type == TOKEN_AND)
    {
        Token op = parser->currentToken;
        match(parser, TOKEN_AND);
        SemType right = relationalExpr(parser);
        SemType result = logicalType(left, right, op.lexeme, op.line);
        return andExpr2(parser, result);
    }
    return left;
}

// EXPR_RELACIONAL → EXPR_ADITIVA EXPR_ RELACIONAL2
static SemType relationalExpr(Parser *parser)
{
    SemType left = additiveExpr(parser);
    return relationalExpr2(parser, left);
}

// EXPR_RELACIONAL2 → comp EXPR_ADITIVA | ε
static SemType relationalExpr2(Parser *parser, SemType left)
{
    if (parser->currentToken.type == TOKEN_COMP)
    {
        Token op = parser->currentToken;
        match(parser, TOKEN_COMP);
        SemType right = additiveExpr(parser);
        return comparisonType(left, right, op.lexeme, op.line);
    }
    return left;
}

// EXPR_ADITIVA → EXPR_MULTIPLICATIVA EXPR_ADITIVA2
static SemType additiveExpr(Parser *parser)
{
    SemType left = multiplicativeExpr(parser);
    return additiveExpr2(parser, left);
}

// EXPR_ADITIVA2 → OP_ADITIVO EXPR_MULTIPLICATIVA EXPR_ADITIVA2 | ε
static SemType additiveExpr2(Parser *parser, SemType left)
{
    if (parser->currentToken.type == TOKEN_SUM ||
        parser->currentToken.type == TOKEN_SUB)
    {
        Token op = parser->currentToken;
        additiveOp(parser);
        SemType right = multiplicativeExpr(parser);
        SemType result = arithmeticType(left, right, op.lexeme, op.line);
        return additiveExpr2(parser, result);
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
static SemType multiplicativeExpr(Parser *parser)
{
    SemType left = factor(parser);
    return multiplicativeExpr2(parser, left);
}

// EXPR_MULTIPLICATIVA2 → OP_MULTIPLICATIVO FATOR EXPR_MULTIPLICATIVA2 | ε
static SemType multiplicativeExpr2(Parser *parser, SemType left)
{
    if (parser->currentToken.type == TOKEN_MULT ||
        parser->currentToken.type == TOKEN_DIV ||
        parser->currentToken.type == TOKEN_MOD)
    {
        Token op = parser->currentToken;
        multiplicativeOp(parser);
        SemType right = factor(parser);
        SemType result = arithmeticType(left, right, op.lexeme, op.line);
        return multiplicativeExpr2(parser, result);
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
static SemType factor(Parser *parser)
{
    switch (parser->currentToken.type)
    {
    case TOKEN_TEXT:
        match(parser, TOKEN_TEXT);
        return makeType(TYPE_TEXT, 0);
    case TOKEN_NOT:
    {
        Token op = parser->currentToken;
        match(parser, TOKEN_NOT);
        SemType operand = factor(parser);
        return notType(operand, op.line);
    }
    case TOKEN_OPEN_PAREN:
    {
        match(parser, TOKEN_OPEN_PAREN);
        SemType inner = expression(parser);
        match(parser, TOKEN_CLOSE_PAREN);
        return inner;
    }
    default:
    {
        Token signToken = parser->currentToken;
        int hasSign = sign(parser);
        SemType value = term(parser);
        if (hasSign)
        {
            return signType(value, signToken.lexeme, signToken.line);
        }
        return value;
    }
    }
}

// TERMO → id DIMENSAO2 | CONSTANTE
static SemType term(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_ID)
    {
        Token id = parser->currentToken;
        match(parser, TOKEN_ID);
        SemType varType = requireVariable(id.lexeme, id.line);
        int indexCount = dimension2(parser);
        return indexedType(varType, indexCount, id.lexeme, id.line);
    }

    return constant(parser);
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
