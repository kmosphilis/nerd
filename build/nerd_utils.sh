#! /bin/bash
executable=../bin/nerd_utils
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f $executable
gcc -std=c2x -Wall -Wextra -o $executable ../src/nerd_utils.c ../test/nerd_utils.c -lcheck\
 -L../libs/pcg-c-0.94/src -lpcg_random -I../libs/pcg-c-0.94/include/
if $executable; then
    printf "\n"
    valgrind --leak-check=full $executable
fi
