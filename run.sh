#!/bin/bash

#make clean
set -e
make tool.so

g++ -o test.out test/test.cpp -std=c++11 -fopenmp -gdwarf-2 -O2

${PIN_ROOT}/pin.sh -t tool.so -- ./test.out 16 0