#/bin/bash

base="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ "$PIN_ROOT" == "" ]; then
export PIN_ROOT=${base}/../../pin
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${PIN_ROOT}/intel64/lib-ext:${PIN_ROOT}/intel64/runtime
fi
