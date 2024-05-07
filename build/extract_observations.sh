#! /bin/bash
executable=../bin/extract_observations
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f $executable
gcc -O3 -std=gnu2x -Wall -Wextra -o $executable ../src/nerd_utils.c ../src/literal.c ../src/scene.c\
 ../src/context.c ../src/rule.c ../src/rule_queue.c ../src/queue.c ../src/rule_hypergraph.c ../libs/prb.o\
 ../src/knowledge_base.c ../src/sensor.c ../src/nerd_helper.c ../src/metrics.c ../src/nerd.c\
 ../src/extract_observations.c -lm -L../libs/pcg-c-0.94/src -lpcg_random\
 -I../libs/pcg-c-0.94/include/ -I../libs/avl-2.0.3/
