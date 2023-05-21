#! /bin/bash
executable=../bin/sensor
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f $executable
gcc -g -std=c2x -Wall -Wextra -o $executable ../src/nerd_utils.c ../src/literal.c ../src/scene.c\
 ../src/sensor.c ../test/sensor.c -lcheck -lm -L../libs/pcg-c-0.94/src -lpcg_random\
 -I../libs/pcg-c-0.94/include
cd ../src/
if $executable; then
    printf "\n"
    valgrind --leak-check=full $executable
fi
