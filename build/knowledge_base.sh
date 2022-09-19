#! /bin/bash
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f ../bin/knowledge_base
gcc -Wall -Wextra -o ../bin/knowledge_base ../src/int_vector.c ../src/literal.c ../src/rule.c \
../src/rule_queue.c ../src/knowledge_base.c ../src/scene.c ../src/context.c \
../test/helper/int_vector.c ../test/helper/literal.c ../test/helper/rule.c \
../test/helper/rule_queue.c ../test/helper/knowledge_base.c ../test/knowledge_base.c -lcheck -lm
../bin/knowledge_base