#! /bin/bash
set -x
cd "${0%/*}"
mkdir ../bin
rm -f ../bin/knowledge_base
gcc -Wall -Wextra -o ../bin/knowledge_base ../src/literal.c ../src/rule.c ../src/rule_queue.c \
../src/knowledge_base.c ../test/helper/literal.c ../test/helper/rule.c ../test/helper/rule_queue.c \
../test/helper/knowledge_base.c ../test/knowledge_base.c -lcheck -lm
../bin/knowledge_base