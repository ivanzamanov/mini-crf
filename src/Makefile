
CC = clang++
CFLAGS = -std=c++11 -Wall -I./include -g
OPTS = -O3 -Ofast -mtune=native
#CLIB = -stdlib=libc++
CMD = $(CC) $(CLIB) $(CFLAGS) $(OPTS)
LIBS = -lpthread -lOpenCL
CMD_NOOPT = $(CC) $(CLIB) $(CFLAGS)
CPP_FILES = $(wildcard */*.cpp)
OBJ_FILES = $(patsubst %.cpp,%.o,$(CPP_FILES))
MAIN_DEPS = $(wildcard */*.hpp *.hpp) main.cpp $(OBJ_FILES)

main: $(MAIN_DEPS)
	$(CMD_NOOPT) main.cpp $(OBJ_FILES) -o main $(LIBS)

test: $(MAIN_DEPS) test.cpp
	$(CMD) test.cpp $(CPP_FILES) -o test $(LIBS)

db-stats: $(MAIN_DEPS) db-stats.cpp
	$(CMD) db-stats.cpp $(CPP_FILES) -o db-stats $(LIBS)

include/tool.o: include/tool.cpp
	$(CMD) $< -c -o $@

%.o: %.cpp
	$(CMD) $< -c -o $@

main-opt: $(MAIN_DEPS)
	$(CMD) main.cpp include/*.o -o main-opt $(LIBS)

text2bin: $(MAIN_DEPS) text2bin.cpp
	$(CMD_NOOPT) text2bin.cpp -g include/*.o -o text2bin $(LIBS)

single: speech_mod_single.cpp $(OBJ_FILES)
	$(CMD) $< $(OBJ_FILES) -o $@ $(LIBS)

all: clean main test main-opt text2bin text2bin-opt

clean:
	rm -f $(wildcard *.exe) main main-opt db-stats test text2bin text2bin-opt
	rm -f */*.o
