#include<stdio.h>

#include"dot.hpp"

void DotPrinter::start() {
  if(filePath != 0) {
    file = fopen(filePath, "w");
    fprintf(file, "digraph {\nrankdir = LR;\n");
    fprintf(file, "ranksep=2\n");
    //fprintf(file, "nodesep=1.1\n");
    fprintf(file, "splines=line;\n");
  }
}

void DotPrinter::end() {
  if(filePath != 0) {
    fprintf(file, "}\n");
    fclose(file);
  }}

void DotPrinter::edge(int src, int dest, char label, int payload) {
  if(filePath != 0) {
    // fprintf(file, "%d -> %d [label=\"%c:%d\"]\n", src, dest, label, payload);
    fprintf(file, "%d -> %d [label=\"%d\"]\n", src, dest, payload);
  }
}

void DotPrinter::node(int id, int output, bool isFinal) {
  const char* shape = isFinal ? "doublecircle" : "circle";
  if(filePath != 0)
    fprintf(file, "%d [label=\"%d:%d\"shape=%s]\n", id, id, output, shape);
}
