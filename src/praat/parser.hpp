#ifndef __PRAAT_PARSER_H__
#define __PRAAT_PARSER_H__

#include<iostream>
#include<vector>

#include"../crf/util.hpp"

static const int MFCC_N = 12;

typedef FixedArray<double, MFCC_N> MfccArray;

struct PhonemeInstance {
  double start;
  double end;
  // Array of MFCC arrays
  MfccArray* mfcc;
  // Length of mfcc
  int frames;
  Array<double> pitch;
  char label;
};

// For binary streams only
std::ostream& operator<<(std::ostream&, const PhonemeInstance&);
// For binary streams only
std::istream& operator>>(std::istream&, const PhonemeInstance&);

PhonemeInstance* parse_file(std::istream& stream, int& size);

#endif
