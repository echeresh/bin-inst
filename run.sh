#!/bin/bash

if [ -z "$PIN_ROOT" ]; then
    source tools/setup.sh
fi

set -e

#make clean
make -f make_tool build/tool/tool.so -j 4
make -f make_query -j 4

g++ -o build/test.out test/test.cpp -std=c++11 -fopenmp -gdwarf-2 -O0

(cd ..; test=lu ./run_npb.sh; test=cg ./run_npb.sh)

: ${exe_cmd:="./build/test.out 16 0"}

set +e

mkdir -p bin

if [ -n "$debug" ]; then
${PIN_ROOT}/intel64/bin/pinbin -pause_tool 10 -t build/tool/tool.so -- $exe_cmd
else
${PIN_ROOT}/pin.sh -t build/tool/tool.so -- $exe_cmd
fi

./build/query/query.exe

rm -rf calls dwarf.log
