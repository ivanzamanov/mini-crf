#ifndef __PRAAT_PARSER_H__
#define __PRAAT_PARSER_H__

#include<iostream>
#include<vector>

#include"../praat/matrix.hpp"
#include"../crf/util.hpp"

static const int MFCC_N = 12;

typedef FixedArray<double, MFCC_N> MfccArray;

struct Frame {
  Frame() { }
  Frame(double pitch): pitch(pitch) {
    for(unsigned i = 0; i < mfcc.length(); i++) mfcc[i] = 0;
  }

  MfccArray mfcc;
  double pitch;
};

struct PhonemeInstance {
  Array<Frame> frames;
  double start;
  double end;
  char label;

  unsigned size() const { return frames.length; }

  double duration() const { return end - start; }
  const Frame& at(unsigned index) const { return frames[index]; }
  const Frame& first() const { return frames[0]; }
  const Frame& last() const { return frames[size() - 1]; }
};

void print_synth_input_csv(std::ostream&, std::vector<PhonemeInstance>&);
std::vector<PhonemeInstance> parse_synth_input_csv(std::istream&);

BinaryWriter& operator<<(BinaryWriter&, const PhonemeInstance&);
BinaryReader& operator>>(BinaryReader&, PhonemeInstance&);

PhonemeInstance* parse_file(std::istream& stream, int& size);

bool compare(Frame& p1, Frame& p2);
bool compare(PhonemeInstance& p1, PhonemeInstance& p2);

#endif
