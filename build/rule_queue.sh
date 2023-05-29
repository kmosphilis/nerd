#! /bin/bash
executable=../bin/rule_queue
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f $executable
gcc -std=c2x -Wall -Wextra -o $executable ../src/nerd_utils.c ../src/literal.c ../src/rule.c\
 ../src/scene.c ../src/context.c ../src/rule_queue.c ../test/helper/rule_queue.c\
 ../test/rule_queue.c -lcheck -lm -L../libs/pcg-c-0.94/src -lpcg_random\
 -I../libs/pcg-c-0.94/include/ -I../libs/avl-2.0.3/
if $executable; then
    printf "\n"
    valgrind --leak-check=full $executable
fi
