#! /bin/bash
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f ../bin/int_vector
gcc -Wall -Wextra -o ../bin/int_vector ../src/int_vector.c ../test/int_vector.c -lcheck
../bin/int_vector