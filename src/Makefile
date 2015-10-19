
CC = clang++
CFLAGS = -std=c++11 -Wall -I./include -g
OPTS = -Ofast -mtune=native
#CLIB = -stdlib=libc++
CMD = $(CC) $(CLIB) $(CFLAGS) $(OPTS)
CMD_NOOPT = $(CC) $(CLIB) $(CFLAGS)
CPP_FILES = $(wildcard */*.cpp)
OBJ_FILES = $(patsubst %.cpp,%.o,$(CPP_FILES))
MAIN_DEPS = $(wildcard */*.hpp *.hpp) main.cpp $(OBJ_FILES)

main: $(MAIN_DEPS)
	$(CMD) main.cpp $(OBJ_FILES) -o main -lpthread

test: $(MAIN_DEPS) test.cpp
	$(CMD) test.cpp $(CPP_FILES) -o test -lpthread

db-stats: $(MAIN_DEPS) db-stats.cpp
	$(CMD) db-stats.cpp $(CPP_FILES) -o db-stats -lpthread

include/tool.o: include/tool.cpp
	$(CMD) $< -c -o $@

%.o: %.cpp
	$(CMD_NOOPT) $< -c -o $@

main-opt: $(MAIN_DEPS)
	$(CMD) $(OPTS) main.cpp include/*.o -o main-opt -lpthread

text2bin: $(MAIN_DEPS) text2bin.cpp
	$(CMD) text2bin.cpp -g include/*.o -o text2bin

single: speech_mod_single.cpp $(OBJ_FILES)
	$(CMD) $< $(OBJ_FILES) -o $@ -lpthread

all: clean main test main-opt text2bin text2bin-opt

clean:
	rm -f $(wildcard *.exe) main main-opt db-stats test text2bin text2bin-opt
	rm -f */*.o
