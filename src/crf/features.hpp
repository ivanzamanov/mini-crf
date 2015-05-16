#ifndef __FEATURES_H__
#define __FEATURES_H__

#include<cmath>
#include"crf.hpp"
#include"speech_synthesis.hpp"

enum FunctionId {
  pitch = 0,
  mfcc = 1
};

struct FeatureValues {
  static const unsigned COUNT = 2;
  double values[COUNT];

  int diff(const FeatureValues& o) const {
    for(unsigned i = 0; i < COUNT; i++) {
      if(values[i] != o.values[i])
        return (int) i;
    }
    return -1;
  }

  unsigned size() const { return COUNT; }
};

double Pitch(const PhonemeInstance& prev, const PhonemeInstance& next, int, const vector<PhonemeInstance>&) {
  return std::abs(prev.pitch_contour[1] - next.pitch_contour[0]);
}

double PitchState(const PhonemeInstance& p1, int pos, const vector<PhonemeInstance>& x) {
  const PhonemeInstance& p2 = x[pos];
  return p1.pitch_contour.diff(p2.pitch_contour);
}

double MFCCDist(const PhonemeInstance& prev, const PhonemeInstance& next, int, const vector<PhonemeInstance>&) {
  const MfccArray& mfcc1 = prev.last().mfcc;
  const MfccArray& mfcc2 = next.first().mfcc;
  double result = 0;
  for (unsigned i = 0; i < mfcc1.size(); i++) {
    double diff = mfcc1[i] - mfcc2[i];
    result += diff * diff;
  }
  return std::sqrt(result);
}

double Duration(const PhonemeInstance& prev, int pos, const vector<PhonemeInstance>& x) {
  const PhonemeInstance& next = x[pos];
  double d1 = prev.end - prev.start;
  double d2 = next.end - next.start;
  return std::abs(d1 - d2);
}

struct PhoneticFeatures;
typedef CRandomField<PhonemeAlphabet, PhonemeInstance, PhoneticFeatures> CRF;

struct PhoneticFeatures {
  typedef double (*_EdgeFeature)(const PhonemeInstance&, const PhonemeInstance&, int, const vector<PhonemeInstance>&);
  typedef double (*_VertexFeature)(const PhonemeInstance&, int, const vector<PhonemeInstance>&);

  _EdgeFeature f[2] = { Pitch, MFCCDist };
  _VertexFeature g[2] = { Duration, PitchState };
};

template<class Eval>
double feature_value(const PhonemeInstance& p1, const PhonemeInstance& p2) {
  Eval eval;
  vector<PhonemeInstance> empty;
  return eval(p1, p2, 0, empty);
}

FeatureValues get_feature_values(const PhonemeInstance& p1, const PhonemeInstance& p2) {
  FeatureValues result;
  vector<PhonemeInstance> dummy;
  result.values[FunctionId::pitch] = Pitch(p1, p2, 0, dummy);
  result.values[FunctionId::mfcc] = MFCCDist(p1, p2, 0, dummy);
  return result;
}

#endif

