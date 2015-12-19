if [ "$PIN_ROOT" == "" ]; then
export PIN_ROOT=/root/ms/pin
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/root/ms/pin/intel64/lib-ext:/root/ms/pin/intel64/runtime
fi