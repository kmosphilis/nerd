#! /bin/bash
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f ../bin/nerd
gcc -std=c2x -g -Wall -Wextra -o ../bin/nerd ../src/int_vector.c ../src/literal.c ../src/scene.c \
../src/context.c ../src/rule.c ../src/rule_queue.c ../src/knowledge_base.c ../src/sensor.c \
../src/nerd.c ../test/helper/rule_queue.c ../test/nerd.c -lcheck -lm
cd ../src/
../bin/nerd
