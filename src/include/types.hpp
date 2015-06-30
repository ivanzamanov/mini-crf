#ifndef __TYPES_H__
#define __TYPES_H__

#include<cmath>
#include<vector>
#include<array>

static const int MFCC_N = 12;

typedef float frequency;
typedef float time_point_t;
typedef std::array<double, MFCC_N> MfccArray;
typedef unsigned id_t;

typedef int PhoneticLabel;
const PhoneticLabel INVALID_LABEL = -1;

struct Frame {
  Frame():pitch(0) { init(); }
  Frame(double pitch): pitch(pitch) { init(); }

  MfccArray mfcc;
  frequency pitch;

private:
  void init() { mfcc.fill(0); }
};

typedef std::array<Frame, 2> FrameArray;

struct PitchContour : std::array<frequency, 2> {
  frequency diff(const PitchContour& other) const {
    frequency result = 0;
    for(unsigned i = 0; i < size(); i++) {
      frequency val = (*this)[i] - other[i];
      result += std::abs(val);
    }

    return result;
  }
};

struct PhonemeInstance {
  PhonemeInstance() {
    start = 0;
    end = 0;
    label = ' ';
    id = 0;
  }

  FrameArray frames;
  PitchContour pitch_contour;
  time_point_t start;
  time_point_t end;
  time_point_t duration;
  id_t id;
  PhoneticLabel label;
  PhoneticLabel ctx_left;
  PhoneticLabel ctx_right;

  unsigned size() const { return frames.size(); }

  const Frame& at(unsigned index) const { return frames[index]; }
  const Frame& first() const { return frames[0]; }
  const Frame& last() const { return frames[size() - 1]; }
};

struct StringLabelProvider {
  std::vector<std::string> labels;

  std::string convert(id_t i) {
    return labels[i];
  }

  id_t convert(std::string str) {
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

  void removeObsoletes(std::string& str) {
    std::string result;
    for(auto it = str.begin(); it != str.end(); it++) {
      char c = *it;
      if(!(c == '\"' || c == '~' || c == ':' || c == '\''))
        result.push_back(c);
    }
    str = result;
  }
};

#endif
