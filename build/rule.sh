#! /bin/bash
set -x
cd "${0%/*}"
mkdir ../bin
rm -f ../bin/rule
gcc -o ../bin/rule ../src/rule.c ../src/literal.c ../test/rule.c -lcheck
../bin/rule