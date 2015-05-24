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

void find_min_max(const TextGrid& grid, double& xmin, double& xmax) {
  xmin = grid[0].xmin;
  xmax = grid[0].xmax;
  for(unsigned i = 1; i < grid.size(); i++) {
    if(xmin > grid[i].xmin)
      xmin = grid[i].xmin;
    if(xmax < grid[i].xmax)
      xmax = grid[i].xmax;
  }
}

std::ostream& operator<<(std::ostream& output, const TextGrid& grid) {
  output << "File type = \"ooTextFile\"\n";
  output << "Object class = \"TextGrid\"\n\n";
  double xmin, xmax;
  find_min_max(grid, xmin, xmax);
  output << "xmin = " << xmin << "\n";
  output << "xmax = " << xmax << "\n";
  output << "tiers? <exists>\nsize = 1\nitem []:\nitem [1]:\n";
  output << "class = \"IntervalTier\"\n";
  output << "name = \"ann\"\n";
  output << "xmin = " << xmin << "\n";
  output << "xmax = " << xmax << "\n";
  output << "intervals: size = " << grid.size() << "\n";
  for(unsigned i = 0; i < grid.size(); i++) {
    output << "intervals [" << i+1 << "]:\n";
    output << "xmin = " << grid[i].xmin << "\n";
    output << "xmax = " << grid[i].xmax << "\n";
    output << "label = \"" << grid[i].text << "\"\n";
  }
  return output;
}

static double nextValue(std::istream& input) {
  std::string buf;
  std::string dummy;
  std::getline(input, buf);
  std::stringstream str_stream(buf);
  str_stream >> dummy >> dummy;
  double result;
  str_stream >> result;
  return result;
}

static void nextStringValue(std::istream& input,
                            std::string& result) {
  std::string buf;
  std::getline(input, buf);
  std::stringstream str_stream(buf);
  str_stream >> buf;
  str_stream >> buf;
  str_stream >> result;
}

int readIntervalsCount(std::istream& input) {
  int result;
  std::string buf, dummy;
  std::getline(input, buf);
  std::stringstream str_stream(buf);
  str_stream >> buf >> buf >> buf;
  str_stream >> result;
  return result;
}

void readIntervals(TextGrid* text_grid, std::istream& input) {
  TextGrid& grid = *text_grid;
  std::string buf;

  for(unsigned i = 0; i < grid.size(); i++) {
    std::getline(input, buf); // interval marker
    grid[i].xmin = nextValue(input);
    grid[i].xmax = nextValue(input);
    nextStringValue(input, grid[i].text);
  }
}

TextGrid* readIntervalsTextGrid(std::string filename) {
  std::ifstream input(filename.c_str());
  std::string buf;
  std::getline(input, buf); // File type = ...
  std::getline(input, buf); // Object class = ...
  std::getline(input, buf); // empty line
  std::getline(input, buf); // xmin
  std::getline(input, buf); // xmax
  std::getline(input, buf); // tiers? ...
  std::getline(input, buf); // items count
  std::getline(input, buf); // item array begin marker
  std::getline(input, buf); // item begin marker
  std::getline(input, buf); // class = ...
  std::getline(input, buf); // name = ...
  std::getline(input, buf); // xmin = ...
  std::getline(input, buf); // xmax = ...
  int size = readIntervalsCount(input); // intervals... = ...
  TextGrid* result = new TextGrid(size);
  readIntervals(result, input);
  return result;
}

#endif
