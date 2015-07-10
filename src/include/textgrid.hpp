#ifndef __TEXT_GRID_H__
#define __TEXT_GRID_H__

#include<iostream>
#include<sstream>
#include<fstream>
#include<cstdlib>
#include<string>

struct Interval {
  Interval() { }
  Interval(double xmin, double xmax, const std::string text):
    xmin(xmin), xmax(xmax), text(text) { }
  double xmin;
  double xmax;
  std::string text;
};

class TextGrid {
public:
  TextGrid(unsigned size): s(size), intervals(new Interval[size]) { }
  const Interval& operator[](unsigned x) const { return intervals[x]; }
  Interval& operator[](unsigned x) { return intervals[x]; }
  ~TextGrid() { delete[] intervals; }
  unsigned size() const { return this->s; }
private:
  unsigned s;
  Interval* intervals;
};

std::ostream& operator<<(std::ostream&, const TextGrid&);
int readIntervalsCount(std::istream&);
void readIntervals(TextGrid*, std::istream&);
TextGrid* readIntervalsTextGrid(std::string);

#endif
