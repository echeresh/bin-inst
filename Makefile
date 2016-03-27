all: tool query

tool:
	make -f make_tool

query:
	make -f make_query

# test: $(OBJECTS_DWARF)
# 	g++ -o test dwarftest.cpp $^ $(INC) $(LIBS) -std=c++11

clean:
	rm -rf build
	rm -rf *.o *.so
