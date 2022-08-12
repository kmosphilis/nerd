#! /bin/bash
set -x
cd "${0%/*}"
mkdir ../bin
rm -f ../bin/literal
gcc -Wall -Wextra -o ../bin/literal ../src/literal.c ../test/helper/literal.c ../test/literal.c \
-lcheck
../bin/literal