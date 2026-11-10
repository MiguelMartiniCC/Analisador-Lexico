# Explicação do analisador semântico

O analisador léxico quebra o texto em tokens e o sintático confere se os tokens formam uma frase da gramática. Nenhum dos dois sabe o que as palavras significam. `x = y + 1;` é sintaticamente correto mesmo que `y` nunca tenha sido declarado, e `int x; x = 2.5;` também. O analisador semântico cobre essa parte: confere se os nomes foram declarados, se os tipos combinam e se as chamadas de função e os retornos fazem sentido.

Este documento continua o [`EXPLICACAO.md`](EXPLICACAO.md), que descreve o léxico e o sintático. Termos de C já explicados lá (`struct`, `enum`, ponteiro, `static`) não são repetidos. Os que aparecem pela primeira vez são explicados onde são usados.

A geração de código de três endereços não faz parte desta etapa. O analisador semântico só deixa pronto o que ela vai precisar: o tipo de cada expressão e a tabela de declarações.

## Visão geral

A análise semântica roda **junto com o parser, na mesma passada**. Cada função do parser (`assignmentStatement`, `term`, `additiveExpr2`, …) continua reconhecendo o mesmo trecho da gramática, mas agora também chama funções do módulo `semantic` para fazer as verificações daquele trecho. Esse estilo se chama tradução dirigida pela sintaxe: a estrutura do parser guia onde cada regra semântica é aplicada.

```text
Código-fonte
     |
     v
1a passagem: Lexer imprime os tokens e a Tabela de Símbolos (sem mudanças)
     |
     v
2a passagem: Parser
     |   reconhece a gramática
     |   e chama o analisador semântico em cada declaração, expressão e comando
     v
Sucesso sintático -> Tabela semântica -> Sucesso semântico
```

Não existe árvore sintática. Em vez disso, as funções que reconhecem expressões **devolvem o tipo** do que reconheceram. Quem chama usa esse tipo para conferir a regra do operador, da atribuição ou do argumento. Foi a escolha mais simples para manter o formato "um não-terminal, uma função" das etapas anteriores.

O programa para no primeiro erro, como já acontecia no parser:

```text
Erro Semântico [Linha 7]: Atribuição incompatível: 'x' é 'int', mas o valor é 'float'.
```

Como as duas análises acontecem na mesma passada, um erro semântico no começo do arquivo aparece antes de um erro sintático que esteja mais abaixo. Em `main.c` o cabeçalho da segunda passada é `=== ANÁLISE SINTÁTICA E SEMÂNTICA ===`. Se o programa passar, o parser imprime o sucesso sintático e, logo em seguida, `finishSemanticAnalysis` imprime a tabela semântica e o sucesso semântico.

## Arquivos novos e alterados

| Arquivo | O que faz |
|---|---|
| [`include/semantic.h`](include/semantic.h) | Tipos (`BaseType`, `SemType`) e as funções que o parser pode chamar |
| [`src/semantic/semantic.c`](src/semantic/semantic.c) | Tabela de declarações, escopo, regras de tipo e mensagens de erro |
| [`src/parser/parser.c`](src/parser/parser.c) | Passa a chamar o módulo semântico; as funções de expressão devolvem tipos |
| [`src/main.c`](src/main.c) | Chama `initSemantic` no início e `finishSemanticAnalysis` no fim |
| [`Makefile`](Makefile) | Inclui `src/semantic/semantic.c` na compilação |

A Tabela de Símbolos da etapa 1 (`symbol_table.c`) **não foi alterada**. Ela guarda só os nomes que o lexer viu, não sabe o que é declaração e mistura todas as funções num único conjunto de nomes. A análise semântica precisa de tipo e escopo, então tem uma tabela própria.

## Tipos: `BaseType` e `SemType`

```c
typedef enum {
    TYPE_INT, TYPE_FLOAT, TYPE_CHAR, TYPE_BOOLEAN, TYPE_VOID, TYPE_TEXT
} BaseType;

typedef struct {
    BaseType base;
    int dimensions;
} SemType;
```

`SemType` é o tipo completo de um valor: o tipo base e o número de dimensões. `int idade` é `{ TYPE_INT, 0 }`. `char[10] nome` é `{ TYPE_CHAR, 1 }`. `int[2][2] matriz` é `{ TYPE_INT, 2 }`. O tamanho (`10`, `2`) não é guardado.

`TYPE_TEXT` é o tipo de uma constante entre aspas, como `"Idade:"`. Ele existe só nas expressões; nenhuma variável é declarada com esse tipo. `TYPE_VOID` aparece só como tipo de retorno de função.

Um `SemType` é passado e devolvido **por valor**: a função recebe uma cópia da struct. Isso evita ponteiros nas regras de tipo. `makeType(base, dimensions)` monta uma struct e `typeToString` a transforma em texto para as mensagens (`int`, `char[]`, `texto`).

## A tabela de declarações

```c
typedef struct {
    char name[256];
    DeclKind kind;               // variável, parâmetro ou função
    SemType type;                // tipo da variável, ou tipo de retorno da função
    int owner;                   // índice da função dona; -1 para as próprias funções
    int paramCount;              // só para funções
    SemType params[MAX_PARAMS];  // só para funções: tipo de cada parâmetro
} Declaration;
```

A tabela é um vetor global (`static`, visível só em `semantic.c`) com no máximo 200 linhas, mais um contador. Para `teste.txt` ela fica assim (é a saída de `finishSemanticAnalysis`):

```text
[0] Nome: soma            | Categoria: funcao    | Tipo: float      | Escopo: global
[1] Nome: a               | Categoria: parametro | Tipo: int        | Escopo: soma
[2] Nome: b               | Categoria: parametro | Tipo: float      | Escopo: soma
[3] Nome: resultado       | Categoria: variavel  | Tipo: float      | Escopo: soma
[4] Nome: main            | Categoria: funcao    | Tipo: int        | Escopo: global
[5] Nome: idade           | Categoria: variavel  | Tipo: int        | Escopo: main
[6] Nome: i               | Categoria: variavel  | Tipo: int        | Escopo: main
[7] Nome: x               | Categoria: variavel  | Tipo: int        | Escopo: main
[8] Nome: pi              | Categoria: variavel  | Tipo: float      | Escopo: main
[9] Nome: ok              | Categoria: variavel  | Tipo: boolean    | Escopo: main
[10] Nome: nome            | Categoria: variavel  | Tipo: char[]     | Escopo: main
```

### Escopo

Há um escopo global, onde ficam os nomes das funções, e um escopo local por função, com os parâmetros e as variáveis dela. O escopo local é identificado por `owner`: o índice da função na própria tabela. `currentFunction` guarda o índice da função que está sendo analisada.

- `beginFunction` registra a função e a torna a função atual. O parser a chama depois de ler o nome (`functionDecl`) ou a palavra `main` (`mainFunction`).
- `endFunction` volta ao escopo global quando o bloco da função termina.
- `findLocal` procura uma variável ou parâmetro olhando só as linhas cujo `owner` é a função atual. Por isso `resultado` pode existir em `soma` e em `main` sem conflito.
- `findFunction` procura entre as funções, que são sempre globais.

A função entra na tabela **antes** de o corpo ser lido. Por isso ela pode chamar a si mesma (recursão). Em compensação, uma função só pode ser chamada depois de declarada: não há como declarar o protótipo de uma função adiante.

Os parâmetros são registrados duas vezes: como linhas da tabela (para serem encontrados dentro do corpo) e na lista `params` da função (para conferir as chamadas depois).

Um nome de variável pode ser igual ao nome de uma função, porque a chamada sempre usa `func` (`func f()`) e nunca se confunde com uma variável.

## Como o parser devolve os tipos

Antes, as funções de expressão eram `void`. Agora devolvem `SemType`. A escada de precedência continua igual; o que muda é que cada degrau combina os tipos dos dois lados.

As funções com sufixo `2` (`additiveExpr2`, `orExpr2`, …) ganharam um parâmetro `left`: o tipo do que já foi lido à esquerda do operador. Elas conferem o operador, calculam o tipo do resultado e o passam adiante.

```c
static SemType additiveExpr2(Parser *parser, SemType left)
{
    if (parser->currentToken.type == TOKEN_SUM ||
        parser->currentToken.type == TOKEN_SUB)
    {
        Token op = parser->currentToken;      // guarda o operador antes de consumi-lo
        additiveOp(parser);
        SemType right = multiplicativeExpr(parser);
        SemType result = arithmeticType(left, right, op.lexeme, op.line);
        return additiveExpr2(parser, result);   // continua com o tipo acumulado
    }
    return left;                                // ε: o tipo é o que já tínhamos
}
```

`Token op = parser->currentToken;` copia o token inteiro. É preciso copiar antes de `additiveOp`, porque ela chama `advance` e o `currentToken` passa a ser o próximo.

Exemplo, `resultado = a + b;` dentro de `soma` (onde `a` é `int` e `b` é `float`):

1. `assignmentStatement` consome `resultado` e pede o tipo dele a `requireVariable`: `float`.
2. `complement` chama `expression`. A escada desce até `term`, que consome `a` e devolve `int`.
3. `additiveExpr2` vê `+`, lê `b` (`float`) e chama `arithmeticType(int, float)`, que devolve `float`.
4. De volta a `assignmentStatement`, `checkAssignment` compara o destino (`float`) com o valor (`float`). Compatíveis, segue para o `;`.

Exemplo, `if (idade >= 18 && !ok || idade != 0)`: `idade >= 18` devolve `boolean` (`comparisonType`), `!ok` devolve `boolean` (`notType` aceita `boolean` ou `int`), o `&&` e o `||` combinam `boolean` com `boolean` e `requireCondition` confere que o resultado final pode ser condição.

### Funções do parser que mudaram de assinatura

| Função | Antes | Agora |
|---|---|---|
| `returnType`, `type` | `void` | devolvem `SemType` |
| `baseType` | `void` | devolve `BaseType` |
| `dimension` | `void` | devolve quantas dimensões foram declaradas |
| `dimension2` | `void` | devolve quantos índices foram usados (e confere que cada um é `int`) |
| `idList` | `(parser)` | `(parser, varType)`: todas as variáveis da lista têm o mesmo tipo |
| `expression`, `orExpr`, …, `factor`, `term`, `constant` | `void` | devolvem `SemType` |
| `orExpr2`, `andExpr2`, `relationalExpr2`, `additiveExpr2`, `multiplicativeExpr2` | `(parser)` | `(parser, left)` |
| `sign` | `void` | devolve 1 se havia sinal, 0 se não |
| `complement`, `functionCall` | `void` | devolvem o tipo do valor / o tipo de retorno da função |
| `arguments`, `newArgument` | `void` | devolvem quantos argumentos a chamada tem |

A gramática e os `match` não mudaram.

## Regras verificadas

### Declarações

| Regra | Onde | Mensagem |
|---|---|---|
| Nome de variável, parâmetro ou função declarado duas vezes | `declareVariable`, `declareParameter`, `beginFunction` | `'x' já foi declarado na função 'main'.` |
| Uso de variável não declarada | `requireVariable` (atribuição, `scanf`, `println`, expressões) | `A variável 'y' não foi declarada.` |
| Chamada de função não declarada | `requireFunction` | `A função 'dobro' não foi declarada (...)` |

Um parâmetro e uma variável local com o mesmo nome também contam como declaração repetida, porque os dois vivem no mesmo escopo.

### Tipos

`isAssignable(target, value)` decide se um valor pode ser guardado num destino. É usada nas atribuições, nos retornos e nos argumentos.

| Destino | Valor aceito |
|---|---|
| `int` | `int` |
| `float` | `float` e `int` (conversão automática) |
| `boolean` | `boolean` e `int` (a linguagem não tem `true`/`false`, então `ok = 0` precisa funcionar) |
| `char` | `char` |
| vetor | vetor com o mesmo tipo base e o mesmo número de dimensões |

`float` em `int` é erro (perderia a parte decimal). Texto e `void` nunca podem ser atribuídos a variáveis.

| Operador | Operandos aceitos | Resultado |
|---|---|---|
| `+ - * /` | `int` ou `float` | `float` se algum for `float`, senão `int` |
| `%` | só `int` | `int` |
| `< <= > >= == !=` | `int` ou `float` | `boolean` |
| `&& \|\|` | `boolean` ou `int` | `boolean` |
| `!` | `boolean` ou `int` | `boolean` |
| sinal `+` / `-` na frente de um termo | `int` ou `float` | o mesmo tipo |

Todas essas conferências exigem **zero dimensões**: um vetor inteiro não entra em conta, soma ou comparação.

### Condições, retorno e índices

- **Condição de `if` e `while`**: `requireCondition` aceita `boolean` ou `int`. `float`, texto e vetor são erro.
- **Retorno**: `checkReturn` confere com a função atual. Função `void` não pode ter `return valor;`. Nas demais, o valor precisa ser atribuível ao tipo de retorno. `main` é registrada com retorno `int`, então `return 0;` é válido.
- **Atribuição de função `void`**: `x = func mostrar(...)` é erro quando `mostrar` é `void`.
- **Índices**: o índice precisa ser `int` (`requireIntIndex`). `indexedType` recebe o tipo da variável e a quantidade de índices usados: variável sem dimensões não aceita índice, e mais índices que dimensões é erro. Com menos índices que dimensões o resultado é um vetor menor (`matriz[0]`, de `int[][]`, tem tipo `int[]`).

### Chamadas de função

`functionCall` pede o tipo de retorno a `requireFunction` e, durante a leitura dos argumentos, chama `checkArgument` para cada um. Ao fechar o parêntese, `checkArgumentCount` confere a quantidade. Cada argumento precisa ser atribuível ao parâmetro correspondente, com uma exceção: uma constante entre aspas é aceita por parâmetro `char`.

### Comandos sem restrição de tipo

`println` aceita qualquer tipo (inclusive um vetor inteiro, como `println("Nome:", nome)`), mas cada identificador usado precisa estar declarado e os índices precisam ser válidos. `scanf` segue a mesma regra.

## Erros

`semanticError(line, formato, ...)` imprime `Erro Semântico [Linha N]: ` seguido da mensagem e chama `exit(EXIT_FAILURE)`. É o mesmo comportamento do parser: sem recuperação de erro, o primeiro problema encerra o programa. Os `...` na assinatura significam que a função aceita um número variável de argumentos, como o `printf`. Ela repassa o formato e os argumentos para `vprintf`.

Como `semanticError` encerra o programa, as funções que a chamam não precisam de um `else` nem de um valor de retorno especial para "tipo inválido".

A linha reportada é a do token que originou a regra: o identificador numa atribuição ou declaração, o operador numa expressão, a palavra `return` num retorno, o início da condição num `if`/`while`.

## Arquivos de teste

| Arquivo | O que mostra |
|---|---|
| [`testes/compiler/teste.txt`](testes/compiler/teste.txt) | Programa do enunciado; passa nas três fases |
| [`testes/compiler/teste_erro.txt`](testes/compiler/teste_erro.txt) | Falta um `;`: continua sendo erro sintático |
| [`testes/semantico/valido.txt`](testes/semantico/valido.txt) | Recursão, vetores, conversão `int` → `float`, `int` como `boolean`, texto como argumento `char` |
| `testes/semantico/erro_*.txt` | Um erro semântico por arquivo |

A primeira linha de cada `erro_*.txt` é um comentário com a mensagem esperada. Para rodar um deles:

```powershell
mingw32-make run FILE=testes/semantico/erro_float_em_int.txt
```

| Arquivo `erro_*` | Erro |
|---|---|
| `variavel_nao_declarada`, `funcao_nao_declarada`, `scanf_nao_declarado` | Nome usado sem declaração |
| `variavel_repetida`, `parametro_repetido`, `variavel_igual_parametro`, `funcao_repetida` | Declaração repetida |
| `float_em_int`, `texto_em_atribuicao` | Atribuição incompatível |
| `texto_em_soma`, `modulo_float` | Operador com tipo inválido |
| `condicao_float`, `condicao_if_texto` | Condição que não é `boolean`/`int` |
| `argumentos_quantidade`, `argumentos_excesso`, `argumentos_tipo` | Chamada com argumentos errados |
| `return_em_void`, `return_tipo` | Retorno inválido |
| `atribui_void` | Resultado de função `void` atribuído a variável |
| `indice_em_nao_vetor`, `indice_demais`, `indice_nao_int` | Índice inválido |

Os dois testes de `testes/compiler` mudaram em relação à etapa anterior: `soma` agora é `float soma(...)`, e a chamada é `pi = func soma(idade, pi);`. No original, `soma` era `void` mas retornava um valor, e o resultado era atribuído a um `int`; as duas coisas são erros semânticos.

## Limitações conhecidas

- **Um escopo por função.** Variáveis declaradas dentro de um `if` ou `while` entram no escopo da função inteira. Declarar o mesmo nome em dois blocos da mesma função é erro de declaração repetida.
- **Função só pode ser chamada depois de declarada.** A linguagem não tem protótipos, então duas funções que se chamam mutuamente não são possíveis.
- **Não confere `return` ausente.** Uma função `int` sem nenhum `return` passa na análise.
- **Tamanho de vetor não é comparado.** `int[3]` e `int[5]` são o mesmo tipo, e não há verificação de limites do índice.
- **Elementos de vetor não podem ser atribuídos.** A gramática define a atribuição como `id = COMPLEMENTO`, sem índice à esquerda. Na prática, os vetores só recebem valor por `scanf`.
- **Chamada de função só em atribuição.** `statementList` não aceita `func` como comando solto (limitação já documentada no `EXPLICACAO.md`).
- **`==` e `!=` só comparam números.** `boolean == boolean` e comparação de `char` são erro.
- **Sem avisos.** Variável declarada e nunca usada (como `i` em `teste.txt`) não gera mensagem.
- **Limites fixos.** No máximo 200 declarações no programa e 10 parâmetros por função; ao estourar, o analisador reporta erro.

## O que a próxima etapa aproveita

Como cada expressão já sai do parser com o tipo calculado, o gerador de código de três endereços poderá usar o mesmo ponto de chamada para decidir, por exemplo, onde inserir uma conversão de `int` para `float`. A tabela semântica tem o tipo e o escopo de cada nome, que é o que o gerador precisa para declarar variáveis e temporários.
