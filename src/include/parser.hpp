#ifndef __PRAAT_PARSER_H__
#define __PRAAT_PARSER_H__

#include<iostream>
#include<vector>
#include<string>

#include"types.hpp"
#include"matrix.hpp"
#include"util.hpp"

template<bool _log>
PitchContour to_pitch_contour(const PhonemeInstance& p) {
  PitchContour result;
  if(_log) {
    result[0] = std::log(p.first().pitch);
    result[1] = std::log(p.last().pitch);
  } else {
    result[0] = p.first().pitch;
    result[1] = p.last().pitch;
  }
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
