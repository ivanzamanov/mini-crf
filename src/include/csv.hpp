#ifndef __CSV_H__
#define __CSV_H__

#include<string>
#include<array>
#include<iostream>
#include<fstream>

template<int columns>
class CSVOutput {
public:
  CSVOutput(std::string file): str(file), has_output_headers(false) {}

  void header(int i, std::string str) {
    headers[i] = str;
  }

  template<class T>
  CSVOutput& operator<<(const std::array<T, columns> output) {
    if(!has_output_headers) {
      str << headers[0];
      for(int i = 1; i < columns; i++)
        str << '\t' << headers[i];
      str << '\n';
      has_output_headers = true;
    }

    str << output[0];
    for(int i = 1; i < columns; i++)
      str << '\t' << output[i];
    str << '\n';
    return *this;
  }

  CSVOutput& comment(std::string comment) { str << "# " << comment << '\n'; }

private:
  std::ofstream str;
  std::string headers[columns];

  bool has_output_headers;
};

#endif
