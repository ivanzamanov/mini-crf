#ifndef __PRAAT_PARSER_H__
#define __PRAAT_PARSER_H__

#include<iostream>
#include<vector>

static const int MFCC_N = 12;
template<class T, int _length>
struct FixedArray {
  T values[_length];

  T& operator[](int n) {
    return values[n];
  };

  const T& operator[](int n) const {
    return values[n];
  };

  int length() {
    return _length;
  }
};

template<class T>
struct Array {
  int length;
  T* data;

  T& operator[](int n) {
    return data[n];
  };

  const T& operator[](int n) const {
    return data[n];
  };
};

template<class T>
Array<T> to_array(std::vector<T> &v) {
  Array<T> result;
  result.data = new T[v.size()];
  result.length = v.size();
  auto it = v.begin();
  int i = 0;
  while(it != v.end()) {
    result[i] = *it;
    i++;
    ++it;
  }
  return result;
}

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

std::ostream& operator<<(std::ostream&, PhonemeInstance&);
std::istream& operator>>(std::istream&, PhonemeInstance&);
PhonemeInstance* parse_file(std::istream& stream, int& size);

#endif
