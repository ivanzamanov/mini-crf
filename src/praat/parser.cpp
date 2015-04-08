#include<sstream>
#include<string>
#include<iostream>
#include<cstring>
#include<vector>

#include"parser.hpp"

static void check_buffer(const std::string& expected, const std::string& actual) {
  if(actual.compare(expected)) {
    std::cerr << "Expected: " << expected << " got: " << actual << '\n';
  }
}

static std::string buffer;
static void section(std::istream& stream, const std::string& check) {
  stream >> buffer;
  check_buffer(check, buffer);
}

template<class T>
static T value(std::istream& stream, const std::string& check) {
  T result;
  stream >> buffer;
  int index = buffer.find_first_of('=');
  check_buffer(check, buffer.substr(0, index));
  std::stringstream string_stream(buffer.substr(index + 1));
  string_stream >> result;
  return result;
}

PhonemeInstance* parse_file(std::istream& stream, int& size) {
  section(stream, "[Config]"); // [Config]
  value<double>(stream, "timeStep"); // timeStep
  value<double>(stream, "mfccWindow"); // mfccWindow
  int mfcc_count = value<int>(stream, "mfcc");
  size = value<int>(stream, "intervals");
  PhonemeInstance* result = new PhonemeInstance[size];

  for(int i = 0; i < size; i++) {
    section(stream, "[Entry]");
    result[i].label = value<char>(stream, "label");
    result[i].start = value<double>(stream, "start");
    result[i].end = value<double>(stream, "end");
    int frames = value<int>(stream, "frames");
    result[i].frames = frames;
    result[i].mfcc = new MfccArray[frames];
    result[i].pitch.data = new double[frames];
    result[i].pitch.length = frames;

    for(int frame = 0; frame < frames; frame++) {
      int c;

      std::stringstream index_str;
      result[i].pitch[frame] = value<double>(stream, "pitch");
      for(c = 0; c < MFCC_N; c++) {
        index_str.seekp(0);
        index_str << c+1;
        result[i].mfcc[frame][c] = value<double>(stream, index_str.str());
      }

      for(; c < mfcc_count; c++) {
        index_str.seekp(0);
        index_str << c+1;
        value<double>(stream, index_str.str());
      }
    }
  }

  return result;
}

std::ostream& operator<<(std::ostream& str, const PhonemeInstance& ph) {
  str << ph.start;
  str << ph.end;
  str << ph.frames;
  str << ph.label;
  for(int i = 0; i < ph.frames; i++) {
    for (int j = 0; j < MFCC_N; j++) {
      str << ph.mfcc[i][j];
    }
  }
  return str;
}

std::istream& operator>>(std::istream& str, PhonemeInstance& ph) {
  str >> ph.start;
  str >> ph.end;
  str >> ph.frames;
  str >> ph.label;
  ph.mfcc = new MfccArray[ph.frames];
  for(int i = 0; i < ph.frames; i++) {
    for (int j = 0; j < MFCC_N; j++) {
      str >> ph.mfcc[i][j];
    }
  }
  return str;
}
