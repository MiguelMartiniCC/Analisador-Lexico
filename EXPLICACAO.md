# Explicação do analisador léxico e sintático

Este programa é um reconhecedor da linguagem da disciplina. Ele lê um arquivo-fonte, quebra o texto em tokens e depois confere se essa sequência obedece à gramática. Ainda não há análise semântica nem geração de código: o resultado é aceitar o arquivo ou parar no primeiro erro sintático.

O código está em C. Um arquivo `.h` declara o que os outros arquivos podem usar (a interface). O `.c` correspondente contém o corpo das funções. Uma `struct` é um registro com campos. Um `enum` é uma lista de nomes que o compilador de C numera por baixo dos panos: `TOKEN_ID`, `TOKEN_INT` e os demais são categorias de token, não texto. Um ponteiro (`Lexer *`, `FILE *`) é o endereço de um objeto já existente; a função recebe o endereço para ler e alterar o original, em vez de uma cópia.

## Fluxo completo

O exemplo válido é [`testes/compiler/teste.txt`](testes/compiler/teste.txt): uma função `soma` e, depois, `main`. A execução, comandada por [`src/main.c`](src/main.c), faz duas leituras do mesmo arquivo.

```text
teste.txt
    |
    v
fopen
    |
    v
initSymbolTable
    |
    v
1a passagem: getNextToken até EOF
    |                          |
    v                          v
Imprime cada token     Insere só identificadores
    |                          |
    +------------+-------------+
                 |
                 v
fseek para o byte 0 e initLexer
                 |
                 v
2a passagem: parse pede token sob demanda
                 |
                 v
EOF depois de main: sucesso
```

Na primeira passagem o lexer percorre o arquivo até o fim e o `main` imprime a lista. Cada identificador (`soma`, `a`, `resultado`, `idade`, …) entra na tabela de símbolos. Palavra reservada (`void`, `int`, `main`, `return`) vira token próprio e fica de fora da tabela.

A teoria costuma desenhar “lexer produz a fita de tokens e o parser consome a fita”. Aqui a fita não é guardada. O `main` volta o cursor do arquivo para o byte 0 com `fseek`, zera a linha do lexer e o parser pede o próximo token na hora em que precisa dele. A tabela não é apagada nessa segunda leitura. Como `insertSymbol` devolve o índice de um nome que já existe, a tabela impressa no fim da primeira passagem continua com uma entrada por nome.

[`testes/compiler/teste_erro.txt`](testes/compiler/teste_erro.txt) é o mesmo programa sem o `;` de `ok = 0`. O lexer termina normalmente. O parser aceita `ok = 0` e, ao exigir o `;`, encontra `scanf` e encerra o processo.

## `include/token.h` — o vocabulário

```c
typedef enum {
    TOKEN_ID,
    TOKEN_NUM_INT,
    TOKEN_NUM_DEC,
    TOKEN_TEXT,
    /* palavras reservadas, operadores, símbolos */
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[256];
    int line;
} Token;
```

`TokenType` é o conjunto de categorias que a análise léxica pode produzir. `Token` é um token concreto: a categoria (`type`), o texto que originou o token (`lexeme`, no máximo 255 caracteres mais o terminador) e a linha. Na teoria, o lexema é a sequência de caracteres casada pelo padrão; o tipo é a classe desse padrão.

Três decisões desse arquivo mudam o resto do compilador:

- `<`, `<=`, `>`, `>=`, `==` e `!=` são todos `TOKEN_COMP`. O parser só pergunta “é comparação?”. Qual comparação foi, fica no lexema.
- `for` existe como `TOKEN_FOR`, ao lado de `if` e `while`. A gramática do parser não tem comando `for`.
- `TOKEN_EOF` marca o fim do arquivo. `TOKEN_ERROR` marca um erro léxico. O lexer devolve esse token e segue; quem aborta a execução é o parser, mais adiante, quando não consegue encaixar esse token em produção nenhuma.

`tokenTypeToString` está declarada aqui e implementada no lexer. Ela só serve para imprimir o nome da categoria.

## `include/lexer.h` — o estado do analisador léxico

```c
typedef struct {
    FILE *file;
    int currentLine;
} Lexer;

void initLexer(Lexer *lexer, FILE *file);
Token getNextToken(Lexer *lexer);
```

O estado do lexer é pequeno de propósito: o arquivo aberto e a linha atual. Não há buffer de token. Cada chamada de `getNextToken` continua de onde a leitura parou, porque `FILE` guarda a posição do próximo caractere. `currentLine` começa em 1 e sobe a cada `\n`, para a mensagem de erro dizer a linha do fonte, não a posição em bytes.

## `src/lexer/lexer.c` — o autômato escrito à mão

Não há Flex nem tabela de transição gerada. `getNextToken` é o autômato: olha o caractere atual, escolhe uma classe e consome o lexema inteiro antes de devolver o token. As funções auxiliares são `static` quando só este arquivo precisa delas. Em C, `static` numa função esconde o nome dos outros `.c`.

### Palavras reservadas e `lookupKeyword`

```c
static const Keyword keywords[] = {
    {"int",     TOKEN_INT},
    {"float",   TOKEN_FLOAT},
    {"char",    TOKEN_CHAR},
    {"boolean", TOKEN_BOOLEAN},
    {"void",    TOKEN_VOID},
    {"if",      TOKEN_IF},
    {"else",    TOKEN_ELSE},
    {"for",     TOKEN_FOR},
    {"while",   TOKEN_WHILE},
    {"scanf",   TOKEN_SCANF},
    {"println", TOKEN_PRINTLN},
    {"main",    TOKEN_MAIN},
    {"return",  TOKEN_RETURN},
    {"func",    TOKEN_FUNC},
};
```

O padrão de identificador é `L(L|D)*`, com `L` letra ou `_` e `D` dígito. Esse padrão também casa `int`, `main` e `func`. A separação entre identificador e palavra reservada não está na expressão regular: depois de montar a palavra, `lookupKeyword` percorre o vetor com `strcmp` (comparação exata, sensível a maiúsculas). Achou, devolve o tipo reservado. Não achou, devolve `TOKEN_ID`. `Int` seria identificador; `int` é palavra reservada.

`keywordCount` é calculado com `sizeof(keywords) / sizeof(keywords[0])`: tamanho do vetor dividido pelo tamanho de um elemento. Incluir uma palavra nova no vetor atualiza a contagem sem um número mágico.

### `tokenTypeToString`

É um `switch` do enum para um texto curto (`"ID"`, `"NUM_INT"`, `"="`, `"FIM_ARQUIVO"`). O `default` devolve `"ERRO"`, então `TOKEN_ERROR` e qualquer valor inesperado caem na mesma string. Serve à impressão da lista de tokens, da tabela e das mensagens do parser.

### `initLexer` e `makeToken`

`initLexer` guarda o `FILE` e põe `currentLine` em 1. É chamada duas vezes no `main`: no começo e de novo depois do `fseek`.

`makeToken` preenche os três campos do `Token`. `strncpy` copia o lexema e a linha seguinte força um `\0` no último byte. Em C, string é um vetor de caracteres terminado por zero; sem esse byte, quem for imprimir o lexema leria memória além do campo.

### `getNextToken` — descarte de espaço e comentário

O laço externo repete enquanto houver espaço ou comentário. `fgetc` lê um caractere (devolve um `int`, porque `EOF` não cabe num `char`). `isspace` reconhece espaço, tab, quebra de linha e outros brancos. Cada `\n` incrementa `currentLine`.

Se o caractere que sobrou é `/`, o lexer lê mais um. Segundo `/`: é comentário `//`, e ele consome até o fim da linha, conta essa linha e volta ao laço para ignorar o que vier de espaço. Qualquer outra coisa depois da primeira barra: `ungetc` devolve esse caractere ao arquivo e o `/` será tratado depois como divisão. `ungetc` é o lookahead de um caractere. A teoria chama isso de “espreitar o próximo símbolo e devolvê-lo se não pertence a este token”.

Comentário e branco não viram token. O parser nunca os vê.

### Identificador

Se o caractere é letra ou `_`, o lexer acumula letras, dígitos e `_` num buffer de 255 caracteres. O caractere que quebrou o padrão é devolvido com `ungetc`, porque pertence ao próximo token. Em seguida `lookupKeyword` decide o tipo. Só no caso `TOKEN_ID` o nome vai para `insertSymbol`. Em `void soma(int a, float b)`, `soma`, `a` e `b` entram na tabela; `void`, `int` e `float` não.

Se o nome passar de 255 caracteres, os extras são lidos e descartados do lexema, mas saem do arquivo. Dois nomes longos que coincidam nos primeiros 255 caracteres viram o mesmo lexema.

### Número inteiro e decimal

Dígito abre `D+`. Enquanto houver dígito, o lexer acumula. Se aparecer `.`, ele espreita o próximo caractere:

- dígito: é `D+.D+`, token `TOKEN_NUM_DEC` (`3.14` no teste);
- qualquer outra coisa: é o erro léxico do decimal incompleto (`3.`). O ponto entra no lexema, o caractere seguinte volta ao arquivo, e o token é `TOKEN_ERROR`.

Sem ponto, o token é `TOKEN_NUM_INT` e o caractere que parou a leitura volta ao arquivo. O sinal de um número negativo não faz parte deste token. `-3` sai como `TOKEN_SUB` seguido de `TOKEN_NUM_INT`. Quem entende o menos unário é a gramática, na produção `sign`.

### Texto

Aspas abrem `TOKEN_TEXT`. O lexer copia até a aspa de fechamento, sem atravessar linha nem `EOF`. Aspas fechadas: o lexema inclui as duas aspas, como em `"Resultado:"`. Fim de linha ou de arquivo antes de fechar: `TOKEN_ERROR`, e o `\n` ou `EOF` é devolvido para não ser engolido. Não há sequência de escape: `\"` não é tratado de forma especial.

### Operadores e símbolos

O `switch` final trata um caractere que já não é letra, dígito nem aspas. Os casos de dois caracteres espreitam o seguinte:

| Entrada | Se o próximo confirma | Senão |
|---|---|---|
| `=` | `==` vira `TOKEN_COMP` | `=` vira `TOKEN_ATRIB` |
| `!` | `!=` vira `TOKEN_COMP` | `!` vira `TOKEN_NOT` |
| `<` e `>` | `<=` e `>=` viram `TOKEN_COMP` | `<` e `>` também são `TOKEN_COMP` |
| `&` e `\|` | `&&` e `\|\|` são `TOKEN_AND` e `TOKEN_OR` | o caractere sozinho é `TOKEN_ERROR` |

`+ - * / %` e `( ) [ ] { } , ;` são tokens de um caractere, com tipo próprio. Qualquer outro caractere (`@`, `#`, …) vira `TOKEN_ERROR` com aquele caractere como lexema. A função retorna; a próxima chamada continua no caractere seguinte. Erro léxico não interrompe a varredura.

## Tabela de símbolos

Arquivos: [`include/symbol_table.h`](include/symbol_table.h) e [`src/symbol_table/symbol_table.c`](src/symbol_table/symbol_table.c).

```c
static Symbol symbolTable[MAX_SYMBOLS];
static int symbolCount = 0;

int insertSymbol(const char *name, TokenType type) {
    for (int i = 0; i < symbolCount; i++) {
        if (strcmp(symbolTable[i].name, name) == 0) {
            return i;
        }
    }
    if (symbolCount < MAX_SYMBOLS) {
        strcpy(symbolTable[symbolCount].name, name);
        symbolTable[symbolCount].type = type;
        symbolCount++;
        return symbolCount - 1;
    }
    return -1;
}
```

`Symbol` tem `name` e `type`. A tabela é um vetor global de 100 posições, visível só neste arquivo, mais um contador. `initSymbolTable` zera o contador. O `main` chama isso uma vez, antes da primeira passagem.

`insertSymbol` faz busca linear. Nome já presente: devolve o índice antigo e não altera o tipo. Nome novo e ainda há vaga: copia o nome, grava o tipo e devolve o índice novo. Tabela cheia: devolve `-1`. O lexer ignora esse retorno, então o 101º identificador diferente continua sendo token `TOKEN_ID` e simplesmente não aparece na tabela.

O tipo gravado é sempre `TOKEN_ID`, porque é isso que o lexer passa. A tabela desta fase responde “quais nomes apareceram”, não “qual é o tipo declarado de `idade`” nem “em que escopo”. Declaração, uso antes de declarar e compatibilidade de tipo ficam para a análise semântica, que ainda não existe. `printSymbolTable` percorre o contador e imprime índice, nome e o texto do tipo.

No `teste.txt`, a ordem de inserção é a ordem do primeiro aparecimento: `soma`, `a`, `b`, `resultado`, `idade`, `i`, `x`, `pi`, `ok`, `nome`. `i` entra mesmo sem ser usado num comando. A tabela léxica não distingue declaração de uso.

## `src/main.c` — quem amarra as duas fases

`argc` é a quantidade de argumentos da linha de comando; `argv[0]` é o nome do programa e `argv[1]` é o arquivo-fonte. Sem esse argumento, o programa explica o uso e devolve 1. `fopen` no modo `"r"` abre o texto. Se falhar, `perror` imprime o motivo do sistema (`No such file or directory`, por exemplo) e também devolve 1.

Depois da tabela e do lexer inicializados, um `do-while` chama `getNextToken` e imprime linha, categoria e lexema até receber `TOKEN_EOF`. O `do-while` garante que o `EOF` também seja impresso. Em seguida vem `printSymbolTable`.

```c
fseek(file, 0, SEEK_SET);
initLexer(&lexer, file);

printf("\n=== ANÁLISE SINTÁTICA ===\n");

Parser parser;
initParser(&parser, &lexer);
parse(&parser);
```

`fseek(..., 0, SEEK_SET)` coloca a leitura de volta no primeiro byte. `initLexer` repõe a linha em 1; sem isso, a segunda passagem continuaria a numeração de onde a primeira parou. `initParser` só guarda o ponteiro do lexer. `parse` é a análise sintática inteira. `fclose` roda se `parse` retornar. Num erro sintático isso não acontece: `match` chama `exit` e o processo termina ali.

O retorno 0 do `main` significa que as duas fases chegaram ao fim. Um erro léxico no meio do arquivo não muda esse retorno: o token `ERRO` foi impresso, e o parser é que decide se a sequência ainda é uma frase da linguagem.

## `include/parser.h` — o lookahead

```c
typedef struct {
    Lexer *lexer;
    Token currentToken;
} Parser;

void initParser(Parser *parser, Lexer *lexer);
void parse(Parser *parser);
```

`currentToken` é o lookahead: o único token que a gramática está olhando agora. Parser preditivo LL(1) escolhe a produção com base nesse token, sem voltar atrás. O ponteiro do lexer está aqui para `advance` poder pedir o próximo.

## `src/parser/parser.c` — um não-terminal, uma função

A gramática está nos comentários acima de cada função. O método é descida recursiva: reconhecer um não-terminal é chamar a função dele, que chama as funções dos símbolos do lado direito. As funções das produções são `static`; a interface pública é só `initParser` e `parse`. Há um bloco de protótipos no topo porque em C uma função precisa ser declarada antes do uso, e várias produções se chamam mutuamente (`expression` chama `factor`, que chama `expression` de novo).

Não há árvore. Nenhuma função guarda operador nem operando. Se a sequência casa, a função retorna. Se não casa, o processo termina. Isto é um reconhecedor.

### `advance` e `match`

`advance` substitui `currentToken` pelo retorno de `getNextToken`. É o “consumir o lookahead”.

`match` compara o tipo do lookahead com o tipo esperado. Iguais: avança. Diferentes: imprime a linha, o que esperava, o lexema encontrado e a categoria encontrada, e chama `exit`. Não há modo pânico, não há sincronização em `;` ou `}`. O primeiro erro sintático é o último relatório.

`match` olha só o tipo. `<=` e `!=` satisfazem o mesmo `match(TOKEN_COMP)`.

### `parse` e `program`

`parse` avança uma vez, para o primeiro token real, e chama `program`. `initParser` não faz esse avanço: antes de `parse`, `currentToken` não foi preenchido.

`program` é `LISTAFUNÇÕES PRINCIPAL`: zero ou mais funções e, obrigatoriamente, o `main`. No teste, `functionList` consome `void soma(...) { ... }` e `mainFunction` consome o resto. Se depois do `main` ainda houver token que não seja `EOF`, `parse` reporta token inesperado após o fim do programa. Se o lookahead for `EOF`, imprime o sucesso.

### Lista de funções e declaração

`functionList` implementa `DECFUNÇÃO LISTAFUNÇÕES | ε`. O conjunto FIRST de uma declaração é o tipo de retorno: `int`, `float`, `char`, `boolean`, `void`. Lookahead nesse conjunto: chama `functionDecl` e depois `functionList` de novo (a recursão é a repetição da lista). Lookahead fora desse conjunto: a função retorna sem consumir nada. Isso é a produção vazia, o épsilon. Em `teste.txt`, ao terminar `soma` o lookahead é `main`. `main` não é tipo, então a lista acaba e `program` segue para `mainFunction`.

Há um teste interno `currentToken == TOKEN_MAIN` que pede retorno imediato. Ele está dentro do ramo que só entra com tipo. `TOKEN_MAIN` nunca é, ao mesmo tempo, `int`, `float`, `char`, `boolean` ou `void`. Esse `if` não executa. O épsilon ao ver `main` já acontece pelo `if` de fora.

`functionDecl` é `TIPORETORNO id ( PARÂMETROS ) BLOCO`. Para `soma`: `returnType` consome `void`, `match` exige um `TOKEN_ID` (`soma`), abre parêntese, `parameters` lê `int a, float b`, fecha parêntese e `block` lê o corpo.

`returnType` é `void` ou `type`. `type` é `baseType` seguido de `dimension`. `baseType` aceita `char`, `float`, `int` e `boolean`; outro token gera uma mensagem específica de “tipo base” e encerra. `dimension` é `[ num_int ]` repetido, ou épsilon. Por isso a declaração do teste é `char[10] nome`: o `[10]` faz parte do tipo, e o tamanho tem de ser inteiro literal. Várias dimensões seguem em sequência (`int[2][3] matriz`) porque `dimension` chama a si mesma.

`parameters` é `TIPO id LISTAPARAMETROS` ou épsilon. O épsilon é o que permite `main()` e também uma função sem parâmetros. O FIRST aqui não inclui `void`: um parâmetro `void` não é aceito; `void` só aparece como tipo de retorno. `parameterList` consome `, tipo id` enquanto houver vírgula.

`int main() { }` não é o programa principal. `int` faz `functionList` entrar em `functionDecl`, o tipo é consumido, e o `match(TOKEN_ID)` falha porque `main` foi classificado como `TOKEN_MAIN`, não como identificador. A forma que a gramática aceita é a palavra `main` seguida de `( )` e um bloco, em `mainFunction`.

### Bloco, declarações e comandos

`block` é `{ LISTAVARIÁVEIS COMANDOS }`. As duas listas são épsilon-capazes, então `{ }` é um bloco válido. A ordem é fixa: toda declaração local vem antes de qualquer comando. Isso é a produção, não uma checagem semântica.

`varList` olha se o lookahead é um tipo. Se for, lê tipo, um `id`, a lista de outros nomes (`idList`, vírgulas), exige `;` e repete. `int idade, i, x;` é uma entrada: `type` consome `int`, `match` consome `idade`, `idList` consome `, i, x`. Quando o lookahead deixa de ser tipo, a lista acaba. No `main` do teste, o primeiro comando é `idade = 20`. `idade` é `TOKEN_ID`, não é tipo, então `varList` retorna e `statementList` assume.

`statementList` só entra em `statement` se o lookahead for `scanf`, `println`, `if`, `while`, `return` ou `id`. Esse é o FIRST que o código realmente usa para comando. Fora disso, épsilon: a lista de comandos terminou e `block` espera `}`.

`statement` redistribui pelo mesmo lookahead: leitura, escrita, `if`, `while`, `return` ou atribuição. Existe um `case TOKEN_FUNC`, mas `statementList` nunca chama `statement` quando o token é `func`. Pelo fluxo normal, esse caso não roda. Há também um `default` com a mensagem “esperado comando”; ele também não roda a partir de `statementList`, porque a lista só chama `statement` nos tipos que o `switch` já cobre.

### Leitura, escrita e índice de vetor

`readStatement` é `scanf ( TERMOLEITURA NOVOTERMOLEITURA ) ;`. `readTerm` é um `id` com `dimension2` opcional. `newReadTerm` repete `, termo` enquanto houver vírgula. `scanf(idade, pi);` do teste passa por aqui.

`dimension2` é diferente de `dimension`. Na declaração, o tamanho é `num_int`. No uso, o índice é uma expressão aditiva (`EXPR_ADITIVA`), e pode haver vários colchetes. `nome[i + 1]` seria aceito aqui; `char[i] nome` não, porque na declaração `dimension` exige literal inteiro.

`writeStatement` é `println ( TERMOESCRITA NOVOTERMOESCRITA ) ;`. `writeTerm` aceita identificador com índice, ou texto, ou constante numérica. `println("Resultado:", resultado);` usa texto e depois, pela vírgula, um identificador.

### `if`, `else` e `while`

`ifStatement` é `if ( EXPRESSÃO ) BLOCO SENÃO`. A condição reusa a expressão geral. O corpo é um bloco, com chaves obrigatórias. `elseClause` consome `else BLOCO` só se o lookahead for `TOKEN_ELSE`; senão é épsilon. Não há `else` pendurado num comando sem bloco, e não há ambiguidade de `else` para resolver no código: o `else` só aparece depois de um bloco já fechado.

`whileStatement` tem a mesma forma, sem `else`: `while ( EXPRESSÃO ) BLOCO`. O `while (x <= 5) { ... }` do teste entra por `statementList` por causa de `TOKEN_WHILE`.

### Atribuição, complemento e chamada

`assignmentStatement` é `id = COMPLEMENTO ;`. O identificador à esquerda não leva índice: `nome[0] = ...` não é esta produção. `complement` olha o lookahead. `TOKEN_FUNC`: chama `functionCall`. Qualquer outro caso: chama `expression`.

`functionCall` é `func id ( ARGUMENTOS )`. O comentário no fonte pede para acrescentar `func` no token e no lexer; isso já está feito (`TOKEN_FUNC` e a entrada `"func"` nas palavras reservadas). A chamada não consome `;`. Quem consome é a atribuição. Por isso a linha válida do teste é:

```text
idade = func soma(idade, pi);
```

`arguments` reconhece uma expressão se o lookahead pode começar expressão: `+`, `-`, `!`, `(`, identificador, número ou texto. `newArgument` repete `, EXPRESSÃO`. Lista vazia é épsilon, então `func foo()` é aceito no complemento.

Um comando solto `func soma(idade, pi);` não entra em `statementList`. A lista trata `func` como fim dos comandos e `block` exige `}`. A mensagem fala em `}` esperado, não em chamada de função.

### Retorno e a escada de expressões

`returnStatement` é `return EXPRESSÃO ;`. `return resultado;` e `return 0;` usam a mesma produção. Não existe `return;` sem expressão: `expression` sempre exige um fator.

`expression` é só a porta de entrada. A precedência está na ordem das chamadas, de baixo para cima na prioridade:

| Função | Gramática | Operador que ela reconhece por fora |
|---|---|---|
| `orExpr` / `orExpr2` | `EXPR_E` seguido de `\|\| EXPR_E` repetido | `\|\|` |
| `andExpr` / `andExpr2` | `EXPR_RELACIONAL` seguido de `&&` repetido | `&&` |
| `relationalExpr` / `relationalExpr2` | `EXPR_ADITIVA` e, no máximo, um `comp` | `< <= > >= == !=` |
| `additiveExpr` / `additiveExpr2` | `EXPR_MULTIPLICATIVA` seguido de `+` ou `-` | `+` `-` |
| `multiplicativeExpr` / `multiplicativeExpr2` | `FATOR` seguido de `*` `/` `%` | `*` `/` `%` |
| `factor` | texto, `! FATOR`, `( EXPRESSÃO )`, ou sinal e termo | os operadores unários e o parêntese |

As funções com sufixo `2` são a eliminação da recursão à esquerda. Uma gramática do tipo `E → E + T | T` chama `E` antes de consumir qualquer token, e a descida recursiva entraria em loop. A forma usada aqui é `E → T E2` e `E2 → + T E2 | ε`. No código, `additiveExpr` reconhece o primeiro operando e `additiveExpr2` pergunta: o lookahead é `+` ou `-`? Se for, consome o operador, reconhece o próximo operando e chama a si mesma. Se não for, retorna. Esse retorno é o épsilon. O mesmo desenho vale para `orExpr2`, `andExpr2` e `multiplicativeExpr2`.

`relationalExpr2` não repete. A produção comentada é `comp EXPR_ADITIVA | ε`, um único comparador. `a < b < c` reconhece `a < b` e deixa o segundo `<` para o chamador, que não espera comparação.

`additiveOp` e `multiplicativeOp` são o lugar onde o operador é de fato consumido. O `switch` chama `match` no tipo correspondente. O `default` deles, na prática, não roda: quem chama já testou o lookahead.

`factor` é o átomo da expressão, com quatro formas:

- `TOKEN_TEXT` consome o texto;
- `TOKEN_NOT` consome `!` e chama `factor` de novo, então `!!ok` é aceito e `!` liga mais forte que `&&`;
- `TOKEN_OPEN_PAREN` consome `(`, chama `expression` inteira (pode haver `||` lá dentro) e exige `)`;
- qualquer outro lookahead cai em `sign` seguido de `term`.

`sign` consome um `+` ou um `-` se estiverem na frente, ou não consome nada. É o épsilon do sinal. `term` é identificador com `dimension2`, ou `constant`. `constant` aceita `TOKEN_NUM_INT` ou `TOKEN_NUM_DEC` e, nos outros casos, emite “esperado constante”.

`boolean` não tem literais `true` e `false`. `ok = 0` é sintaticamente uma atribuição de inteiro. A incompatibilidade com `boolean` seria regra semântica.

### A condição do `if`, token por token

A linha `if (idade >= 18 && !ok || idade != 0)` cai em `expression` e depois em `orExpr`:

1. `andExpr` reconhece `idade >= 18` em `relationalExpr` (`>=` é um `TOKEN_COMP`) e, em `andExpr2`, vê `&&`, então reconhece `!ok`. O `!` é `factor`; `ok` é `sign` vazio mais `term`.
2. O próximo token é `||`. `andExpr2` não trata `||`, então retorna épsilon.
3. `orExpr2` vê `||`, consome, e um novo `andExpr` reconhece `idade != 0`.

A frase aceita é `(idade >= 18 && !ok) || (idade != 0)`. Nenhuma tabela de precedência foi consultada. A prioridade é a profundidade da chamada.

### `resultado = a + b` função por função

Dentro do bloco de `soma`, `statementList` vê `TOKEN_ID` e chama `statement`, que chama `assignmentStatement`:

1. `match(TOKEN_ID)` consome `resultado`.
2. `match(TOKEN_ATRIB)` consome `=`.
3. `complement` não vê `func`, então chama `expression`.
4. A escada desce até `multiplicativeExpr`, depois `factor`, depois `sign` (épsilon, porque `a` não é `+` nem `-`) e `term`, que consome o `id` `a`.
5. `multiplicativeExpr2` vê `+`, que não é `*`, `/` nem `%`, e retorna.
6. `additiveExpr2` vê `+`, consome com `additiveOp`, e um novo `multiplicativeExpr` consome `b`.
7. As funções `2` acima devolvem épsilon ao ver `;`.
8. `assignmentStatement` faz `match(TOKEN_SEMICOLON)`.

Se o `;` faltar, como em `teste_erro.txt` na linha de `ok = 0`, o passo 8 encontra `scanf`. A mensagem é “esperado `;`, encontrado `scanf`”, na linha de `scanf`, porque o lookahead já avançou até o primeiro token que não pertence à expressão.

## O que o código faz de diferente do que o comentário sugere

- O `if (TOKEN_MAIN)` dentro de `functionList` é inalcançável. A parada correta da lista é o `if` externo falhar.
- Chamada de função só é alcançada por `complement`, no lado direito de uma atribuição. O `case TOKEN_FUNC` de `statement` não está no FIRST usado por `statementList`.
- `for` vira token e não abre comando. Num bloco, `statementList` trata `for` como fim da lista e `block` reclama de `}` esperado.
- `additiveExpr2` recursivo à direita agruparia `a - b - c` como `a - (b - c)` se um dia as funções passassem a montar árvore. Hoje as duas associações são aceitas do mesmo jeito, porque o parser só verifica a forma.
- Mais de 100 identificadores diferentes: a partir do 101º, `insertSymbol` devolve `-1` e o lexer segue. A lista de tokens continua completa; a tabela, não.

## Perguntas

Cada uma diz onde olhar e qual é a resposta compatível com este código.

### Nível médio

**1.** Em `getNextToken`, o que `ungetc` evita no caso de `=` e no caso de `/`?

Olhe os dois pontos em que o lexer lê um caractere extra e pode devolvê-lo: o descarte de comentário e o `case '='`.

Resposta: em `=`, o caractere extra distingue atribuição de `==`. Se não for o segundo `=`, ele pertence ao token seguinte e precisa voltar ao arquivo; senão o próximo `getNextToken` perderia esse caractere. Em `/`, o extra distingue divisão de `//`. Se não for outro `/`, volta ao arquivo e o primeiro `/` segue para o `case '/'`, que emite `TOKEN_DIV`.

**2.** Por que `match` não tenta outra produção quando o tipo não confere?

Olhe `match` e qualquer `if` que escolhe produção antes de consumir, como `functionList` e `orExpr2`.

Resposta: a escolha já foi feita pelo lookahead, olhando o FIRST da produção. `match` só confirma um terminal que aquela produção se comprometeu a consumir. Se o tipo diverge, a frase não pertence à linguagem e o parser encerra. Não há retrocesso.

**3.** O que `insertSymbol` faz quando o mesmo nome aparece de novo, e por que a segunda passagem do `main` não duplica a tabela?

Olhe o laço de `insertSymbol` e a ausência de um segundo `initSymbolTable` depois do `fseek`.

Resposta: o nome existente devolve o índice antigo, sem nova cópia e sem trocar o tipo. A segunda passagem chama `insertSymbol` outra vez para cada `id`, mas todos já foram inseridos na primeira. A tabela impressa permanece com uma linha por nome.

**4.** Qual é o papel de `fseek` junto com a segunda chamada de `initLexer`?

Olhe o trecho do `main` entre `printSymbolTable` e `initParser`.

Resposta: a primeira passagem deixa o `FILE` no fim. `fseek` devolve a posição ao primeiro byte para o parser reler o fonte. `initLexer` repõe `currentLine` em 1; o `fseek` não mexe nesse contador, porque a linha é estado do `Lexer`, não do arquivo.

**5.** `char[10] nome` e um uso `nome[i + 1]` passam por funções diferentes. Quais, e o que cada uma exige dentro dos colchetes?

Olhe `dimension` e `dimension2`.

Resposta: a declaração passa por `type` e depois `dimension`, que exige `TOKEN_NUM_INT`. O uso passa por `readTerm`, `writeTerm` ou `term`, todos chamando `dimension2`, que exige `additiveExpr`. Índice com `+` é legal no uso; tamanho de declaração com expressão não é.

**6.** O lexer devolveu `TOKEN_ERROR` para um `@`. O que o parser faz com esse token?

Olhe o fim de `getNextToken` e pense em que produção aceita `TOKEN_ERROR`. Depois olhe `parse` e `match`.

Resposta: nenhuma produção espera `TOKEN_ERROR`. O parser não tem um teste especial para erro léxico. O token segue na sequência e falha no `match`, ou no teste de `EOF` no fim de `parse`, com uma mensagem de token inesperado. O lexer, sozinho, não aborta.

**7.** Acompanhe `resultado = a + b;` e diga qual função consome o `+`.

Olhe `assignmentStatement`, `complement`, `additiveExpr2` e `additiveOp`.

Resposta: `additiveExpr2` vê `TOKEN_SUM` e chama `additiveOp`, cujo `case TOKEN_SUM` faz o `match`. As camadas de `||`, `&&`, comparação e produto também são chamadas, mas as funções `2` delas retornam épsilon porque o lookahead é `+` e, depois, `;`.

### Nível difícil

**8.** Por que o `if (currentToken == TOKEN_MAIN)` de `functionList` nunca executa, e o que `int main() { }` produz?

Olhe a condição externa de `functionList`, o corpo desse `if` interno e `lookupKeyword` para `"main"`.

Resposta: o `if` interno está no ramo em que o token já é um tipo. `TOKEN_MAIN` não é nenhum desses tipos, então o ramo interno é código morto. A lista vazia ao encontrar `main` acontece porque a condição externa falha. Em `int main() { }`, `int` faz a lista entrar em `functionDecl`. `returnType` consome `int` e o `match(TOKEN_ID)` falha: o lexer classificou `main` como `TOKEN_MAIN`. O programa principal deste compilador é a palavra `main` sem tipo na frente.

**9.** `func soma(1, 2);` como comando, dentro de um bloco, não cai no `case TOKEN_FUNC` de `statement`. Por quê, e qual mensagem sai?

Olhe o `if` de `statementList`, o `switch` de `statement` e o `match` de `}` em `block`.

Resposta: `statementList` só chama `statement` para `scanf`, `println`, `if`, `while`, `return` e `id`. `TOKEN_FUNC` faz a lista retornar épsilon. `block` então executa `match(TOKEN_CLOSE_BRACE)` e reporta `}` esperado, encontrado `func`. O `case TOKEN_FUNC` só rodaria se alguma função chamasse `statement` com esse lookahead; no fluxo atual, ninguém chama. A chamada válida é o complemento de atribuição, e o `;` dessa forma é consumido por `assignmentStatement`, não por `functionCall`.

**10.** Se as funções de expressão passassem a montar árvore na mesma ordem em que consomem, como `a - b - c` ficaria agrupado?

Olhe `additiveExpr` e a chamada recursiva no fim de `additiveExpr2`.

Resposta: `additiveExpr` reconhece `a` e `additiveExpr2` consome `-`, reconhece `b` e chama `additiveExpr2` de novo para `- c`. A recursão fica à direita, então a árvore seria `a - (b - c)`. A gramática comentada tem a mesma forma. Hoje isso não muda a aceitação: as duas associações geram a mesma sequência de tokens, e o parser não avalia a expressão.

**11.** Um bloco contém `for (i = 0; i < 10; i = i + 1) { x = 1; }`. O lexer conhece `for`. Por que o erro tende a ser “esperado `}`”?

Olhe `TOKEN_FOR` em `token.h` e nas palavras reservadas, o FIRST de `statementList` e o que `block` faz quando a lista de comandos retorna.

Resposta: `for` vira `TOKEN_FOR` e deixa de ser identificador. `statementList` não inclui esse tipo, então trata o token como fim dos comandos. O próximo passo de `block` é exigir `}`. O `switch` de `statement` nem chega a ver o `for`. O erro aponta para a posição do `for`, com o lexema `for`, dizendo que esperava fechar o bloco.

**12.** O arquivo tem 101 identificadores diferentes, todos usados de forma sintaticamente válida. O que a lista de tokens mostra e o que a tabela mostra?

Olhe `MAX_SYMBOLS`, o `return -1` de `insertSymbol` e a chamada em `getNextToken`, que não testa o retorno.

Resposta: a lista de tokens tem os 101 `ID`, porque o estouro da tabela não muda o token devolvido. A tabela para em 100. O 101º nome não é inserido e não há mensagem. Na segunda passagem, `insertSymbol` continua devolvendo `-1` para esse nome; os outros 100 apenas reencontram o índice que já tinham.

**13.** `char nome[10];` e `char[10] nome;` chegam ao mesmo ponto do parser? Qual dos dois arquivos de teste é aceito pela gramática?

Olhe `varList`, `type`, `dimension` e `idList`, e compare [`testes/compiler/teste.txt`](testes/compiler/teste.txt) com [`testes/lexer/test2e.txt`](testes/lexer/test2e.txt).

Resposta: os dois começam em `varList` porque o lookahead é `char`. Em `char[10] nome`, `dimension` consome `[10]` e o `match` seguinte consome o `id`. Essa é a forma de `teste.txt`, e a gramática aceita. Em `char nome[10]`, `dimension` vê `nome`, não vê `[`, e retorna épsilon. O `id` consome `nome`. `idList` não vê vírgula. O `match` do `;` encontra `[` e falha. `test2e.txt` serve para observar o lexer; como frase desta gramática, ele para na declaração `char nome[10]`.
