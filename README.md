# Compilador

Repositório acadêmico em C com as etapas do compilador organizadas em pastas irmãs.

Hoje o **Analisador Léxico** está implementado. A pasta **Analisador_Sintatico** já existe para receber o parser.

O lexer lê um arquivo de código-fonte, processa o conteúdo caractere por caractere e gera a **lista de tokens reconhecidos** e a **Tabela de Símbolos**. As regras léxicas seguem a tabela de expressões regulares da Aula 4 (Compiladores, IFSC Lages).

---

## Estrutura de pastas

```text
Compilador/
├── Analisador_Lexico/          # Análise léxica (Lexer)
│   ├── Makefile                # Compila e executa o lexer
│   ├── include/                # Cabeçalhos (.h)
│   │   ├── token.h             # TokenType e struct Token
│   │   ├── symbol_table.h      # Interface da Tabela de Símbolos
│   │   └── lexer.h             # Interface do analisador léxico
│   │
│   ├── src/                    # Implementações (.c)
│   │   ├── lexer/
│   │   │   └── lexer.c         # Implementação do analisador léxico
│   │   ├── symbol_table/
│   │   │   └── symbol_table.c  # Implementação da Tabela de Símbolos
│   │   └── main.c              # Ponto de entrada do analisador
│   │
│   └── testes/                 # Arquivos utilizados nos testes
│       └── lexer/
│           └── teste.txt       # Entrada de teste cobrindo todos os tokens
│
└── Analisador_Sintatico/       # Análise sintática (Parser) — em construção
```

O executável (`lexer.exe` no Windows, `lexer` no Linux/macOS) é gerado dentro de `Analisador_Lexico/` e não faz parte do código-fonte versionado.

---

## Requisitos

- **GCC** no `PATH`
  - Windows: MSYS2 (UCRT64) com `mingw-w64-ucrt-x86_64-gcc`
  - Linux: `sudo apt install build-essential`
  - macOS: `xcode-select --install`
- **make** (opcional, mas recomendado)
  - Windows (MSYS2): `pacman -S mingw-w64-ucrt-x86_64-make`; o comando se chama `mingw32-make`
  - Linux/macOS: `make`
- **VS Code** com a extensão C/C++ da Microsoft (opcional)

Verifique o compilador:

```powershell
gcc --version
```

---

## Executando o analisador léxico

Entre na pasta do lexer:

```powershell
cd Analisador_Lexico
```

### Com Makefile (recomendado)

```powershell
mingw32-make                                      # compila
mingw32-make run                                  # compila (se preciso) e roda com testes/lexer/teste.txt
mingw32-make run FILE=testes/lexer/outro.txt      # roda com outro arquivo
mingw32-make clean                                # remove o executável
```

No Linux/macOS, use `make` no lugar de `mingw32-make`.

### Sem Makefile

```powershell
gcc -Wall -Iinclude src/symbol_table/symbol_table.c src/lexer/lexer.c src/main.c -o lexer.exe
.\lexer.exe testes/lexer/teste.txt
```

- `-Iinclude` aponta para os cabeçalhos
- `-Wall` ativa os avisos do compilador

> No terminal do Windows, se os acentos da saída aparecerem quebrados, rode `chcp 65001` antes do programa.

---

## Tokens reconhecidos

Cada token do lexer corresponde a uma linha da tabela da Aula 4. O parser usa o `TokenType` (um por token), e não só a categoria.

| Expressão regular | TokenType | Categoria |
|---|---|---|
| `D+` | `TOKEN_NUM_INT` | Número inteiro |
| `D+\.D+` | `TOKEN_NUM_DEC` | Número decimal |
| `L(L\|D)*` | `TOKEN_ID` | Identificador |
| `".*"` | `TOKEN_TEXT` | Constante de texto |
| `int float char boolean void if else for while scanf println main return` | `TOKEN_INT`, `TOKEN_FLOAT`, ..., `TOKEN_RETURN` | Palavra reservada |
| `//.*\n` | (descartado) | Comentário |
| `=` | `TOKEN_ATRIB` | Operador de atribuição |
| `+ - * / %` | `TOKEN_SUM`, `TOKEN_SUB`, `TOKEN_MULT`, `TOKEN_DIV`, `TOKEN_MOD` | Operador aritmético |
| `&& \|\| !` | `TOKEN_AND`, `TOKEN_OR`, `TOKEN_NOT` | Operador lógico |
| `> >= < <= != ==` | `TOKEN_COMP` | Operador de comparação |
| `( ) [ ] { } , ;` | `TOKEN_OPEN_PAREN`, `TOKEN_CLOSE_PAREN`, `TOKEN_OPEN_BRACKET`, `TOKEN_CLOSE_BRACKET`, `TOKEN_OPEN_BRACE`, `TOKEN_CLOSE_BRACE`, `TOKEN_COMMA`, `TOKEN_SEMICOLON` | Símbolo especial |

Onde `D = [0-9]` e `L = [a-zA-Z_]`.

Observações:

- Todos os operadores de comparação chegam ao parser como `TOKEN_COMP`; para saber qual é, consulte o `lexeme`.
- Comentários e espaços são descartados pelo lexer e nunca chegam ao parser.
- Só os `ID` entram na Tabela de Símbolos; palavras reservadas não.
- Todo token carrega `type`, `lexeme` e `line`.

### Erros léxicos

O lexer devolve `TOKEN_ERROR` e continua a análise nos casos:

- caractere inválido (`@`, `#`, ...)
- `&` ou `|` sozinhos (só `&&` e `||` existem)
- número decimal incompleto (`3.`)
- texto sem aspas de fechamento

O `main.c` mostra cada erro com a linha e o total de erros ao final.

---

## Exemplo de saída

Entrada:

```c
int idade = 20;
```

Saída:

```text
[Linha 01] INT                : int
[Linha 01] ID                 : idade
[Linha 01] =                  : =
[Linha 01] NUM_INT            : 20
[Linha 01] ;                  : ;
[Linha 01] FIM_ARQUIVO        : EOF
```

---

## Integração com o analisador sintático

O parser deve ficar em `Analisador_Sintatico/` e consumir o lexer **sob demanda**: pede o próximo token com `getNextToken()` em vez de carregar a lista inteira de antemão.

```text
Código-fonte
     │
     ▼
   Lexer          (Analisador_Lexico)
     │
     │ getNextToken()
     ▼
   Token
     │
     ▼
   Parser         (Analisador_Sintatico)
```

### Inclusão dos cabeçalhos

No código do parser:

```c
#include "lexer.h"
#include "symbol_table.h"
```

Ao compilar o parser, informe a pasta de includes do lexer. A partir da raiz do repositório:

```powershell
gcc -Wall -IAnalisador_Lexico/include ...
```

Se a compilação partir de `Analisador_Sintatico/`, use `-I../Analisador_Lexico/include`.

### Consumo de tokens

```c
static Token currentToken;
static Lexer *globalLexer;

static void advance(void) {
    currentToken = getNextToken(globalLexer);
}

void parse(Lexer *lexer) {
    globalLexer = lexer;
    advance();

    while (currentToken.type != TOKEN_EOF) {
        /* análise sintática com base em currentToken.type */
        advance();
    }
}
```

`currentToken` é o lookahead: o token que o parser está examinando agora. `advance()` solicita o próximo token ao lexer.

Exemplo:

```text
Fonte:
    int idade = 20;

Tokens:
    TOKEN_INT
    TOKEN_ID
    TOKEN_ATRIB
    TOKEN_NUM_INT
    TOKEN_SEMICOLON
    TOKEN_EOF
```

### Consulta à Tabela de Símbolos

A tabela é preenchida pelo lexer com **todos os identificadores encontrados**, tenham sido declarados ou não. Verificar declaração e tipos é papel da análise semântica, que pode consultar a tabela com `getSymbol()`:

```c
Symbol *sym = getSymbol(currentToken.lexeme);

if (sym != NULL) {
    printf("Identificador '%s' encontrado. Tipo: %d\n", sym->name, sym->type);
}
```

---

## Fluxo do compilador

```text
                 Código-fonte
                      │
                      ▼
             ┌─────────────────┐
             │      Lexer      │  Analisador_Lexico
             │ Análise Léxica  │
             └────────┬────────┘
                      │ Tokens
                      ▼
             ┌─────────────────┐
             │      Parser     │  Analisador_Sintatico
             │ Análise Sintática│
             └────────┬────────┘
                      │
                      ▼
             ┌─────────────────┐
             │   Analisador    │
             │    Semântico    │
             └────────┬────────┘
                      │
                      ▼
              Tabela de Símbolos
```

O **lexer** transforma o código-fonte em tokens. O **parser** verifica se esses tokens obedecem à gramática. O **analisador semântico** usa a Tabela de Símbolos para regras como declaração e tipos.