#ifndef __DOT_H__
#define __DOT_H_

#include<cstdio>

struct DotPrinter {
  DotPrinter(const char* const filePath): filePath(filePath) { };
  const char* const filePath;
  void start();
  void end();
  void edge(int src, int  dest, char label='-', int payload=0);
  void node(int id, int payload=0, bool isFinal=false);
private:
  FILE* file;
};

#endif
