#! /bin/bash
executable=../bin/main
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f $executable
gcc -O3 -std=c2x -Wall -Wextra -o $executable ../src/*.c ../libs/prb.o -lm -L../libs/pcg-c-0.94/src\
 -lpcg_random -I../libs/pcg-c-0.94/include
