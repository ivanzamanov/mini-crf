#ifndef __PRAAT_PARSER_H__
#define __PRAAT_PARSER_H__

#include<iostream>
#include<vector>

#include"../crf/util.hpp"

static const int MFCC_N = 12;

typedef FixedArray<double, MFCC_N> MfccArray;

struct Frame {
  MfccArray mfcc;
  double pitch;
};

struct PhonemeInstance {
  Array<Frame> frames;
  double start;
  double end;
  char label;

  unsigned size() const { return frames.length; }

  const Frame& at(unsigned index) const { return frames[index]; }
  const Frame& first() const { return frames[0]; }
  const Frame& last() const { return frames[size() - 1]; }
};

BinaryWriter& operator<<(BinaryWriter&, const PhonemeInstance&);
BinaryReader& operator>>(BinaryReader&, PhonemeInstance&);

PhonemeInstance* parse_file(std::istream& stream, int& size);

bool compare(Frame& p1, Frame& p2);
bool compare(PhonemeInstance& p1, PhonemeInstance& p2);

#endif
