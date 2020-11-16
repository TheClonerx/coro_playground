# vim: noexpandtab

CXXFLAGS+=-std=c++20 -Iinclude -fcoroutines

ifdef DEBUG
CXXFLAGS+=-g -O0
else
CXXFLAGS+=-O3 -DNDEBUG
endif

all: bin/main

bin/main: obj/main.cpp.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(LDFLAGS) obj/main.cpp.o -o bin/main

obj/main.cpp.o: src/main.cpp include/task.hpp include/generator.hpp
	@mkdir -p obj
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c src/main.cpp -o obj/main.cpp.o

clean:
	rm -rfv obj/ bin/
