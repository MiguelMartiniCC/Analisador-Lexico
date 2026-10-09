# Compilador

Repositório acadêmico em C para a disciplina de Compiladores (IFSC Lages).

O projeto já conta com o **Analisador Léxico** e o **Analisador Sintático** implementados e integrados. O executável (`compiler.exe` no Windows, `compiler` no Linux/macOS) lê um arquivo de código-fonte e executa:

1. **Análise léxica** — processa o conteúdo caractere por caractere e gera a lista de tokens e a **Tabela de Símbolos** (as regras léxicas seguem a tabela de expressões regulares da Aula 4);
2. **Análise sintática** — consome os tokens gerados pelo lexer e valida a gramática da linguagem.

---

## Estrutura de pastas

```text
Compilador/
├── Makefile                    # Compila e executa o compilador
├── include/                    # Cabeçalhos (.h)
│   ├── token.h                 # TokenType e struct Token
│   ├── lexer.h                 # Interface do analisador léxico
│   ├── parser.h                # Interface do analisador sintático
│   └── symbol_table.h          # Interface da Tabela de Símbolos
│
├── src/
│   ├── lexer/
│   │   └── lexer.c             # Implementação do analisador léxico
│   ├── parser/
│   │   └── parser.c            # Implementação do analisador sintático
│   ├── symbol_table/
│   │   └── symbol_table.c      # Implementação da Tabela de Símbolos
│   └── main.c                  # Ponto de entrada
│
└── testes/
    └── lexer/
        └── test2e.txt          # Entrada de teste
```

O executável é gerado na raiz e não faz parte do código-fonte versionado.

---

## Requisitos

- **GCC** no `PATH`
  - Windows: MSYS2 (UCRT64) com `mingw-w64-ucrt-x86_64-gcc`
  - Linux: `sudo apt install build-essential`
  - macOS: `xcode-select --install`
- **make**
  - Windows (MSYS2): `pacman -S mingw-w64-ucrt-x86_64-make`; o comando se chama `mingw32-make`
  - Linux/macOS: `make`

Verifique o compilador:

```powershell
gcc --version
```

---

## Compilando e executando

Todos os comandos abaixo são executados **na raiz do repositório**.

### Com Makefile (recomendado)

```powershell
mingw32-make                                      # compila o compiler.exe
mingw32-make run                                  # roda com o arquivo padrão (testes/lexer/test2e.txt)
mingw32-make run FILE=testes/lexer/outro.txt      # roda com outro arquivo
mingw32-make clean                                # remove o executável
```

> O `FILE` deve apontar para um arquivo que exista; caso contrário o programa termina com `Erro ao abrir o ficheiro: No such file or directory`.

No Linux/macOS, use `make` no lugar de `mingw32-make`.

### Sem Makefile

```powershell
gcc -Wall -Iinclude src/symbol_table/symbol_table.c src/lexer/lexer.c src/parser/parser.c src/main.c -o compiler.exe
.\compiler.exe testes/lexer/test2e.txt
```

- `-Iinclude` aponta para os cabeçalhos
- `-Wall` ativa os avisos do compilador

> No terminal do Windows, se os acentos da saída aparecerem quebrados, rode `chcp 65001` antes do programa.

---

## Tokens reconhecidos

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
| `( ) [ ] { } , ;` | `TOKEN_OPEN_PAREN`, ..., `TOKEN_SEMICOLON` | Símbolo especial |

Onde `D = [0-9]` e `L = [a-zA-Z_]`.

Observações:

- Todos os operadores de comparação chegam ao parser como `TOKEN_COMP`; para saber qual é, consulte o `lexeme`.
- Comentários e espaços são descartados pelo lexer e nunca chegam ao parser.
- Só os `ID` entram na Tabela de Símbolos; palavras reservadas não.
- Todo token carrega `type`, `lexeme` e `line`.

### Erros léxicos

O lexer devolve `TOKEN_ERROR` e continua a análise nos casos:
![alt text](image.png)
- caractere inválido (`@`, `#`, ...)
- `&` ou `|` sozinhos (só `&&` e `||` existem)
- número decimal incompleto (`3.`)
- texto sem aspas de fechamento

---

## Fluxo de execução

```text
Código-fonte
     │
     ▼
   Lexer        (src/lexer)          → imprime a LISTA DE TOKENS
     │
     │ getNextToken()                  → Tabela de Símbolos
     ▼                                 (src/symbol_table) → imprime a tabela
   Token
     │
     ▼
   Parser       (src/parser)         → executa a ANÁLISE SINTÁTICA
```

Em `main.c`, o arquivo-fonte é aberto e o lexer roda primeiro, imprimindo cada token com a linha e o lexeme. Depois o ponteiro do arquivo volta ao início (`fseek`), o lexer é reinicializado e o parser reconsome os tokens sob demanda via `getNextToken()`.

### Interface do parser (`include/parser.h`)

```c
typedef struct {
    Lexer *lexer;
    Token currentToken;
} Parser;

void initParser(Parser *parser, Lexer *lexer);
void parse(Parser *parser);
```

`currentToken` é o lookahead: o token que o parser está examinando agora.

### Consulta à Tabela de Símbolos

A tabela é preenchida pelo lexer com **todos os identificadores encontrados**, tenham sido declarados ou não. A análise semântica (futura) poderá usar a tabela para regras como declaração e tipos.

---

## Próximas etapas

- **Análise semântica** — verificação de declarações e tipos usando a Tabela de Símbolos.
- **Geração de código** — produção do código de destino a partir da árvore sintática.