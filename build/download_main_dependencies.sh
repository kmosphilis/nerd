#! /bin/bash
cd "${0%/*}"
mkdir -p ../libs
cd ../libs
wget -nc https://ftp.gnu.org/pub/gnu/avl/avl-2.0.3.tar.gz\
 https://www.pcg-random.org/downloads/pcg-c-0.94.zip
tar -xf avl-2.0.3.tar.gz
rm avl-2.0.3.tar.gz
gcc -std=c99 -c avl-2.0.3/prb.c -o prb.o
unzip pcg-c-0.94.zip
rm pcg-c-0.94.zip
cd pcg-c-0.94
make
