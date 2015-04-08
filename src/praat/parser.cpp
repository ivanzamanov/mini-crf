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
    result[i].frames.length = frames;
    result[i].frames.data = new Frame[frames];

    for(int frame = 0; frame < frames; frame++) {
      int c;

      std::stringstream index_str;
      result[i].frames[frame].pitch = value<double>(stream, "pitch");
      for(c = 0; c < MFCC_N; c++) {
        index_str.seekp(0);
        index_str << c+1;
        result[i].frames[frame].mfcc[c] = value<double>(stream, index_str.str());
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

BinaryWriter& operator<<(BinaryWriter& str, const PhonemeInstance& ph) {
  str << ph.frames.length;
  for(unsigned i = 0; i < ph.frames.length; i++)
    str << ph.frames[i];

  str << ph.start;
  str << ph.end;
  str << ph.label;
  return str;
}

BinaryReader& operator>>(BinaryReader& str, PhonemeInstance& ph) {
  unsigned length;
  str >> length;
  ph.frames.data = new Frame[length];
  ph.frames.length = length;
  for(unsigned i = 0; i < length; i++)
    str >> ph.frames[i];

  str >> ph.start;
  str >> ph.end;
  str >> ph.label;
  return str;
}

bool compare(Frame& p1, Frame& p2) {
  bool same = p1.pitch == p2.pitch;
  for(int i = 0; i < p1.mfcc.length(); i++)
    same = same && p1.mfcc[i] == p2.mfcc[i];
  return same;
}

bool compare(PhonemeInstance& p1, PhonemeInstance& p2) {
  return compare(p1.frames, p2.frames) && p1.start == p2.start && p1.end == p2.end && p1.label == p2.label;
}
