#! /bin/bash
executable=../bin/rule
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f $executable
gcc -g -std=c2x -Wall -Wextra -o $executable ../src/nerd_utils.c ../src/literal.c ../src/rule.c\
 ../src/scene.c ../src/context.c ../test/rule.c -lcheck -lm
if $executable; then
    printf "\n"
    valgrind --leak-check=full $executable
fi
