# Analisador Léxico e Sintático

Compilador acadêmico em C (IFSC Lages) com duas etapas implementadas:

1. **Análise léxica** — lê o código-fonte caractere a caractere e gera tokens.
2. **Análise sintática** — verifica se a sequência de tokens obedece à gramática da linguagem.

O programa imprime a lista de tokens, a tabela de símbolos e o resultado da análise sintática.

---

## Estrutura do projeto

```text
Analisador-Lexico/
├── Makefile
├── include/
│   ├── token.h           # TokenType e struct Token
│   ├── lexer.h           # Interface do analisador léxico
│   ├── parser.h          # Interface do analisador sintático
│   └── symbol_table.h    # Tabela de símbolos
├── src/
│   ├── main.c            # Ponto de entrada: lexer + parser
│   ├── lexer/lexer.c
│   ├── parser/parser.c
│   └── symbol_table/symbol_table.c
└── testes/compiler/
    ├── teste.txt         # Programa válido
    └── teste_erro.txt    # Programa com erro sintático (falta ;)
```

O executável (`compiler.exe` no Windows, `compiler` no Linux/macOS) é gerado na raiz do projeto e não é versionado.

---

## Requisitos

- **GCC** no `PATH`
  - Windows: MSYS2 (UCRT64) com `mingw-w64-ucrt-x86_64-gcc`
  - Linux: `sudo apt install build-essential`
  - macOS: `xcode-select --install`
- **make** (opcional, recomendado)
  - Windows (MSYS2): `pacman -S mingw-w64-ucrt-x86_64-make` — o comando é `mingw32-make`
  - Linux/macOS: `make`

Verifique o compilador:

```powershell
gcc --version
```

---

## Compilação e execução

Na raiz do repositório:

### Com Makefile (recomendado)

```powershell
mingw32-make
mingw32-make run FILE=testes/compiler/teste.txt
mingw32-make run FILE=testes/compiler/teste_erro.txt
mingw32-make clean
```

No Linux/macOS, use `make` no lugar de `mingw32-make`.

Alvos:

| Alvo | Função |
|---|---|
| `all` (padrão) | Compila `compiler.exe` / `compiler` |
| `run` | Compila se necessário e executa com `FILE` |
| `clean` | Remove o executável |

### Sem Makefile

```powershell
gcc -Wall -Iinclude src/symbol_table/symbol_table.c src/lexer/lexer.c src/parser/parser.c src/main.c -o compiler.exe
.\compiler.exe testes/compiler/teste.txt
```

- `-Iinclude` localiza os cabeçalhos
- `-Wall` ativa avisos do compilador

Uso do executável:

```text
compiler.exe <ficheiro_fonte>
```

> No terminal do Windows, se os acentos saírem quebrados, execute `chcp 65001` antes do programa.

---

## Como o programa funciona

`main.c` executa as etapas nesta ordem:

1. Abre o arquivo-fonte passado na linha de comando.
2. Inicializa a **tabela de símbolos** (`initSymbolTable`).
3. Inicializa o **lexer** (`initLexer`) e consome todos os tokens com `getNextToken`, imprimindo cada um.
4. Imprime a **tabela de símbolos**.
5. Volta o arquivo ao início (`fseek`) e reinicia o lexer.
6. Inicializa o **parser** (`initParser`) e chama `parse`.

Se a gramática for respeitada até `TOKEN_EOF`, a mensagem de sucesso é impressa. Qualquer token inesperado encerra o programa com erro sintático (linha, lexema e tipo esperado).

---

## Funções principais

### Lexer (`src/lexer/lexer.c`)

| Função | Papel |
|---|---|
| `initLexer(lexer, file)` | Associa o arquivo ao lexer e zera o contador de linha. |
| `getNextToken(lexer)` | Devolve o próximo token (descarta espaços e comentários `//`). |
| `tokenTypeToString(type)` | Nome do token para impressão. |

Reconhecimento interno:

- Identificadores e palavras reservadas: `L(L|D)*`
- Inteiros `D+` e decimais `D+\.D+`
- Texto entre aspas `"..."` (não atravessa linha)
- Operadores, comparações e símbolos
- Palavra `func` como `TOKEN_FUNC` (usada em chamadas de função)

Identificadores (`TOKEN_ID`) são inseridos na tabela de símbolos. Palavras reservadas não entram.

### Tabela de símbolos (`src/symbol_table/symbol_table.c`)

| Função | Papel |
|---|---|
| `initSymbolTable()` | Esvazia a tabela. |
| `insertSymbol(name, type)` | Insere um identificador se ainda não existir (máximo 100 entradas). |
| `printSymbolTable()` | Lista nome e tipo de cada símbolo. |

A tabela registra **todos os IDs encontrados** pelo lexer. Checagem de declaração e de tipos fica para a análise semântica (ainda não implementada).

### Parser (`src/parser/parser.c`)

Analisador **descendente recursivo** (um procedimento por não-terminal).

| Função | Papel |
|---|---|
| `initParser(parser, lexer)` | Liga o parser ao lexer. |
| `parse(parser)` | Pede o primeiro token e analisa `PROGRAMA`. |
| `advance` | Consome o próximo token (`getNextToken`). |
| `match(tipo)` | Aceita o token esperado ou aborta com erro. |

Cada produção da gramática tem uma função correspondente (`program`, `functionDecl`, `ifStatement`, `expression`, etc.).

---

## Tokens reconhecidos

Cada token corresponde a uma linha da tabela de expressões regulares da Aula 4. O parser usa o `TokenType`, não só a categoria.

| Expressão regular | TokenType | Categoria |
|---|---|---|
| `D+` | `TOKEN_NUM_INT` | Número inteiro |
| `D+\.D+` | `TOKEN_NUM_DEC` | Número decimal |
| `L(L\|D)*` | `TOKEN_ID` | Identificador |
| `".*"` | `TOKEN_TEXT` | Constante de texto |
| `int float char boolean void if else for while scanf println main return func` | `TOKEN_INT` … `TOKEN_FUNC` | Palavra reservada |
| `//.*\n` | (descartado) | Comentário |
| `=` | `TOKEN_ATRIB` | Atribuição |
| `+ - * / %` | `TOKEN_SUM`, `TOKEN_SUB`, `TOKEN_MULT`, `TOKEN_DIV`, `TOKEN_MOD` | Aritmético |
| `&& \|\| !` | `TOKEN_AND`, `TOKEN_OR`, `TOKEN_NOT` | Lógico |
| `> >= < <= != ==` | `TOKEN_COMP` | Comparação |
| `( ) [ ] { } , ;` | `TOKEN_OPEN_PAREN` … `TOKEN_SEMICOLON` | Símbolo |

Onde `D = [0-9]` e `L = [a-zA-Z_]`.

Observações:

- Comparações chegam todas como `TOKEN_COMP`; o operador concreto está no `lexeme`.
- Comentários e espaços não chegam ao parser.
- `for` é palavra reservada no lexer, mas **não há produção** para `for` na gramática atual.
- Chamadas de função usam a palavra `func` (`TOKEN_FUNC`).
- Todo token carrega `type`, `lexeme` e `line`.

### Erros léxicos

O lexer devolve `TOKEN_ERROR` e continua nos casos:

- caractere inválido (`@`, `#`, …)
- `&` ou `|` isolados (só `&&` e `||` existem)
- decimal incompleto (`3.`)
- texto sem aspas de fechamento

---

## Gramática

A linguagem é a da disciplina de Compiladores: um programa é uma lista (possivelmente vazia) de funções auxiliares seguida obrigatoriamente de `main`.

O parser é LL(1) por descida recursiva. `| ε` indica produção vazia.

```text
PROGRAMA            → LISTAFUNÇÕES PRINCIPAL

LISTAFUNÇÕES        → DECFUNÇÃO LISTAFUNÇÕES | ε

DECFUNÇÃO           → TIPORETORNO id ( PARÂMETROS ) BLOCO

TIPORETORNO         → TIPO | void

TIPO                → TIPOBASE DIMENSÃO

TIPOBASE            → char | float | int | boolean

DIMENSÃO            → [ num_int ] DIMENSÃO | ε

PARÂMETROS          → TIPO id LISTAPARAMETROS | ε

LISTAPARAMETROS     → , TIPO id LISTAPARAMETROS | ε

PRINCIPAL           → main ( ) BLOCO

BLOCO               → { LISTAVARIÁVEIS COMANDOS }

LISTAVARIÁVEIS      → TIPO id LISTAID ; LISTAVARIÁVEIS | ε

LISTAID             → , id LISTAID | ε

COMANDOS            → COMANDO COMANDOS | ε

COMANDO             → LEITURA | ESCRITA | ATRIBUIÇÃO | FUNÇÃO | SELEÇÃO | ENQUANTO | RETORNO

LEITURA             → scanf ( TERMOLEITURA NOVOTERMOLEITURA ) ;

TERMOLEITURA        → id DIMENSAO2

NOVOTERMOLEITURA    → , TERMOLEITURA NOVOTERMOLEITURA | ε

DIMENSAO2           → [ EXPR_ADITIVA ] DIMENSAO2 | ε

ESCRITA             → println ( TERMOESCRITA NOVOTERMOESCRITA ) ;

TERMOESCRITA        → id DIMENSAO2 | CONSTANTE | texto

NOVOTERMOESCRITA    → , TERMOESCRITA NOVOTERMOESCRITA | ε

SELEÇÃO             → if ( EXPRESSÃO ) BLOCO SENÃO

SENÃO               → else BLOCO | ε

ENQUANTO            → while ( EXPRESSÃO ) BLOCO

ATRIBUIÇÃO          → id = COMPLEMENTO ;

COMPLEMENTO         → EXPRESSÃO | FUNÇÃO

FUNÇÃO              → func id ( ARGUMENTOS )

ARGUMENTOS          → EXPRESSÃO NOVO_ARGUMENTO | ε

NOVO_ARGUMENTO      → , EXPRESSÃO NOVO_ARGUMENTO | ε

RETORNO             → return EXPRESSÃO ;

EXPRESSÃO           → EXPR_OU

EXPR_OU             → EXPR_E EXPR_OU2
EXPR_OU2            → || EXPR_E EXPR_OU2 | ε

EXPR_E              → EXPR_RELACIONAL EXPR_E2
EXPR_E2             → && EXPR_RELACIONAL EXPR_E2 | ε

EXPR_RELACIONAL     → EXPR_ADITIVA EXPR_RELACIONAL2
EXPR_RELACIONAL2    → comp EXPR_ADITIVA | ε

EXPR_ADITIVA        → EXPR_MULTIPLICATIVA EXPR_ADITIVA2
EXPR_ADITIVA2       → OP_ADITIVO EXPR_MULTIPLICATIVA EXPR_ADITIVA2 | ε
OP_ADITIVO          → + | -

EXPR_MULTIPLICATIVA → FATOR EXPR_MULTIPLICATIVA2
EXPR_MULTIPLICATIVA2→ OP_MULTIPLICATIVO FATOR EXPR_MULTIPLICATIVA2 | ε
OP_MULTIPLICATIVO   → * | / | %

FATOR               → SINAL TERMO | texto | ! FATOR | ( EXPRESSÃO )

TERMO               → id DIMENSAO2 | CONSTANTE

SINAL               → + | - | ε

CONSTANTE           → num_int | num_dec
```

`comp` é qualquer operador de comparação: `>`, `>=`, `<`, `<=`, `!=`, `==`.

### Precedência das expressões

Da mais baixa para a mais alta:

1. `||`
2. `&&`
3. comparação (`comp`)
4. `+` `-`
5. `*` `/` `%`
6. unários `+` `-` `!` e agrupamento `( )`

### Regras práticas da linguagem

- O arquivo deve terminar em `main() { ... }`. Funções auxiliares, se existirem, vêm **antes** de `main`.
- Declarações de variáveis ficam no **início do bloco**, antes dos comandos.
- Tipos de vetor usam dimensão estática: `char[10] nome;`
- `scanf` só aceita identificadores (com índices opcionais).
- `println` aceita identificadores, constantes numéricas ou texto.
- Chamada de função **sempre** começa com `func`: `idade = func soma(idade, pi);`
- `if` e `while` exigem condição entre parênteses e um `BLOCO` com chaves. `else` é opcional.
- Não há `for` na gramática, embora `for` seja token reservado.

---

## Exemplo de saída

Entrada:

```c
int idade = 20;
```

Essa linha **não** é um programa completo (falta `main` e a declaração no formato `TIPO id ;` seguida de atribuição). Um recorte válido dentro de `main`:

```c
main() {
    int idade;
    idade = 20;
    return 0;
}
```

Tokens (trecho):

```text
[Linha 01] MAIN               : main
[Linha 01] (                  : (
[Linha 01] )                  : )
[Linha 01] {                  : {
[Linha 02] INT                : int
[Linha 02] ID                 : idade
[Linha 02] ;                  : ;
[Linha 03] ID                 : idade
[Linha 03] =                  : =
[Linha 03] NUM_INT            : 20
[Linha 03] ;                  : ;
[Linha 04] RETURN             : return
[Linha 04] NUM_INT            : 0
[Linha 04] ;                  : ;
[Linha 05] }                  : }
[Linha 05] FIM_ARQUIVO        : EOF
```

Se a análise sintática terminar em `TOKEN_EOF`:

```text
=========================================
 SUCCESSO: Análise Sintática Completa!
=========================================
```

O arquivo `testes/compiler/teste_erro.txt` omite um `;` após `ok = 0`. O parser reporta o erro na linha correspondente e encerra.

---

## Fluxo das etapas

```text
Código-fonte
     │
     ▼
  Lexer          getNextToken()
     │
     ▼
  Tokens  +  Tabela de Símbolos
     │
     ▼
  Parser         descida recursiva
     │
     ▼
  Aceita / Erro sintático
```

A análise semântica ainda não está implementada. A tabela de símbolos já existe para uso futuro (declarações, tipos, etc.).
