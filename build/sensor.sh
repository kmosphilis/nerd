#! /bin/bash
executable=../bin/sensor
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f $executable
gcc -g -std=c2x -Wall -Wextra -o $executable ../src/nerd_utils.c ../src/literal.c ../src/scene.c\
 ../src/sensor.c ../test/sensor.c -lcheck -lm
cd ../src/
if $executable; then
    printf "\n"
    valgrind --leak-check=full $executable
fi
