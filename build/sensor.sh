#! /bin/bash
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f ../bin/sensor
gcc -std=c2x -Wall -Wextra -o ../bin/sensor ../src/literal.c ../src/scene.c ../src/sensor.c \
../test/sensor.c -lcheck -lm
cd ..
./bin/sensor
