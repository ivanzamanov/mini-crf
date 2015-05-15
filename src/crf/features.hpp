#ifndef __FEATURES_H__
#define __FEATURES_H__

#include<cmath>
#include"crf.hpp"
#include"speech_synthesis.hpp"

typedef CRandomField<PhonemeAlphabet, PhonemeInstance> CRF;

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

template<class Eval>
double feature_value(const PhonemeInstance& p1, const PhonemeInstance& p2) {
  Eval eval;
  return eval(p1, p2);
}

template<class Evaluate>
class StateFunction : public CRF::StateFunction {
public:
  double operator()(const CRF::Label& l1, int pos, const std::vector<CRF::Input>& inputs) const {
    return feature_value<Evaluate>(l1, inputs[pos]);
  }
};

template<class Evaluate, FunctionId id>
class TransitionFunction : public CRF::TransitionFunction {
public:
  TransitionFunction() { this->id = id; }

  double operator()(const CRF::Label& l1, const CRF::Label& l2, int, const std::vector<CRF::Input>&) const {
    return feature_value<Evaluate>(l1, l2);
  }
};

struct Duration {
  double operator()(const PhonemeInstance& prev, const PhonemeInstance& next) const {
    double d1 = prev.end - prev.start;
    double d2 = next.end - next.start;
    return std::abs(d1 - d2);
  }
};

struct Pitch {
  double operator()(const PhonemeInstance& prev, const PhonemeInstance& next) const {
    double d1 = prev.last().pitch;
    double d2 = next.first().pitch;
	double result = std::abs( std::log(d1/d2) );
	if(std::isinf(result))
		std::cerr << "Found\n";
    return result;
  }
};

struct PitchState {
  double operator()(const PhonemeInstance& p1, const PhonemeInstance& p2) const {
    return p1.pitch_contour.diff(p2.pitch_contour);
  }
};

struct MFCCDist {
  double operator()(const PhonemeInstance& prev, const PhonemeInstance& next) const {
    const MfccArray& mfcc1 = prev.last().mfcc;
    const MfccArray& mfcc2 = next.first().mfcc;
    double result = 0;
    for (unsigned i = 0; i < mfcc1.length(); i++) {
      double diff = mfcc1[i] - mfcc2[i];
      result += diff * diff;
    }
	if(std::isinf(result))
		std::cerr << "Found\n";
    return std::sqrt(result);
  }
};

std::vector<CRF::StateFunction*> state_functions() {
  std::vector<CRF::StateFunction*> result;
  result.push_back(new StateFunction<PitchState>());
  result.push_back(new StateFunction<Duration>());
  return result;
}

FeatureValues get_feature_values(const PhonemeInstance& p1, const PhonemeInstance& p2) {
  FeatureValues result;
  result.values[FunctionId::pitch] = feature_value<Pitch>(p1, p2);
  result.values[FunctionId::mfcc] = feature_value<MFCCDist>(p1, p2);
  return result;
}

std::vector<CRF::TransitionFunction*> transition_functions() {
  std::vector<CRF::TransitionFunction*> result;
  result.push_back(new TransitionFunction<Pitch, FunctionId::pitch>());
  result.push_back(new TransitionFunction<MFCCDist, FunctionId::mfcc>());
  return result;
}

#endif

