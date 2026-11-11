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
static IntList block(Parser *parser);
static void varList(Parser *parser);
static void idList(Parser *parser, SemType varType);
static IntList statementList(Parser *parser, IntList pending);
static IntList statement(Parser *parser);
static void readStatement(Parser *parser);
static void readTerm(Parser *parser);
static void newReadTerm(Parser *parser);
static int dimension2(Parser *parser, const char *arrayName, int level, Expr *offset);
static Expr variableValue(const char *name, int indexCount, Expr offset);
static void writeStatement(Parser *parser);
static void writeTerm(Parser *parser, ArgList *terms);
static void newWriteTerm(Parser *parser, ArgList *terms);
static IntList ifStatement(Parser *parser);
static IntList elseClause(Parser *parser, Expr condition, IntList thenNext);
static IntList whileStatement(Parser *parser);
static void assignmentStatement(Parser *parser);
static SemType complement(Parser *parser, const char *target);
static SemType functionCall(Parser *parser, const char *target);
static int arguments(Parser *parser, const char *functionName, ArgList *args);
static int newArgument(Parser *parser, const char *functionName, int position, ArgList *args);
static void addArgumentValue(ArgList *args, Attr argument);
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
    IntList next = block(parser);

    codegenEndFunction(next); // CODEGEN: backpatch da nextlist que sobrou + return implícito
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
    beginTypeDimensions(); // CODEGEN: as dimensões lidas a seguir são deste tipo
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
        recordDimension(parser->currentToken.lexeme); // CODEGEN: tamanho desta dimensão
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
        declareArrayDimensions(id.lexeme); // CODEGEN: guarda os tamanhos se for vetor
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
        declareArrayDimensions(id.lexeme); // CODEGEN: guarda os tamanhos se for vetor
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
    IntList next = block(parser);

    codegenEndFunction(next); // CODEGEN: backpatch da nextlist que sobrou + return implícito
    endFunction();
}

// BLOCO → { LISTAVARIÁVEIS COMANDOS }
// SDT: S → { L }  { S.nextlist = L.nextlist; }   (declarações não geram código)
static IntList block(Parser *parser)
{
    match(parser, TOKEN_OPEN_BRACE);
    varList(parser);
    IntList next = statementList(parser, emptylist()); // CODEGEN: L.nextlist
    match(parser, TOKEN_CLOSE_BRACE);
    return next;
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
        declareArrayDimensions(id.lexeme); // CODEGEN: guarda os tamanhos se for vetor
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
        declareArrayDimensions(id.lexeme); // CODEGEN: guarda os tamanhos se for vetor
        idList(parser, varType);
    }
}

// COMANDOS → COMANDO COMANDOS | ε
// SDT: L → L1 M S  { backpatch(L1.nextlist, M.instr); L.nextlist = S.nextlist; }
//      L → S       { L.nextlist = S.nextlist; }
// 'pending' é a nextlist dos comandos já lidos (L1). Se vier outro comando,
// ele começa em nextinstr (o M) e os desvios pendentes vão para lá.
static IntList statementList(Parser *parser, IntList pending)
{
    if (parser->currentToken.type == TOKEN_SCANF ||
        parser->currentToken.type == TOKEN_PRINTLN ||
        parser->currentToken.type == TOKEN_IF ||
        parser->currentToken.type == TOKEN_WHILE ||
        parser->currentToken.type == TOKEN_RETURN ||
        parser->currentToken.type == TOKEN_FUNC ||
        parser->currentToken.type == TOKEN_ID)
    {
        backpatch(pending, nextinstr()); // CODEGEN: backpatch(L1.nextlist, M.instr)
        IntList next = statement(parser);
        return statementList(parser, next);
    }
    return pending; // CODEGEN: sem mais comandos, a nextlist sobe para quem chamou
}

// COMANDO → LEITURA | ESCRITA | ATRIBUIÇÃO | FUNÇÃO | SELEÇÃO | ENQUANTO | RETORNO
// Devolve a nextlist do comando. Só if e while deixam desvios pendentes;
// os demais seguem para a próxima instrução (SDT: S → A ;  { S.nextlist = null; }).
static IntList statement(Parser *parser)
{
    IntList next = emptylist();

    switch (parser->currentToken.type)
    {
    case TOKEN_SCANF:
        readStatement(parser);
        break;
    case TOKEN_PRINTLN:
        writeStatement(parser);
        break;
    case TOKEN_IF:
        next = ifStatement(parser); // CODEGEN: S.nextlist
        break;
    case TOKEN_FUNC:
        functionCall(parser, NULL); // CODEGEN: chamada sem destino ("call f, n")
        break;
    case TOKEN_WHILE:
        next = whileStatement(parser); // CODEGEN: S.nextlist
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

    return next;
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
// Cada termo lido vira uma chamada sem argumentos: "id = call scanf, 0".
// Num elemento de vetor: "t = call scanf, 0" e depois "v[i] = t".
static void readTerm(Parser *parser)
{
    Token id = parser->currentToken;
    match(parser, TOKEN_ID);
    SemType varType = requireVariable(id.lexeme, id.line);
    Expr offset = makeValue(""); // CODEGEN: preenchido por dimension2 se houver índice
    int indexCount = dimension2(parser, id.lexeme, 0, &offset);
    indexedType(varType, indexCount, id.lexeme, id.line);

    if (indexCount == 0)
    {
        emitCall("scanf", 0, id.lexeme); // CODEGEN: id = call scanf, 0
        return;
    }

    char temp[ADDR_SIZE];
    newTemp(temp);
    emitCall("scanf", 0, temp);                                // CODEGEN: t = call scanf, 0
    emitArrayWrite(id.lexeme, indexCount, offset, temp);       // CODEGEN: v[i] = t
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
// CODEGEN: 'offset' acumula o índice linear de 'arrayName' (índice de
// elemento, sem largura em bytes); 'level' é a posição do índice atual.
static int dimension2(Parser *parser, const char *arrayName, int level, Expr *offset)
{
    if (parser->currentToken.type == TOKEN_OPEN_BRACKET)
    {
        match(parser, TOKEN_OPEN_BRACKET);
        int line = parser->currentToken.line;
        Attr index = additiveExpr(parser);
        requireIntIndex(index.type, line);
        *offset = arrayOffset(arrayName, level, *offset, index.code); // CODEGEN: offset * tam + índice
        match(parser, TOKEN_CLOSE_BRACKET);
        return 1 + dimension2(parser, arrayName, level + 1, offset);
    }
    return 0;
}

// CODEGEN: endereço de "id" ou, se houver índices, do elemento lido
// num temporário ("t = v[i]", a instrução x = y[i] da aula).
static Expr variableValue(const char *name, int indexCount, Expr offset)
{
    if (indexCount == 0)
    {
        return makeValue(name);
    }
    return emitArrayRead(name, indexCount, offset);
}

// ESCRITA → println ( TERMOESCRITA NOVOTERMOESCRITA ) ;
// Tratado como chamada de procedimento: um "param" por termo e "call println, n".
static void writeStatement(Parser *parser)
{
    ArgList terms;
    initArgList(&terms); // CODEGEN: endereços dos termos, na ordem

    match(parser, TOKEN_PRINTLN);
    match(parser, TOKEN_OPEN_PAREN);
    writeTerm(parser, &terms);
    newWriteTerm(parser, &terms);
    match(parser, TOKEN_CLOSE_PAREN);
    match(parser, TOKEN_SEMICOLON);

    emitParams(&terms);                          // CODEGEN: param x (um por termo)
    emitCall("println", terms.count, NULL);      // CODEGEN: call println, n
}

// TERMOESCRITA → id DIMENSAO2 | CONSTANTE | texto
// Guarda em 'terms' o endereço do termo (o próprio id, número ou texto).
static void writeTerm(Parser *parser, ArgList *terms)
{
    Token current = parser->currentToken;

    if (current.type == TOKEN_ID)
    {
        match(parser, TOKEN_ID);
        SemType varType = requireVariable(current.lexeme, current.line);
        Expr offset = makeValue(""); // CODEGEN: preenchido por dimension2 se houver índice
        int indexCount = dimension2(parser, current.lexeme, 0, &offset);
        indexedType(varType, indexCount, current.lexeme, current.line);
        Expr value = variableValue(current.lexeme, indexCount, offset); // CODEGEN: id ou t = v[i]
        addArgument(terms, value.addr);                                 // CODEGEN
        return;
    }

    if (current.type == TOKEN_TEXT)
    {
        match(parser, TOKEN_TEXT);
    }
    else
    {
        constant(parser);
    }

    addArgument(terms, current.lexeme); // CODEGEN: o próprio texto ou número
}

// NOVOTERMOESCRITA → , TERMOESCRITA NOVOTERMOESCRITA | ε
static void newWriteTerm(Parser *parser, ArgList *terms)
{
    if (parser->currentToken.type == TOKEN_COMMA)
    {
        match(parser, TOKEN_COMMA);
        writeTerm(parser, terms);
        newWriteTerm(parser, terms);
    }
}

// SELEÇÃO → if ( EXPRESSÃO ) BLOCO SENÃO
// SDT: S → if ( B ) M S1                 e
//      S → if ( B ) M1 S1 N else M2 S2
// As duas regras começam iguais; quem decide entre elas é elseClause().
static IntList ifStatement(Parser *parser)
{
    match(parser, TOKEN_IF);
    match(parser, TOKEN_OPEN_PAREN);
    int line = parser->currentToken.line;
    Attr condition = expression(parser);
    requireCondition(condition.type, "if", line);
    Expr b = toCondition(condition.code); // CODEGEN: B em código de desvio
    match(parser, TOKEN_CLOSE_PAREN);

    int m1 = nextinstr();                   // CODEGEN: M1.instr
    IntList s1 = block(parser);
    backpatch(b.truelist, m1);              // CODEGEN: backpatch(B.truelist, M1.instr)
    return elseClause(parser, b, s1);
}

// SENÃO → else BLOCO | ε
// Com else:  backpatch(B.falselist, M2.instr);
//            S.nextlist = merge(merge(S1.nextlist, N.nextlist), S2.nextlist)
// Sem else:  S.nextlist = merge(B.falselist, S1.nextlist)
static IntList elseClause(Parser *parser, Expr condition, IntList thenNext)
{
    if (parser->currentToken.type == TOKEN_ELSE)
    {
        IntList n = emitGotoPlaceholder();  // CODEGEN: N (o "then" pula o else)
        match(parser, TOKEN_ELSE);
        int m2 = nextinstr();               // CODEGEN: M2.instr
        IntList s2 = block(parser);
        backpatch(condition.falselist, m2); // CODEGEN: backpatch(B.falselist, M2.instr)
        IntList temp = merge(thenNext, n);  // CODEGEN: temp = merge(S1.nextlist, N.nextlist)
        return merge(temp, s2);             // CODEGEN: S.nextlist = merge(temp, S2.nextlist)
    }

    return merge(condition.falselist, thenNext); // CODEGEN: S.nextlist (sem else)
}

// ENQUANTO → while ( EXPRESSÃO ) BLOCO
// SDT: S → while M1 ( B ) M2 S1  { backpatch(S1.nextlist, M1.instr);
//                                  backpatch(B.truelist, M2.instr);
//                                  S.nextlist = B.falselist;
//                                  emit('goto' M1.instr); }
static IntList whileStatement(Parser *parser)
{
    match(parser, TOKEN_WHILE);
    int m1 = nextinstr();                   // CODEGEN: M1.instr (início do teste)
    match(parser, TOKEN_OPEN_PAREN);
    int line = parser->currentToken.line;
    Attr condition = expression(parser);
    requireCondition(condition.type, "while", line);
    Expr b = toCondition(condition.code); // CODEGEN: B em código de desvio
    match(parser, TOKEN_CLOSE_PAREN);

    int m2 = nextinstr();                   // CODEGEN: M2.instr (início do corpo)
    IntList s1 = block(parser);
    backpatch(s1, m1);                      // CODEGEN: backpatch(S1.nextlist, M1.instr)
    backpatch(b.truelist, m2);              // CODEGEN: backpatch(B.truelist, M2.instr)
    emitGoto(m1);                           // CODEGEN: emit('goto' M1.instr)
    return b.falselist;                     // CODEGEN: S.nextlist = B.falselist
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
        return functionCall(parser, target); // CODEGEN: id = call f, n (direto, sem temporário)
    }

    Attr value = expression(parser);
    Expr result = toValue(value.code);     // CODEGEN: booleano (ex.: a < b) vira 0/1
    emit("=", result.addr, NULL, target);  // CODEGEN: id = E.addr
    return value.type;
}

// FUNÇÃO → func id ( ARGUMENTOS ) IMPORTANTE: ADICIONAR O FUNC NO TOKEN.H E NA LEITURA DO LEXER.
// Devolve o tipo de retorno da função chamada.
// 'target' recebe o resultado ("target = call f, n"); NULL quando a chamada
// é um comando sozinho ("call f, n").
// Todos os argumentos são avaliados antes do primeiro param, para que o
// código de um argumento não fique no meio da sequência de params.
static SemType functionCall(Parser *parser, const char *target)
{
    ArgList args;
    initArgList(&args); // CODEGEN: endereços dos argumentos, na ordem

    match(parser, TOKEN_FUNC);
    Token id = parser->currentToken;
    match(parser, TOKEN_ID);
    SemType result = requireFunction(id.lexeme, id.line);
    match(parser, TOKEN_OPEN_PAREN);
    int argumentCount = arguments(parser, id.lexeme, &args);
    match(parser, TOKEN_CLOSE_PAREN);
    checkArgumentCount(id.lexeme, argumentCount, id.line);

    emitParams(&args);                             // CODEGEN: param x (um por argumento)
    emitCall(id.lexeme, argumentCount, target);    // CODEGEN: [target =] call f, n
    return result;
}

// Avalia um argumento e guarda o endereço dele (booleano vira 0/1).
static void addArgumentValue(ArgList *args, Attr argument)
{
    Expr value = toValue(argument.code); // CODEGEN: booleano (ex.: a < b) vira 0/1
    addArgument(args, value.addr);       // CODEGEN: guarda o endereço para o param
}

// ARGUMENTOS → EXPRESSÃO NOVO_ARGUMENTO | ε
// Confere o tipo de cada argumento e devolve quantos argumentos a chamada tem.
static int arguments(Parser *parser, const char *functionName, ArgList *args)
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
        addArgumentValue(args, argument); // CODEGEN
        return 1 + newArgument(parser, functionName, 1, args);
    }
    return 0;
}

// NOVO_ARGUMENTO → , EXPRESSÃO NOVO_ARGUMENTO | ε
// 'position' é a posição (a partir de 0) do próximo argumento.
static int newArgument(Parser *parser, const char *functionName, int position, ArgList *args)
{
    if (parser->currentToken.type == TOKEN_COMMA)
    {
        match(parser, TOKEN_COMMA);
        int line = parser->currentToken.line;
        Attr argument = expression(parser);
        checkArgument(functionName, position, argument.type, line);
        addArgumentValue(args, argument); // CODEGEN
        return 1 + newArgument(parser, functionName, position + 1, args);
    }
    return 0;
}

// RETORNO → return EXPRESSÃO ;
// Gera "return y", com y = endereço da expressão (booleano vira 0/1).
static void returnStatement(Parser *parser)
{
    Token returnToken = parser->currentToken;
    match(parser, TOKEN_RETURN);
    Attr value = expression(parser);
    checkReturn(value.type, returnToken.line);
    Expr result = toValue(value.code);            // CODEGEN: booleano vira 0/1
    emit("return", result.addr, NULL, NULL);      // CODEGEN: return y
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
// SDT: B → B1 || M B2  { backpatch(B1.falselist, M.instr);
//                        B.truelist = merge(B1.truelist, B2.truelist);
//                        B.falselist = B2.falselist; }
static Attr orExpr2(Parser *parser, Attr left)
{
    if (parser->currentToken.type == TOKEN_OR)
    {
        Token op = parser->currentToken;
        match(parser, TOKEN_OR);
        Expr b1 = toCondition(left.code);   // CODEGEN: B1 em código de desvio
        int m = nextinstr();                // CODEGEN: M.instr (início de B2)
        Attr right = andExpr(parser);
        Expr b2 = toCondition(right.code);  // CODEGEN: B2 em código de desvio
        SemType result = logicalType(left.type, right.type, op.lexeme, op.line);

        backpatch(b1.falselist, m);         // CODEGEN: B1 falso -> testa B2
        Expr b = makeBool(merge(b1.truelist, b2.truelist), b2.falselist); // CODEGEN
        return orExpr2(parser, makeAttr(result, b));
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
// SDT: B → B1 && M B2  { backpatch(B1.truelist, M.instr);
//                        B.truelist = B2.truelist;
//                        B.falselist = merge(B1.falselist, B2.falselist); }
static Attr andExpr2(Parser *parser, Attr left)
{
    if (parser->currentToken.type == TOKEN_AND)
    {
        Token op = parser->currentToken;
        match(parser, TOKEN_AND);
        Expr b1 = toCondition(left.code);   // CODEGEN: B1 em código de desvio
        int m = nextinstr();                // CODEGEN: M.instr (início de B2)
        Attr right = relationalExpr(parser);
        Expr b2 = toCondition(right.code);  // CODEGEN: B2 em código de desvio
        SemType result = logicalType(left.type, right.type, op.lexeme, op.line);

        backpatch(b1.truelist, m);          // CODEGEN: B1 verdadeiro -> testa B2
        Expr b = makeBool(b2.truelist, merge(b1.falselist, b2.falselist)); // CODEGEN
        return andExpr2(parser, makeAttr(result, b));
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
// SDT: B → E1 rel E2  { B.truelist = makelist(nextinstr);
//                       B.falselist = makelist(nextinstr + 1);
//                       emit('if' E1.addr rel.op E2.addr 'goto _');
//                       emit('goto _'); }
static Attr relationalExpr2(Parser *parser, Attr left)
{
    if (parser->currentToken.type == TOKEN_COMP)
    {
        Token op = parser->currentToken;
        match(parser, TOKEN_COMP);
        Attr right = additiveExpr(parser);
        SemType result = comparisonType(left.type, right.type, op.lexeme, op.line);
        Expr b = emitRelational(op.lexeme, left.code, right.code); // CODEGEN: if E1 rel E2 goto _ / goto _
        return makeAttr(result, b);
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
        // SDT: B → ! B1  { B.truelist = B1.falselist; B.falselist = B1.truelist; }
        // Não emite nada além do que B1 já emitiu: só troca as listas.
        Token op = parser->currentToken;
        match(parser, TOKEN_NOT);
        Attr operand = factor(parser);
        Expr b1 = toCondition(operand.code);           // CODEGEN: B1 em código de desvio
        Expr b = makeBool(b1.falselist, b1.truelist);  // CODEGEN: troca truelist e falselist
        return makeAttr(notType(operand.type, op.line), b);
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
        Expr offset = makeValue(""); // CODEGEN: preenchido por dimension2 se houver índice
        int indexCount = dimension2(parser, current.lexeme, 0, &offset);
        SemType result = indexedType(varType, indexCount, current.lexeme, current.line);
        Expr value = variableValue(current.lexeme, indexCount, offset); // CODEGEN: E.addr = id ou t = v[i]
        return makeAttr(result, value);
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
