CC     = gcc
CFLAGS = -Wall -Iinclude

SRC = src/symbol_table/symbol_table.c \
      src/lexer/lexer.c \
      src/main.c \
      src/parser/parser.c \
      src/semantic/semantic.c \
      src/codegen/codegen.c

FILE ?= testes/compiler/teste.txt

ifeq ($(OS),Windows_NT)
    TARGET = compiler.exe
    RUN    = .\$(TARGET)
    CLEAN  = del /Q /F $(TARGET) 2>nul
else
    TARGET = compiler
    RUN    = ./$(TARGET)
    CLEAN  = rm -f $(TARGET)
endif

all: $(TARGET)

$(TARGET): $(SRC) $(wildcard include/*.h)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	$(RUN) $(FILE)

# Testes do gerador: compara cada testes/codegen/caso.tac com caso.esperado.tac.
# Precisa de sh no PATH (no Windows, o do Git Bash ou do MSYS2).
test: $(TARGET)
	sh testes/codegen/run_tests.sh ./$(TARGET)

clean:
	-$(CLEAN)

.PHONY: all run test clean
