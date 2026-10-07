# Compilador

Repositório acadêmico em C com as etapas do compilador organizadas em pastas irmãs.

Hoje o **Analisador Léxico** está implementado. A pasta **Analisador Sintático** já existe para receber o parser.

O lexer lê um arquivo de código-fonte, processa o conteúdo caractere por caractere e gera a **lista de tokens reconhecidos** e a **Tabela de Símbolos**.

---

## Estrutura de pastas

```text
Compilador/
├── Analisador_Lexico/          # Análise léxica (Lexer)
│   ├── include/                # Cabeçalhos (.h)
│   │   ├── token.h             # TokenType e struct Token
│   │   ├── symbol_table.h      # Interface da Tabela de Símbolos
│   │   └── lexer.h             # Interface do analisador léxico
│   ├── src/                    # Implementações (.c)
│   │   ├── lexer.c
│   │   ├── symbol_table.c
│   │   └── main.c              # Ponto de entrada do lexer
│   └── teste.txt               # Arquivo de entrada para testes
│
└── Analisador_Sintatico/       # Análise sintática (Parser) — em construção
```

O executável `lexer.exe` é gerado dentro de `Analisador_Lexico/` após a compilação e não faz parte do código-fonte versionado.

---

## Requisitos

- **VS Code** (recomendado)
- **GCC** no `PATH`
  - Windows: MinGW / MSYS2
  - Linux/macOS: GCC
- Extensão **C/C++ da Microsoft** no VS Code (opcional)

Verifique o compilador:

```powershell
gcc --version
```

---

## Executando o analisador léxico (Windows)

Abra o repositório pela pasta `Compilador` (raiz deste projeto).

### 1. Entrar na pasta do lexer

```powershell
cd Analisador_Lexico
```

A pasta deve conter:

```text
include
src
teste.txt
```

### 2. Compilar

```powershell
gcc -Wall -Iinclude src/symbol_table.c src/lexer.c src/main.c -o lexer.exe
```

- `-Iinclude` aponta para os cabeçalhos em `Analisador_Lexico/include`
- o executável é gerado em `Analisador_Lexico/lexer.exe`

No VS Code, a task padrão de build já usa esses caminhos a partir da raiz `Compilador`.

A partir da raiz do repositório, o equivalente é:

```powershell
gcc -Wall -IAnalisador_Lexico/include Analisador_Lexico/src/symbol_table.c Analisador_Lexico/src/lexer.c Analisador_Lexico/src/main.c -o Analisador_Lexico/lexer.exe
```

### 3. Executar

Ainda em `Analisador_Lexico`:

```powershell
.\lexer.exe teste.txt
```

O programa lê `teste.txt`, imprime os tokens e preenche a Tabela de Símbolos.

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

Ao compilar o parser, informe a pasta de includes do lexer, por exemplo:

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
    TOKEN_IDENTIFIER
    TOKEN_ASSIGN
    TOKEN_NUMBER
    TOKEN_SEMICOLON
    TOKEN_EOF
```

### Consulta à Tabela de Símbolos

A tabela é preenchida pelo lexer. O parser (ou o analisador semântico) consulta com `getSymbol()`:

```c
Symbol *sym = getSymbol(currentToken.lexeme);

if (sym == NULL) {
    printf("Erro Semântico: Variável '%s' não declarada.\n", currentToken.lexeme);
} else {
    printf("Variável '%s' encontrada! Tipo: %d\n", sym->name, sym->type);
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
