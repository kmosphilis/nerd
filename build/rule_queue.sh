#! /bin/bash
set -x
cd "${0%/*}"
mkdir ../bin
rm -f ../bin/rule_queue
gcc -Wall -Wextra -o ../bin/rule_queue ../src/literal.c ../src/rule.c ../src/rule_queue.c \
../test/helper/literal.c ../test/helper/rule.c ../test/helper/rule_queue.c ../test/rule_queue.c \
-lcheck
../bin/rule_queue