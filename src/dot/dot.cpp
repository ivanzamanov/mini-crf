#include<stdio.h>

#include"dot.hpp"

void DotPrinter::start() {
  if(filePath != 0) {
    file = fopen(filePath, "w");
    fprintf(file, "digraph {\nrankdir = LR;\n");
    fprintf(file, "ranksep=2.5;\n");
  }
}

void DotPrinter::end() {
  if(filePath != 0) {    
    for(auto it = elements.begin(); it != elements.end(); it++) {
      fprintf(file, "%s];\n", (*it)->stream.str().c_str());
    }
  }

  if(filePath != 0) {
    fprintf(file, "}\n");
    fclose(file);
  }
}

DotElement* DotPrinter::edge(int src, int dest, char label, int payload) {
  DotElement* result = new DotElement();
  *result << src << " -> " << dest << " [label=\"" << label << ":" << payload << "\" ";
  add(result);
  return result;
}

DotElement* DotPrinter::node(int id, int output, bool isFinal) {
  DotElement* result = new DotElement();
  const char* shape = isFinal ? "doublecircle" : "circle";
  *result << id << " [label=\"" << id << ":" << output << "\" shape=" << shape << " ";
  add(result);
  return result;
}

void DotPrinter::add(DotElement* el) {
  elements.push_back(el);
}
