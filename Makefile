CC     = gcc
CFLAGS = -Wall -Iinclude

SRC = src/symbol_table/symbol_table.c \
      src/lexer/lexer.c \
      src/main.c \
      src/parser/parser.c

FILE ?= testes/lexer/teste.txt

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

clean:
	-$(CLEAN)

.PHONY: all run clean