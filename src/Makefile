
CC = g++
CFLAGS = -std=c++11 -Wall -I./include
OBJ_DIR = bin
OBJ = crf.o
#CLIB = -stdlib=libc++
CLIB =
CMD = $(CC) $(CFLAGS)
MAIN_DEPS = $(wildcard */*.hpp */*.cpp *.hpp) main.cpp
CPP_FILES = $(wildcard */*.cpp)

main: $(MAIN_DEPS)
	$(CMD) $(CLIB) -pg -g main.cpp $(CPP_FILES) -o main -lpthread

test: $(MAIN_DEPS) test.cpp
	$(CMD) $(CLIB) test.cpp $(CPP_FILES) -o test -lpthread

db-stats: $(MAIN_DEPS) db-stats.cpp
	$(CMD) $(CLIB) -Ofast -mtune=native -fopenmp db-stats.cpp $(CPP_FILES) -o db-stats -lpthread

main-opt: $(MAIN_DEPS)
	$(CC) $(CLIB) -std=c++11 -Wall -I./include -Ofast -mtune=native main.cpp $(CPP_FILES) -o main-opt -lpthread

text2bin: $(MAIN_DEPS) text2bin.cpp
	$(CMD) $(CLIB) text2bin.cpp -g $(CPP_FILES) -o text2bin -lpthread

text2bin-opt: $(MAIN_DEPS) text2bin.cpp
	$(CMD) $(CLIB) -O3 text2bin.cpp $(CPP_FILES) -o text2bin-opt -lpthread

all: clean main test main-opt text2bin text2bin-opt

clean:
	rm -f $(wildcard *.exe) main main-opt db-stats test text2bin text2bin-opt

# %.o : %.cpp %.hpp
# 	mkdir -p $(OBJ_DIR)
# 	$(CC) -c $< -o $(OBJ_DIR)/$@
#
# crf.so: $(OBJ)
# 	$(CC) -shared -fPIC bin/$^ -o $@
