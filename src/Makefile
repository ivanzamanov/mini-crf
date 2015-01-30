
CC = g++
CFLAGS = -std=c++11 -Wall -g -I.
OBJ_DIR = bin
OBJ = crf.o
CMD = $(CC) $(CFLAGS)
MAIN_DEPS = $(wildcard */*.hpp */*.cpp)

main: $(MAIN_DEPS)
	$(CMD) main.cpp -o main

# %.o : %.cpp %.hpp
# 	mkdir -p $(OBJ_DIR)
# 	$(CC) -c $< -o $(OBJ_DIR)/$@
# 
# crf.so: $(OBJ)
# 	$(CC) -shared -fPIC bin/$^ -o $@
