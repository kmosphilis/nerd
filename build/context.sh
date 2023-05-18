#! /bin/bash
executable="../bin/context"
set -x
cd "${0%/*}"
mkdir -p ../bin
echo $executable
rm -f $executable
gcc -g -std=c2x -Wall -Wextra -o $executable ../src/nerd_utils.c ../src/literal.c ../src/scene.c\
 ../src/context.c ../test/context.c -lcheck
if $executable; then
    printf "\n"
    valgrind --leak-check=full $executable
fi
