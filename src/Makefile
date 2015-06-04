
CC = g++
CFLAGS = -std=c++11 -Wall -I.
OBJ_DIR = bin
OBJ = crf.o
CMD = $(CC) $(CFLAGS)
MAIN_DEPS = $(wildcard */*.hpp */*.cpp *.hpp) main.cpp
CPP_FILES = $(wildcard */*.cpp)

main: $(MAIN_DEPS)
	$(CMD) -pg -g main.cpp $(CPP_FILES) -o main

test: $(MAIN_DEPS) test.cpp
	$(CMD) test.cpp $(CPP_FILES) -o test

db-stats: $(MAIN_DEPS) db-stats.cpp
	$(CMD) -Ofast -mtune=native -fopenmp db-stats.cpp $(CPP_FILES) -o db-stats

main-opt: $(MAIN_DEPS)
	$(CC) -static -std=c++11 -Wall -I. -Ofast -mtune=native main.cpp -fopenmp $(CPP_FILES) -o main-opt

text2bin: $(MAIN_DEPS) text2bin.cpp
	$(CMD) text2bin.cpp -g $(CPP_FILES) -o text2bin

text2bin-opt: $(MAIN_DEPS) text2bin.cpp
	$(CMD) -O3 text2bin.cpp $(CPP_FILES) -o text2bin-opt

all: clean main test main-opt text2bin text2bin-opt


clean:
	rm -f $(wildcard *.exe) main main-opt db-stats test text2bin text2bin-opt

# %.o : %.cpp %.hpp
# 	mkdir -p $(OBJ_DIR)
# 	$(CC) -c $< -o $(OBJ_DIR)/$@
#
# crf.so: $(OBJ)
# 	$(CC) -shared -fPIC bin/$^ -o $@
