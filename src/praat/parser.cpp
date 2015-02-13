#include<sstream>
#include<iostream>
#include<cstring>
#include<vector>

#include"parser.hpp"

static char buffer[1024];
static void section(std::istream& stream) {
  stream >> buffer;
}

template<class T>
static T value(std::istream& stream) {
  T result;
  stream >> buffer;
  char* substr = std::strchr(buffer, '=') + 1;
  std::stringstream string_stream(substr);
  string_stream >> result;
  return result;
}

PhonemeInstance* parse_file(std::istream& stream, int& size) {
  section(stream); // [Config]
  value<double>(stream); // timeStep
  value<double>(stream); // mfccWindow
  int mfcc_count = value<int>(stream);
  size = value<int>(stream);
  PhonemeInstance* result = new PhonemeInstance[size];

  for(int i = 0; i < size; i++) {
    section(stream); // [Entry]
    result[i].label = value<int>(stream);
    result[i].duration = value<double>(stream);
    int frames = value<int>(stream);
    result[i].frames = frames;
    result[i].mfcc = new MfccArray[frames];

    for(int frame = 0; frame < frames; frame++) {
      int c;

      for(c = 0; c < MFCC_N; c++) {
        result[i].mfcc[frame][c] = value<double>(stream);
      }

      for(; c < mfcc_count; c++) {
        value<double>(stream);
      }
    }
  }

  return result;
}
