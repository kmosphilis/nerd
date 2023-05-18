#! /bin/bash
executable=../bin/literal
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f $executable
gcc -g -std=c2x -Wall -Wextra -o $executable ../src/nerd_utils.c ../src/literal.c\
 ../test/literal.c -lcheck
if $executable; then
    printf "\n"
    valgrind --leak-check=full $executable
fi
