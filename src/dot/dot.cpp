#include<stdio.h>

#include"dot.hpp"

void DotPrinter::start() {
  if(filePath != 0) {
    file = fopen(filePath, "w");
    fprintf(file, "digraph {\nrankdir = LR;\n");
  } else
    printf("digraph {\nrankdir = LR;\n");
}

void DotPrinter::end() {
  if(filePath != 0) {
    fprintf(file, "}\n");
    fclose(file);
  } else
    printf("}\n");
}

void DotPrinter::edge(int src, int dest, char label, int payload) {
  if(filePath != 0)
    fprintf(file, "%d -> %d [label=\"%c:%d\"]\n", src, dest, label, payload);
  else
    printf("%d -> %d [label=\"%c\"]\n", src, dest, label);
}

void DotPrinter::node(int id, int output, bool isFinal) {
  const char* shape = isFinal ? "doublecircle" : "circle";
  int labelOutput = isFinal ? output : 0;
  if(filePath != 0)
    fprintf(file, "%d [label=\"%d:%d\"shape=%s]\n", id, id, labelOutput, shape);
  else
    printf("%d [label=\"%d:%d\"shape=%s]\n", id, id, labelOutput, shape);
}
