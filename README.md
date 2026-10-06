# Analisador Léxico em C

Este projeto é um **Analisador Léxico (Lexer)** desenvolvido em linguagem C para fins acadêmicos.

O objetivo principal é ler um arquivo contendo código-fonte, processar seu conteúdo caractere por caractere e gerar como saída a **lista de tokens reconhecidos** e a **Tabela de Símbolos**.

---

## 📁 Estrutura de Arquivos e Pastas

```text
analisador_lexico/
├── include/                  # Arquivos de cabeçalho (.h) - Interfaces do sistema
│   ├── token.h              # Definição do enum TokenType e da struct Token
│   ├── symbol_table.h       # Protótipos das funções da Tabela de Símbolos
│   └── lexer.h              # Protótipos e estrutura principal do Analisador Léxico
│
├── src/                      # Arquivos de código-fonte (.c) - Implementações
│   ├── lexer.c              # Lógica do analisador léxico
│   ├── symbol_table.c       # Implementação da busca e inserção na Tabela de Símbolos
│   └── main.c               # Ponto de entrada: gerencia os arquivos e exibe as saídas
│
├── Makefile                 # Script de automação da compilação
├── teste.txt                # Arquivo de entrada contendo o código de teste
└── lexer.exe                # Executável compilado
```

---

## 🛠️ Requisitos do Sistema

Para executar o projeto, é necessário ter:

- **VS Code** instalado.
- **Compilador C (`gcc`)** configurado nas variáveis de ambiente do sistema.
  - No Windows: **MinGW / MSYS2**
  - No Linux/macOS: **GCC**
- Extensão **C/C++ da Microsoft** para o VS Code (recomendada).

Para verificar se o GCC está configurado corretamente, execute:

```powershell
gcc --version
```

Caso o comando retorne a versão do compilador, o GCC está configurado corretamente.

---

## 🚀 Executando no Windows com VS Code

### 1. Navegar até a pasta do projeto

Entre no diretório onde está localizado o analisador léxico:

```powershell
cd analisador_lexico
```

Verifique se os arquivos estão presentes:

```powershell
dir
```

A estrutura esperada deve ser semelhante a:

```text
include
src
Makefile
teste.txt
```

---

### 2. Compilar o projeto

Execute o comando:

```powershell
gcc -Wall -Iinclude src/symbol_table.c src/lexer.c src/main.c -o lexer.exe
```

#### Explicação dos parâmetros

- `gcc` → chama o compilador C.
- `-Wall` → habilita diversos avisos importantes do compilador.
- `-Iinclude` → informa ao GCC que os arquivos `.h` estão localizados na pasta `include`.
- `src/symbol_table.c` → compila a implementação da Tabela de Símbolos.
- `src/lexer.c` → compila a implementação do Analisador Léxico.
- `src/main.c` → compila o ponto de entrada do programa.
- `-o lexer.exe` → define o nome do executável gerado.

Após a compilação, o arquivo `lexer.exe` será criado na raiz do projeto.

---

### 3. Executar o analisador léxico

Execute o programa informando o arquivo de entrada:

```powershell
.\lexer.exe teste.txt
```

O analisador irá ler o conteúdo de `teste.txt`, identificar os tokens e realizar o preenchimento da Tabela de Símbolos.

---

## 🧩 Integração com o Analisador Sintático

Para a equipe responsável pelo **Analisador Sintático (Parser)**, a integração recomendada é feita utilizando **leitura sob demanda (On-Demand)**.

Nesse modelo, o Parser **não precisa carregar todos os tokens de uma vez**. Em vez disso, ele solicita ao Lexer o próximo token conforme avança pela gramática.

O fluxo básico é:

```text
Código-fonte
     │
     ▼
   Lexer
     │
     │ getNextToken()
     ▼
   Token
     │
     ▼
   Parser
```

---

## 1. Inclusão dos Cabeçalhos

No arquivo responsável pelo Parser, inclua os cabeçalhos do Analisador Léxico e da Tabela de Símbolos:

```c
#include "lexer.h"
#include "symbol_table.h"
```

Certifique-se de que o compilador também esteja configurado para localizar a pasta `include`.

---

## 2. Consumo de Tokens no Parser

Um exemplo simplificado de integração é:

```c
static Token currentToken;
static Lexer *globalLexer;

// Função responsável por consumir o próximo token
static void advance(void) {
    currentToken = getNextToken(globalLexer);
}

// Início da análise sintática
void parse(Lexer *lexer) {
    globalLexer = lexer;

    // Lookahead: carrega o primeiro token
    advance();

    while (currentToken.type != TOKEN_EOF) {

        // Exemplo:
        // lógica de análise sintática baseada
        // no tipo do token atual

        advance();
    }
}
```

### Funcionamento

A variável:

```c
static Token currentToken;
```

armazena o token que está sendo analisado atualmente.

Já:

```c
static Lexer *globalLexer;
```

mantém uma referência para a instância do Lexer utilizada pelo Parser.

A função:

```c
advance();
```

solicita ao Lexer o próximo token:

```c
currentToken = getNextToken(globalLexer);
```

Dessa maneira, o Parser pode avançar pela sequência de tokens conforme sua gramática exige.

---

## 3. Lookahead

No início da análise, o Parser executa:

```c
advance();
```

Isso faz com que o primeiro token seja carregado em:

```c
currentToken
```

Esse token funciona como o **lookahead**, permitindo que o Parser saiba qual elemento da gramática está próximo antes de realizar a próxima operação.

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

O Parser percorre esses tokens sequencialmente:

```text
currentToken
     │
     ▼
TOKEN_INT
     │
   advance()
     │
     ▼
TOKEN_IDENTIFIER
     │
   advance()
     │
     ▼
TOKEN_ASSIGN
     │
   advance()
     │
     ▼
TOKEN_NUMBER
     │
     ▼
    ...
```

---

## 4. Consulta à Tabela de Símbolos

A Tabela de Símbolos é preenchida automaticamente durante a execução do Lexer.

O Parser ou o Analisador Semântico pode consultar essa tabela utilizando:

```c
getSymbol()
```

Por exemplo:

```c
Symbol *sym = getSymbol(currentToken.lexeme);

if (sym == NULL) {
    printf(
        "Erro Semântico: Variável '%s' não declarada.\n",
        currentToken.lexeme
    );
} else {
    printf(
        "Variável '%s' encontrada! Tipo: %d\n",
        sym->name,
        sym->type
    );
}
```

### Funcionamento

A função:

```c
getSymbol(currentToken.lexeme);
```

procura na Tabela de Símbolos uma entrada correspondente ao lexema do token atual.

Caso a variável não seja encontrada:

```c
sym == NULL
```

o programa pode identificar uma situação de erro semântico, como uma variável utilizada sem declaração.

Caso a variável seja encontrada:

```c
sym != NULL
```

o Parser ou Analisador Semântico pode acessar informações armazenadas na Tabela de Símbolos, como:

```c
sym->name
sym->type
```

---

## 🔄 Fluxo Completo do Compilador

A integração entre os componentes pode ser representada da seguinte maneira:

```text
                 Código-fonte
                      │
                      ▼
             ┌─────────────────┐
             │      Lexer      │
             │ Análise Léxica  │
             └────────┬────────┘
                      │
                      │ Tokens
                      ▼
             ┌─────────────────┐
             │      Parser     │
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

O **Lexer** é responsável por transformar o código-fonte em tokens.

O **Parser** recebe esses tokens e verifica se eles obedecem à estrutura definida pela gramática da linguagem.

O **Analisador Semântico** utiliza as informações da Tabela de Símbolos para verificar regras semânticas, como declarações, tipos e uso correto de identificadores.

---

## 📌 Resumo da Integração

A comunicação entre o Lexer e o Parser segue o modelo:

```text
Parser
  │
  │ getNextToken()
  ▼
Lexer
  │
  ▼
Token
  │
  ▼
Parser
```

Enquanto isso, a Tabela de Símbolos funciona como uma estrutura compartilhada que pode ser consultada posteriormente pelos componentes responsáveis pelas análises sintática e semântica.

Essa abordagem permite que o processamento seja realizado de forma incremental, evitando a necessidade de armazenar todos os tokens em memória antes do início da análise sintática.
