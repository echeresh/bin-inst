#/bin/bash

base="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ "$PIN_ROOT" == "" ]; then
export PIN_ROOT=/root/ms/pin
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${base}/../../../pin/intel64/lib-ext:${base}/../../../pin/intel64/runtime
fi
