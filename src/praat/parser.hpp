#ifndef __PRAAT_PARSER_H__
#define __PRAAT_PARSER_H__

#include<iostream>

static const int MFCC_N = 12;
template<class T, int _length>
struct Array {
  T values[_length];

  T& operator[](int n) {
    return values[n];
  };

  const T& operator[](int n) const {
    return values[n];
  };
};

typedef Array<double, MFCC_N> MfccArray;
struct PhonemeInstance {
  double duration;
  MfccArray* mfcc;
  int frames;
  char label;
};

PhonemeInstance* parse_file(std::istream& stream, int& size);

#endif
