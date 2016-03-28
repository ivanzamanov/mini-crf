#ifndef __TYPES_H__
#define __TYPES_H__

#include<cmath>
#include<complex>
#include<vector>
#include<string>
#include<array>

static const int MFCC_N = 12;

typedef double coefficient;
typedef double cost;
typedef double probability;
typedef double frequency;
typedef double stime_t;
typedef double mfcc_t;
typedef std::array<mfcc_t, MFCC_N> MfccArray;
typedef unsigned id_t;

typedef std::complex<cost> cdouble;

typedef int PhoneticLabel;
const PhoneticLabel INVALID_LABEL = -1;

struct Frame {
  Frame():pitch(0) { init(); }
  Frame(frequency pitch): pitch(pitch) { init(); }

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
  PhonemeInstance()
    :start(0),
     end(0),
     duration(0),
     energy(0),
     id(0),
     old_id(0),
     label(INVALID_LABEL),
     ctx_left(INVALID_LABEL),
     ctx_right(INVALID_LABEL)
  { }

  FrameArray frames;
  PitchContour pitch_contour_original;
  PitchContour pitch_contour;
  stime_t start;
  stime_t end;
  stime_t duration;
  stime_t log_duration;
  double energy;
  long energy_val;
  id_t id;
  id_t old_id;
  PhoneticLabel label;
  PhoneticLabel ctx_left;
  PhoneticLabel ctx_right;

  unsigned size() const { return frames.size(); }

  const Frame& at(unsigned index) const { return frames[index]; }
  const Frame& first() const { return frames[0]; }
  const Frame& last() const { return frames[size() - 1]; }
};

struct FileData {
  static FileData of(std::string& path) {
    FileData result;
    result.file = path;
    return result;
  }

  std::vector<double> pitch_marks;
  std::string file;

  bool operator==(const FileData& o) const {
    return pitch_marks == o.pitch_marks && file == o.file;
  }
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

struct Transition {
  void set(int child, cost value) {
    this->child = child;
    this->base_value = value;
  }

  cost base_value;
  int child;

  bool operator>=(const Transition& tr) const {
    return this->base_value > tr.base_value;
  }

  static Transition make(int child, cost value) {
    Transition r;
    r.set(child, value);
    return r;
  }
};

#endif
