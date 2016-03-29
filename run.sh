#!/bin/bash

make clean
set -e
make -f make_tool build/tool/tool.so -j 4
make -f make_query -j 4

g++ -o build/test.out test/test.cpp -std=c++11 -fopenmp -gdwarf-2 -O2

${PIN_ROOT}/pin.sh -t build/tool/tool.so -- ./build/test.out 16 0

rm -rf calls dwarf.log