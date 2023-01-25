#! /bin/bash
set -x
cd "${0%/*}"
mkdir -p ../bin
rm -f ../bin/sensor
gcc -Wall -Wextra -o ../bin/sensor ../src/literal.c ../src/scene.c ../src/sensor.c \
../test/sensor.c -lcheck
cd ..
./bin/sensor
