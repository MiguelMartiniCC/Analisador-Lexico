# Compilador

Repositório acadêmico em C para a disciplina de Compiladores (IFSC Lages).

O projeto já conta com o **Analisador Léxico**, o **Analisador Sintático** e o **Analisador Semântico** implementados e integrados. O executável (`compiler.exe` no Windows, `compiler` no Linux/macOS) lê um arquivo de código-fonte e executa:

1. **Análise léxica** — processa o conteúdo caractere por caractere e gera a lista de tokens e a **Tabela de Símbolos** (as regras léxicas seguem a tabela de expressões regulares da Aula 4);
2. **Análise sintática** — consome os tokens gerados pelo lexer e valida a gramática da linguagem;
3. **Análise semântica** — executada junto com o parser, confere declarações, tipos, chamadas de função e retornos (veja [Análise semântica](#análise-semântica)).

Explicações detalhadas do código: [`EXPLICACAO.md`](EXPLICACAO.md) (léxico e sintático) e [`EXPLICACAO_SEMANTICA.md`](EXPLICACAO_SEMANTICA.md) (semântico).

---

## Estrutura de pastas

```text
Compilador/
├── Makefile                    # Compila e executa o compilador
├── include/                    # Cabeçalhos (.h)
│   ├── token.h                 # TokenType e struct Token
│   ├── lexer.h                 # Interface do analisador léxico
│   ├── parser.h                # Interface do analisador sintático
│   ├── semantic.h              # Tipos e interface do analisador semântico
│   └── symbol_table.h          # Interface da Tabela de Símbolos
│
├── src/
│   ├── lexer/
│   │   └── lexer.c             # Implementação do analisador léxico
│   ├── parser/
│   │   └── parser.c            # Implementação do analisador sintático
│   ├── semantic/
│   │   └── semantic.c          # Implementação do analisador semântico
│   ├── symbol_table/
│   │   └── symbol_table.c      # Implementação da Tabela de Símbolos
│   └── main.c                  # Ponto de entrada
│
└── testes/
    ├── lexer/
    │   └── test2e.txt          # Entrada de teste do lexer
    ├── compiler/
    │   ├── teste.txt           # Programa válido (passa nas três análises)
    │   └── teste_erro.txt      # Erro sintático (falta ';')
    └── semantico/
        ├── valido.txt          # Programa válido com vetores, recursão e conversões
        └── erro_*.txt          # Um erro semântico por arquivo
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
mingw32-make run                                  # roda com o arquivo padrão (testes/compiler/teste.txt)
mingw32-make run FILE=testes/semantico/erro_float_em_int.txt   # roda com outro arquivo
mingw32-make clean                                # remove o executável
```

> O `FILE` deve apontar para um arquivo que exista; caso contrário o programa termina com `Erro ao abrir o ficheiro: No such file or directory`.

No Linux/macOS, use `make` no lugar de `mingw32-make`.

### Sem Makefile

```powershell
gcc -Wall -Iinclude src/symbol_table/symbol_table.c src/lexer/lexer.c src/parser/parser.c src/semantic/semantic.c src/main.c -o compiler.exe
.\compiler.exe testes/compiler/teste.txt
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
     │
     │ chama a cada declaração, expressão e comando
     ▼
   Semantic     (src/semantic)       → executa a ANÁLISE SEMÂNTICA
                                       e imprime a TABELA SEMÂNTICA
```

Em `main.c`, o arquivo-fonte é aberto e o lexer roda primeiro, imprimindo cada token com a linha e o lexeme. Depois o ponteiro do arquivo volta ao início (`fseek`), o lexer é reinicializado e o parser reconsome os tokens sob demanda via `getNextToken()`. Durante essa segunda passada o parser chama o analisador semântico; se o programa passar nas duas análises, `main.c` imprime a tabela semântica e a mensagem de sucesso.

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

A tabela é preenchida pelo lexer com **todos os identificadores encontrados**, tenham sido declarados ou não. Ela guarda só os nomes; declaração, tipo e escopo ficam na tabela semântica (veja abaixo).

---

## Análise semântica

O analisador semântico (`include/semantic.h` e `src/semantic/semantic.c`) roda **junto com o parser, na mesma passada**: cada função do parser faz as verificações semânticas do trecho da gramática que reconhece. As funções de expressão devolvem o **tipo** do que reconheceram, e esse tipo é usado para conferir operadores, atribuições, argumentos e retornos. Ele tem uma tabela própria de declarações (nome, categoria, tipo e função dona), separada da Tabela de Símbolos do lexer.

No primeiro erro, o programa imprime a mensagem e termina, como no parser:

```text
Erro Semântico [Linha 7]: Atribuição incompatível: 'x' é 'int', mas o valor é 'float'.
```

### Regras verificadas

- **Declarações** — variável, parâmetro ou função declarados duas vezes; uso de variável ou função não declarada.
- **Escopo** — cada função tem o seu escopo (parâmetros e variáveis). Nomes iguais em funções diferentes são permitidos. Uma função só pode ser chamada depois de declarada; ela pode chamar a si mesma.
- **Atribuição** — o tipo do valor precisa caber no destino: `int` cabe em `float` e em `boolean`; `float` não cabe em `int`; texto não pode ser atribuído a variável.
- **Operadores** — `+ - * /` e comparações só com `int`/`float`; `%` só com `int`; `&&`, `||` e `!` com `boolean`/`int`.
- **Condições** — `if` e `while` exigem `boolean` ou `int`.
- **Chamadas** (`func`) — quantidade e tipos dos argumentos conferem com os parâmetros. Uma constante de texto só é aceita por parâmetro `char`. Resultado de função `void` não pode ser atribuído.
- **Retorno** — função `void` não retorna valor; nas demais, o valor precisa caber no tipo de retorno. `main` retorna `int`.
- **Vetores** — índice precisa ser `int`; variável que não é vetor não aceita índice; não pode haver mais índices que dimensões.

A explicação completa (tabela de declarações, escopo, como o parser devolve os tipos, mensagens e limitações) está em [`EXPLICACAO_SEMANTICA.md`](EXPLICACAO_SEMANTICA.md).

### Testes

- `testes/compiler/teste.txt` — programa válido; passa nas três análises.
- `testes/semantico/valido.txt` — programa válido com vetores, recursão e conversões.
- `testes/semantico/erro_*.txt` — um erro semântico por arquivo; a primeira linha de cada um traz a mensagem esperada.

---

## Próximas etapas

- **Geração de código intermediário** — código de três endereços resumido, a partir do programa já validado pelas três análises.