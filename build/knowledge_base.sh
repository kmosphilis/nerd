#! /bin/bash
executable=../bin/knowledge_base
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f $executable
gcc -g -std=c2x -Wall -Wextra -o $executable ../src/nerd_utils.c ../src/literal.c\
 ../src/rule.c ../src/rule_queue.c ../src/knowledge_base.c ../src/scene.c ../src/context.c\
 ../test/helper/rule_queue.c ../src/rule_hypergraph.c ../libs/prb.o ../test/knowledge_base.c -lm\
 -lcheck -L../libs/pcg-c-0.94/src -lpcg_random -I../libs/pcg-c-0.94/include
if $executable; then
    printf "\n"
    valgrind --leak-check=full $executable
fi
