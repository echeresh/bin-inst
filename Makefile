export PIN_ROOT

BUILDDIR := build
SOURCEDIR := src
SOURCES := $(shell find $(SOURCEDIR) -name '*.cpp')
OBJECTS := $(addprefix $(BUILDDIR)/,$(subst src/,,$(SOURCES:%.cpp=%.o)))

# SOURCES_DWARF = dwarfparser.cpp \
#                 debuginfo.cpp

# SOURCES = $(SOURCES_DWARF) \
#           execcontext.cpp \
#           exechandler.cpp \

#OBJECTS = $(subst .cpp,.o,$(SOURCES))
#OBJECTS_DWARF = $(subst .cpp,.o,$(SOURCES_DWARF))

INC = -I$(SOURCEDIR) \
      -I$(PIN_ROOT)/source/include/pin \
      -I$(PIN_ROOT)/source/include/pin/gen \
      -I$(PIN_ROOT)/extras/components/include \
      -I$(PIN_ROOT)/extras/xed2-intel64/include \
      -I$(PIN_ROOT)/source/tools/InstLib \


CFLAGS = -O3 -DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX -g
CFLAGS += $(INC)
CFLAGS += -std=c++11


LIBS = -L$(PIN_ROOT)/extras/xed2-intel64/lib \
       -L$(PIN_ROOT)/intel64/lib \
       -L$(PIN_ROOT)/intel64/lib-ext \
       -L$(PIN_ROOT)/intel64/runtime/glibc \
       -lpin -lxed -ldwarf -lelf -ldl

LDFLAGS = -g \
          $(LIBS) \
          -Wl,--hash-style=sysv \
          -Wl,-Bsymbolic \
          -Wl,--version-script=$(PIN_ROOT)/source/include/pin/pintool.ver

TOOL_CFLAGS = $(CFLAGS)
TOOL_CFLAGS += -fomit-frame-pointer -DBIGARRAY_MULTIPLIER=1 \
               -Wall -Werror -Wno-unknown-pragmas -fno-stack-protector

$(BUILDDIR)/%.o: $(SOURCEDIR)/%.cpp
	mkdir -p $(dir $@)
	g++ -std=c++11 -c $^ -o $@ $(CFLAGS)

%.so: %.cpp $(OBJECTS)
	g++ -shared -o $@ $^ $(TOOL_CFLAGS) $(LDFLAGS)

all: tool.so

.SECONDARY: $(OBJECTS)

# test: $(OBJECTS_DWARF)
# 	g++ -o test dwarftest.cpp $^ $(INC) $(LIBS) -std=c++11

clean:
	rm -rf $(BUILDDIR)
	rm -rf *.o *.so
