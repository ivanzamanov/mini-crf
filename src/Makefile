CC = clang++
CFLAGS = -std=c++11 -Wall -Wextra -I./include
DEBUG_FLAGS = -g -fstandalone-debug -fno-omit-frame-pointer
OPTS = -Ofast -mtune=native
#CLIB = -stdlib=libc++

#OPENCL_LIB = -lOpenCL
#OPENCL_DEF = -DUSE_OPENCL

LIBS = -lpthread $(OPENCL_LIB)
CPP_FILES = $(wildcard */*.cpp)
OBJ_FILES = $(patsubst %.cpp,%.o,$(CPP_FILES))
MAIN_DEPS = $(wildcard */*.hpp *.hpp) main.cpp $(OBJ_FILES)

CMD = $(CC) $(OPENCL_DEF) $(CLIB) $(CFLAGS) $(OPTS) $(DEBUG_FLAGS)

main: OPTS= 
main: $(MAIN_DEPS)
	$(CMD) main.cpp include/*.o -o main $(LIBS)

test: OPTS=
test: $(MAIN_DEPS) test.cpp
	$(CMD) test.cpp include/*.o -o test $(LIBS)

db-stats: OPTS=
db-stats: $(MAIN_DEPS) db-stats.cpp
	$(CMD) db-stats.cpp include/*.o -o db-stats $(LIBS)

include/tool.o: include/tool.cpp
	$(CMD) $< -c -o $@

%.o: %.cpp
	$(CMD) $< -c -o $@

main-opt: DEBUG_FLAGS= 
main-opt: $(MAIN_DEPS)
	$(CMD) main.cpp include/*.o -o main-opt $(LIBS)

main-opt-static:
	$(CMD) main.cpp include/*.cpp -o main-opt-static $(LIBS)

text2bin: $(MAIN_DEPS) text2bin.cpp
	$(CMD) text2bin.cpp -g include/*.o -o text2bin $(LIBS)

single: speech_mod_single.cpp $(OBJ_FILES)
	$(CMD) $< $(OBJ_FILES) -o $@ $(LIBS)

is-wav:
	$(CMD) is-wav.cpp -g -o is-wav

all: clean main test main-opt text2bin text2bin-opt

clean:
	rm -f $(wildcard *.exe) main main-opt db-stats test text2bin text2bin-opt
	rm -f */*.o
