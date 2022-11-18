#! /bin/bash
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f ../bin/rule
gcc -Wall -Wextra -o ../bin/rule ../src/literal.c ../src/rule.c ../src/scene.c ../src/context.c \
../test/rule.c -lcheck -lm
../bin/rule