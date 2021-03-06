#ifndef __PRAAT_PARSER_H__
#define __PRAAT_PARSER_H__

#include<iostream>
#include<vector>
#include<string>

#include"types.hpp"
#include"matrix.hpp"
#include"util.hpp"

template<class F>
F takeLog(F val) {
  if(val == 0)
    return 0;
  return std::log(val);
}

template<bool _log>
PitchContour to_pitch_contour(const PhonemeInstance& p) {
  PitchContour result;
  if(_log) {
    result[0] = takeLog(p.first().pitch);
    result[1] = takeLog(p.last().pitch);
  } else {
    result[0] = p.first().pitch;
    result[1] = p.last().pitch;
  }
  return result;
}

void check_buffer(const std::string& expected, const std::string& actual);

template<class T>
T next_value(std::istream& stream, const std::string& check, std::string& buffer) {
  T result;
  stream >> buffer;
  int index = buffer.find_first_of('=');
  check_buffer(check, buffer.substr(0, index));
  std::stringstream string_stream(buffer.substr(index + 1));
  string_stream >> result;
  return result;
}

void print_synth_input_csv(std::ostream&, std::vector<PhonemeInstance>&);
std::vector<PhonemeInstance> parse_synth_input_csv(std::istream&);

BinaryWriter& operator<<(BinaryWriter&, const PhonemeInstance&);
BinaryReader& operator>>(BinaryReader&, PhonemeInstance&);

BinaryWriter& operator<<(BinaryWriter&, const FileData&);
BinaryReader& operator>>(BinaryReader&, FileData&);

std::vector<PhonemeInstance> parse_file(FileData&, StringLabelProvider&);

bool compare(Frame&, Frame&);
bool compare(PhonemeInstance&, PhonemeInstance&);

bool operator==(const PhonemeInstance&, const PhonemeInstance&);

#endif
