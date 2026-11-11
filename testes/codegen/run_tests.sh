#!/bin/sh
# Roda o compilador em cada testes/codegen/*.txt e compara o .tac gerado
# com o gabarito .esperado.tac (escrito à mão a partir do SDT/slides).
# Uso: sh testes/codegen/run_tests.sh [caminho-do-compilador]

COMPILER=${1:-./compiler}
DIR=testes/codegen
passed=0
failed=0

for source in "$DIR"/*.txt; do
    name=${source%.txt}
    expected="$name.esperado.tac"
    generated="$name.tac"

    rm -f "$generated"
    "$COMPILER" "$source" > /dev/null

    # --strip-trailing-cr: no Windows o arquivo pode ser gravado com CRLF.
    if [ -f "$generated" ] && diff --strip-trailing-cr "$expected" "$generated" > /dev/null; then
        echo "OK     $source"
        passed=$((passed + 1))
    else
        echo "FALHOU $source"
        if [ -f "$generated" ]; then
            diff --strip-trailing-cr "$expected" "$generated"
        else
            echo "       (nenhum .tac gerado)"
        fi
        failed=$((failed + 1))
    fi
done

echo
echo "$passed passaram, $failed falharam."
[ "$failed" -eq 0 ]
