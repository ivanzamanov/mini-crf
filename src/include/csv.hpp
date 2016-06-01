#ifndef __CSV_H__
#define __CSV_H__

#include<string>
#include<array>
#include<iostream>
#include<fstream>

template<int columns>
class CSVOutput {
public:
  CSVOutput(std::string file): str(file),
                               row(0),
                               has_output_headers(false) {
    headers[0] = "row";
  }

  void header(int i, std::string str) {
    headers[i + 1] = str;
  }

  template<class ...H>
  void all_headers(H... args) {
    all_headers_helper(0, args...);
  }

  template<class T>
  CSVOutput& operator<<(const std::array<T, columns>& output) {
    outputHeaders();
    str << row++;

    str << '\t' << output[0];
    for(int i = 1; i < columns; i++)
      str << '\t' << output[i];
    str << '\n';
    return *this;
  }

  template<class ...V>
  void print(V... args) {
    outputHeaders();
    str << row++;
    print_helper(args...);
  }
  
  CSVOutput& comment(std::string comment) { str << "# " << comment << '\n'; }

private:
  template<class Hcar, class ...Hcdr>
  void all_headers_helper(int i, Hcar h, Hcdr... hcdr) {
    header(i, h);
    all_headers_helper(i + 1, hcdr...);
  }
  void all_headers_helper(int) { }

  template<class H, class ...V>
  void print_helper(H h, V... args) {
    str << '\t' << h;
    print_helper(args...);
  }
  void print_helper() {
    str << '\n';
  }

  void outputHeaders() {
    if(!has_output_headers) {
      str << headers[0];
      for(int i = 1; i < columns + 1; i++)
        str << '\t' << headers[i];
      str << '\n';
      has_output_headers = true;
    }
  }

  std::ofstream str;
  std::string headers[columns + 1];

  int row;
  bool has_output_headers;
};

#endif
