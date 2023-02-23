#! /bin/bash
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f ../bin/literal
gcc -std=c2x -Wall -Wextra -o ../bin/literal ../src/literal.c ../test/literal.c -lcheck
../bin/literal
