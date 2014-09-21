export PIN_ROOT=/root/ms/pin
export TOOL_PATH=$1
export EXE_PATH=$2
#echo $TOOL_PATH
#echo $EXE_PATH

rm acc* 2>/dev/null
rm *.o 2> /dev/null
rm tool.so 2> /dev/null

g++ -std=c++11 -g -DBIGARRAY_MULTIPLIER=1 -Wall -Wno-unknown-pragmas -fno-stack-protector -DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX  \
	-I$PIN_ROOT/source/include/pin -I$PIN_ROOT/source/include/pin/gen -I$PIN_ROOT/extras/components/include \
	-I$PIN_ROOT/extras/xed2-intel64/include -I$PIN_ROOT/source/tools/InstLib -O3 -fomit-frame-pointer -fno-strict-aliasing   -c -o tool.o $TOOL_PATH
g++ -std=c++11 -g -O3 -fPIC -c dwarfparser.cpp
g++ -std=c++11 -g -O3 -fPIC -c debuginfo.cpp
g++ -std=c++11 -g -O3 -DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX -c execcontext.cpp \
    -I$PIN_ROOT/source/include/pin -I$PIN_ROOT/source/include/pin/gen -I$PIN_ROOT/extras/components/include -I$PIN_ROOT/extras/xed2-intel64/include -I$PIN_ROOT/source/tools/InstLib
g++ -std=c++11 -g -O3 -DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX -c exechandler.cpp \
    -I$PIN_ROOT/source/include/pin -I$PIN_ROOT/source/include/pin/gen -I$PIN_ROOT/extras/components/include -I$PIN_ROOT/extras/xed2-intel64/include -I$PIN_ROOT/source/tools/InstLib
g++ -shared -g -std=c++11 -Wl,--hash-style=sysv -Wl,-Bsymbolic -Wl,--version-script=$PIN_ROOT/source/include/pin/pintool.ver    \
    -o tool.so tool.o debuginfo.o dwarfparser.o execcontext.o exechandler.o -lpthread -L$PIN_ROOT/intel64/lib -L$PIN_ROOT/intel64/lib-ext -L$PIN_ROOT/intel64/runtime/glibc -L$PIN_ROOT/extras/xed2-intel64/lib -lpin -lxed -ldwarf -lelf -ldl

shift
$PIN_ROOT/pin.sh -t tool.so -- ./"$@"