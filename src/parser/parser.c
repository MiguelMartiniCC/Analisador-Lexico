#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

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
static void returnType(Parser *parser);
static void type(Parser *parser);
static void baseType(Parser *parser);
static void dimension(Parser *parser);
static void parameters(Parser *parser);
static void parameterList(Parser *parser);
static void mainFunction(Parser *parser);
static void block(Parser *parser);
static void varList(Parser *parser);
static void idList(Parser *parser);
static void statementList(Parser *parser);
static void statement(Parser *parser);
static void readStatement(Parser *parser);
static void readTerm(Parser *parser);
static void newReadTerm(Parser *parser);
static void dimension2(Parser *parser);
static void writeStatement(Parser *parser);
static void writeTerm(Parser *parser);
static void newWriteTerm(Parser *parser);
static void ifStatement(Parser *parser);
static void elseClause(Parser *parser);
static void whileStatement(Parser *parser);
static void assignmentStatement(Parser *parser);
static void complement(Parser *parser);
static void functionCall(Parser *parser);
static void arguments(Parser *parser);
static void newArgument(Parser *parser);
static void returnStatement(Parser *parser);
static void expression(Parser *parser);
static void orExpr(Parser *parser);
static void orExpr2(Parser *parser);
static void andExpr(Parser *parser);
static void andExpr2(Parser *parser);
static void relationalExpr(Parser *parser);
static void relationalExpr2(Parser *parser);
static void additiveExpr(Parser *parser);
static void additiveExpr2(Parser *parser);
static void additiveOp(Parser *parser);
static void multiplicativeExpr(Parser *parser);
static void multiplicativeExpr2(Parser *parser);
static void multiplicativeOp(Parser *parser);
static void factor(Parser *parser);
static void term(Parser *parser);
static void sign(Parser *parser);
static void constant(Parser *parser);

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
    returnType(parser);
    match(parser, TOKEN_ID);
    match(parser, TOKEN_OPEN_PAREN);
    parameters(parser);
    match(parser, TOKEN_CLOSE_PAREN);
    block(parser);
}

// TIPORETORNO → TIPO | void
static void returnType(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_VOID)
    {
        match(parser, TOKEN_VOID);
    }
    else
    {
        type(parser);
    }
}

// TIPO → TIPOBASE DIMENSÃO
static void type(Parser *parser)
{
    baseType(parser);
    dimension(parser);
}

// TIPOBASE → char | float | int | boolean
static void baseType(Parser *parser)
{
    switch (parser->currentToken.type)
    {
    case TOKEN_CHAR:
        match(parser, TOKEN_CHAR);
        break;
    case TOKEN_FLOAT:
        match(parser, TOKEN_FLOAT);
        break;
    case TOKEN_INT:
        match(parser, TOKEN_INT);
        break;
    case TOKEN_BOOLEAN:
        match(parser, TOKEN_BOOLEAN);
        break;
    default:
        printf("Erro Sintático [Linha %d]: Esperado tipo base, mas foi encontrado '%s' ('%s')\n",
               parser->currentToken.line,
               parser->currentToken.lexeme,
               tokenTypeToString(parser->currentToken.type));
        exit(EXIT_FAILURE);
    }
}

// DIMENSÃO → [ num_int ] DIMENSÃO | ε
static void dimension(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_OPEN_BRACKET)
    {
        match(parser, TOKEN_OPEN_BRACKET);
        match(parser, TOKEN_NUM_INT);
        match(parser, TOKEN_CLOSE_BRACKET);
        dimension(parser);
    }
}

// PARÂMETROS → TIPO id LISTAPARAMETROS | ε
static void parameters(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_INT ||
        parser->currentToken.type == TOKEN_FLOAT ||
        parser->currentToken.type == TOKEN_CHAR ||
        parser->currentToken.type == TOKEN_BOOLEAN)
    {
        type(parser);
        match(parser, TOKEN_ID);
        parameterList(parser);
    }
}

// LISTAPARAMETROS → , TIPO id LISTAPARAMETROS | ε
static void parameterList(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_COMMA)
    {
        match(parser, TOKEN_COMMA);
        type(parser);
        match(parser, TOKEN_ID);
        parameterList(parser);
    }
}

// PRINCIPAL → main ( ) BLOCO
static void mainFunction(Parser *parser)
{
    match(parser, TOKEN_MAIN);
    match(parser, TOKEN_OPEN_PAREN);
    match(parser, TOKEN_CLOSE_PAREN);
    block(parser);
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
        type(parser);
        match(parser, TOKEN_ID);
        idList(parser);
        match(parser, TOKEN_SEMICOLON);
        varList(parser);
    }
}

// LISTAID → , id LISTAID | ε
static void idList(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_COMMA)
    {
        match(parser, TOKEN_COMMA);
        match(parser, TOKEN_ID);
        idList(parser);
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
    match(parser, TOKEN_ID);
    dimension2(parser);
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
static void dimension2(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_OPEN_BRACKET)
    {
        match(parser, TOKEN_OPEN_BRACKET);
        additiveExpr(parser);
        match(parser, TOKEN_CLOSE_BRACKET);
        dimension2(parser);
    }
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
        match(parser, TOKEN_ID);
        dimension2(parser);
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
    expression(parser);
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
    expression(parser);
    match(parser, TOKEN_CLOSE_PAREN);
    block(parser);
}

// ATRIBUIÇÃO → id = COMPLEMENTO ;
static void assignmentStatement(Parser *parser)
{
    match(parser, TOKEN_ID);
    match(parser, TOKEN_ATRIB);
    complement(parser);
    match(parser, TOKEN_SEMICOLON);
}

// COMPLEMENTO → EXPRESSÃO | FUNÇÃO]
static void complement(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_OPEN_PAREN)
    {
        functionCall(parser);
    }
    else
    {
        expression(parser);
    }
}

// FUNÇÃO → func id ( ARGUMENTOS ) IMPORTANTE: ADICIONAR O FUNC NO TOKEN.H E NA LEITURA DO LEXER.
static void functionCall(Parser *parser)
{
    // match(parser, TOKEN_FUNC);
    match(parser, TOKEN_OPEN_PAREN);
    match(parser, TOKEN_ID);
    arguments(parser);
    match(parser, TOKEN_CLOSE_PAREN);
}

// ARGUMENTOS → EXPRESSÃO NOVO_ARGUMENTO | ε
static void arguments(Parser *parser)
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
        expression(parser);
        newArgument(parser);
    }
}

// NOVO_ARGUMENTO → , EXPRESSÃO NOVO_ARGUMENTO | ε
static void newArgument(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_COMMA)
    {
        match(parser, TOKEN_COMMA);
        expression(parser);
        newArgument(parser);
    }
}

// RETORNO → return EXPRESSÃO ;
static void returnStatement(Parser *parser)
{
    match(parser, TOKEN_RETURN);
    expression(parser);
    match(parser, TOKEN_SEMICOLON);
}

// EXPRESSÃO → EXPR_OU
static void expression(Parser *parser)
{
    orExpr(parser);
}

// EXPR_OU → EXPR_E EXPR_OU2
static void orExpr(Parser *parser)
{
    andExpr(parser);
    orExpr2(parser);
}

// EXPR_OU2 → || EXPR_E EXPR_OU2 | ε
static void orExpr2(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_OR)
    {
        match(parser, TOKEN_OR);
        andExpr(parser);
        orExpr2(parser);
    }
}

// EXPR_E → EXPR_RELACIONAL EXPR_E2
static void andExpr(Parser *parser)
{
    relationalExpr(parser);
    andExpr2(parser);
}

// EXPR_E2 → && EXPR_RELACIONAL EXPR_E2 | ε
static void andExpr2(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_AND)
    {
        match(parser, TOKEN_AND);
        relationalExpr(parser);
        andExpr2(parser);
    }
}

// EXPR_RELACIONAL → EXPR_ADITIVA EXPR_ RELACIONAL2
static void relationalExpr(Parser *parser)
{
    additiveExpr(parser);
    relationalExpr2(parser);
}

// EXPR_RELACIONAL2 → comp EXPR_ADITIVA | ε
static void relationalExpr2(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_COMP)
    {
        match(parser, TOKEN_COMP);
        additiveExpr(parser);
    }
}

// EXPR_ADITIVA → EXPR_MULTIPLICATIVA EXPR_ADITIVA2
static void additiveExpr(Parser *parser)
{
    multiplicativeExpr(parser);
    additiveExpr2(parser);
}

// EXPR_ADITIVA2 → OP_ADITIVO EXPR_MULTIPLICATIVA EXPR_ADITIVA2 | ε
static void additiveExpr2(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_SUM ||
        parser->currentToken.type == TOKEN_SUB)
    {
        additiveOp(parser);
        multiplicativeExpr(parser);
        additiveExpr2(parser);
    }
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
static void multiplicativeExpr(Parser *parser)
{
    factor(parser);
    multiplicativeExpr2(parser);
}

// EXPR_MULTIPLICATIVA2 → OP_MULTIPLICATIVO FATOR EXPR_MULTIPLICATIVA2 | ε
static void multiplicativeExpr2(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_MULT ||
        parser->currentToken.type == TOKEN_DIV ||
        parser->currentToken.type == TOKEN_MOD)
    {
        multiplicativeOp(parser);
        factor(parser);
        multiplicativeExpr2(parser);
    }
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
static void factor(Parser *parser)
{
    switch (parser->currentToken.type)
    {
    case TOKEN_TEXT:
        match(parser, TOKEN_TEXT);
        break;
    case TOKEN_NOT:
        match(parser, TOKEN_NOT);
        factor(parser);
        break;
    case TOKEN_OPEN_PAREN:
        match(parser, TOKEN_OPEN_PAREN);
        expression(parser);
        match(parser, TOKEN_CLOSE_PAREN);
        break;
    default:
        sign(parser);
        term(parser);
        break;
    }
}

// TERMO → id DIMENSAO2 | CONSTANTE
static void term(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_ID)
    {
        match(parser, TOKEN_ID);
        dimension2(parser);
    }
    else
    {
        constant(parser);
    }
}

// SINAL → + | - | ε
static void sign(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_SUM)
    {
        match(parser, TOKEN_SUM);
    }
    else if (parser->currentToken.type == TOKEN_SUB)
    {
        match(parser, TOKEN_SUB);
    }
}

// CONSTANTE → num_int | num_dec
static void constant(Parser *parser)
{
    if (parser->currentToken.type == TOKEN_NUM_INT)
    {
        match(parser, TOKEN_NUM_INT);
    }
    else if (parser->currentToken.type == TOKEN_NUM_DEC)
    {
        match(parser, TOKEN_NUM_DEC);
    }
    else
    {
        printf("Erro Sintático [Linha %d]: Esperado constante, mas foi encontrado '%s' ('%s')\n",
               parser->currentToken.line,
               parser->currentToken.lexeme,
               tokenTypeToString(parser->currentToken.type));
        exit(EXIT_FAILURE);
    }
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
