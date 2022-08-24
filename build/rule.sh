#! /bin/bash
set -x
cd "${0%/*}"
mkdir ../bin
rm -f ../bin/rule
gcc -Wall -Wextra -o ../bin/rule ../src/literal.c ../src/rule.c ../test/helper/literal.c \
../test/helper/rule.c ../test/rule.c -lcheck -lm
../bin/rule