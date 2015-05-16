#ifndef __PRAAT_PARSER_H__
#define __PRAAT_PARSER_H__

#include<iostream>
#include<vector>
#include<string>
#include<array>

#include"../crf/matrix.hpp"
#include"../crf/util.hpp"

static const int MFCC_N = 12;

typedef std::array<double, MFCC_N> MfccArray;

static const std::string PHONETIC_LABELS[] = {
    "A",
    "a",
    "O",
    "o",
    "U",
    "u",
    "Y",
    "y",
    "E",
    "e",
    "I",
    "i",
    "ja",
    "jA",
    "jy",
    "jY",
    "ju",
    "jU",
    "je",
    "jE",
    "b",
    "b,",
    "v",
    "v,",
    "g",
    "g,",
    "d",
    "d,",
    "w",
    "z",
    "z,",
    "%",
    "q",
    "q,",
    "k",
    "k,",
    "l",
    "l,",
    "l1",
    "l1,",
    "l2",
    "l2,",
    "m",
    "m,",
    "n",
    "n,",
    "p",
    "p,",
    "r",
    "r,",
    "r1",
    "r2",
    "r1,",
    "r2,",
    "s",
    "s,",
    "t",
    "t,",
    "f",
    "f,",
    "h",
    "h,",
    "c",
    "c,",
    "x",
    "ch",
    "sh",
    "j",
    "_",
    "$",
    ":",
    "*",
    " "
  };

struct PhoneticLabelUtil {
  static std::string fromInt(unsigned i) {
    return PHONETIC_LABELS[i];
  }

  static void removeObsoletes(std::string& str) {
    std::string result;
    for(auto it = str.begin(); it != str.end(); it++) {
      char c = *it;
      if(!(c == '\"' || c == '~' || c == ':' || c == '\''))
        result.push_back(c);
    }
    str = result;
  }

  static unsigned fromString(std::string str) {
    removeObsoletes(str);
    const std::string* label = PHONETIC_LABELS;
    unsigned i = 0;
    while(*label != " " && *label != str) {
      i++; label++;
    }
    if(*label == " ")
      std::cerr << "Invalid label " << str << std::endl;
    //std::cerr << "From string " << count++ << " " << str << " = " << i << std::endl;
    return i;
  }
};

struct Frame {
  Frame():pitch(0) { init(); }
  Frame(double pitch): pitch(pitch) { init(); }

  MfccArray mfcc;
  double pitch;

private:
  void init() { mfcc.fill(0); }
};

static const unsigned PITCH_CONTOUR_LENGTH = 2;
struct PitchContour : std::array<double, PITCH_CONTOUR_LENGTH> {
  double diff(const PitchContour& other) const {
    double result = 0;
    for(unsigned i = 0; i < size(); i++) {
      result += std::abs( (*this)[i] - other[i] );
    }
    return result;
  }
};

typedef unsigned int PhoneticLabel;

struct PhonemeInstance {
  PhonemeInstance() {
    start = 0;
    end = 0;
    label = ' ';
    id = 0;
  }
  Array<Frame> frames;
  PitchContour pitch_contour;
  double start;
  double end;
  double duration;
  unsigned id;
  PhoneticLabel label;

  unsigned size() const { return frames.length; }

  const Frame& at(unsigned index) const { return frames[index]; }
  const Frame& first() const { return frames[0]; }
  const Frame& last() const { return frames[size() - 1]; }
};

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

PhonemeInstance* parse_file(std::istream&, int&);

bool compare(Frame&, Frame&);
bool compare(PhonemeInstance&, PhonemeInstance&);

bool operator==(const PhonemeInstance&, const PhonemeInstance&);
#endif
