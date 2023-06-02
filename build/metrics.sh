#! /bin/bash
executable=../bin/metrics
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f $executable
gcc -g -std=c2x -Wall -Wextra -o $executable ../src/nerd_utils.c ../src/literal.c ../src/scene.c\
 ../src/context.c ../src/rule.c ../src/rule_queue.c ../src/rule_hypergraph.c ../libs/prb.o\
 ../src/knowledge_base.c ../src/sensor.c ../src/nerd_helper.c ../src/nerd.c ../src/metrics.c\
 ../test/metrics.c -lcheck -lm -L../libs/pcg-c-0.94/src -lpcg_random -I../libs/pcg-c-0.94/include/\
 -I../libs/avl-2.0.3/
cd ../src/
if $executable; then
    printf "\n"
    valgrind --leak-check=full $executable
fi
