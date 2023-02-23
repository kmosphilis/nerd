#! /bin/bash
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f ../bin/context
gcc -std=c2x -Wall -Wextra -o ../bin/context ../src/literal.c ../src/scene.c ../src/context.c \
../test/context.c -lcheck
../bin/context
