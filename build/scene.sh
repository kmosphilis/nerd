#! /bin/bash
set -x
cd "${0%/*}"
mkdir ../bin
rm -f ../bin/scene
gcc -Wall -Wextra -o ../bin/scene ../src/literal.c ../src/scene.c ../test/helper/literal.c \
../test/scene.c -lcheck
../bin/scene