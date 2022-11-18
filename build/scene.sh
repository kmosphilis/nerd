#! /bin/bash
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f ../bin/scene
gcc -Wall -Wextra -o ../bin/scene ../src/literal.c ../src/scene.c ../test/scene.c -lcheck
../bin/scene