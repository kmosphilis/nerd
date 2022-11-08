#! /bin/bash
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f ../bin/nerd
gcc -g -Wall -Wextra -o ../bin/nerd ../src/*.c ../test/nerd.c ../test/helper/*.c -lcheck -lm
cd ../src/
../bin/nerd