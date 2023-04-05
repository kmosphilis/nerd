#! /bin/bash
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f ../bin/rule_hypergraph
gcc -g -std=c2x -Wall -Wextra -o ../bin/rule_hypergraph ../src/literal.c ../src/scene.c \
../src/context.c ../src/rule.c ../src/int_vector.c ../src/rule_queue.c ../src/knowledge_base.c \
../src/rule_hypergraph.c ../libs/prb.o -lm -lcheck
../bin/rule_hypergraph
