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

struct Frame {
  Frame():pitch(0) { init(); }
  Frame(double pitch): pitch(pitch) { init(); }

  MfccArray mfcc;
  double pitch;

private:
  void init() { mfcc.fill(0); }
};

typedef std::array<Frame, 2> FrameArray;

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
  FrameArray frames;
  PitchContour pitch_contour;
  double start;
  double end;
  double duration;
  unsigned id;
  PhoneticLabel label;

  unsigned size() const { return frames.size(); }

  const Frame& at(unsigned index) const { return frames[index]; }
  const Frame& first() const { return frames[0]; }
  const Frame& last() const { return frames[size() - 1]; }
};

struct StringLabelProvider {
  std::vector<std::string> labels;

  std::string fromInt(unsigned i) {
    return labels[i];
  }

  void removeObsoletes(std::string& str) {
    std::string result;
    for(auto it = str.begin(); it != str.end(); it++) {
      char c = *it;
      if(!(c == '\"' || c == '~' || c == ':' || c == '\''))
        result.push_back(c);
    }
    str = result;
  }

  unsigned fromString(std::string str) {
    removeObsoletes(str);
    auto it = labels.begin();
    unsigned i = 0;
    while(it != labels.end() && *it != str) {
      i++;
      it++;
    }
    if(it == labels.end())
      labels.push_back(str);
    return i;
  }
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

PhonemeInstance* parse_file(std::istream&, int&, StringLabelProvider&);

bool compare(Frame&, Frame&);
bool compare(PhonemeInstance&, PhonemeInstance&);

bool operator==(const PhonemeInstance&, const PhonemeInstance&);

#endif
