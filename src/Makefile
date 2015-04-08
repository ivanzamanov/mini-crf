
CC = g++
CFLAGS = -std=c++11 -Wall -g -I.
OBJ_DIR = bin
OBJ = crf.o
CMD = $(CC) $(CFLAGS)
MAIN_DEPS = $(wildcard */*.hpp */*.cpp *.hpp) main.cpp
CPP_FILES = $(wildcard */*.cpp)

main: $(MAIN_DEPS)
	$(CMD) main.cpp $(CPP_FILES) -o main

test: $(MAIN_DEPS) test.cpp
	$(CMD) test.cpp $(CPP_FILES) -o test

main-opt: $(MAIN_DEPS) test
	$(CMD) -O3 main.cpp $(CPP_FILES) -o main-opt

text2bin: $(MAIN_DEPS) text2bin.cpp
	$(CMD) text2bin.cpp $(CPP_FILES) -o text2bin

all: clean main test main-opt text2bin
  

clean:
	rm -f $(wildcard *.exe) main test text2bin

# %.o : %.cpp %.hpp
# 	mkdir -p $(OBJ_DIR)
# 	$(CC) -c $< -o $(OBJ_DIR)/$@
# 
# crf.so: $(OBJ)
# 	$(CC) -shared -fPIC bin/$^ -o $@
