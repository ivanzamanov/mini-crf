
CC = g++
CFLAGS = -std=c++11 -Wall -g -I.
OBJ_DIR = bin
OBJ = crf.o
CMD = $(CC) $(CFLAGS)
MAIN_DEPS = $(wildcard */*.hpp */*.cpp *.hpp *.cpp)
CPP_FILES = $(wildcard */*.cpp)

main: $(MAIN_DEPS)
	$(CMD) main.cpp $(CPP_FILES) -o main

# %.o : %.cpp %.hpp
# 	mkdir -p $(OBJ_DIR)
# 	$(CC) -c $< -o $(OBJ_DIR)/$@
# 
# crf.so: $(OBJ)
# 	$(CC) -shared -fPIC bin/$^ -o $@
