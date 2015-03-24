#ifndef __DOT_H__
#define __DOT_H_

#include<cstdio>
#include<sstream>
#include<vector>

struct DotElement {
  std::stringstream stream;
  template<class T>
  DotElement& operator<<(T t) {
    stream << t;
    return *this;
  };
};

struct DotPrinter {
  DotPrinter(const char* const filePath): filePath(filePath) { };
  const char* const filePath;
  void start();
  void end();
  DotElement* edge(int src, int  dest, char label='-', int payload=0);
  DotElement* node(int id, int payload=0, bool isFinal=false);
private:
  void add(DotElement* el);
  FILE* file;
  std::vector<DotElement*> elements;
};

#endif
