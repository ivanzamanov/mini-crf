#ifndef __FEATURES_H__
#define __FEATURES_H__

#include<cmath>
#include"crf.hpp"
#include"speech_synthesis.hpp"

typedef CRandomField<PhonemeAlphabet, PhonemeInstance> CRF;

template<class Evaluate>
class StateFunction : public CRF::StateFunction {
public:
  double operator()(const std::vector<CRF::Label>& labels, int pos, const std::vector<CRF::Input>& inputs) const {
    const PhonemeInstance& prev = labels[pos];
    const PhonemeInstance& next = inputs[pos];
    Evaluate eval;
    return eval(prev, next);
  }

  double operator()(const CRF::Label l1, int pos, const std::vector<CRF::Input>& inputs) const {
    const PhonemeInstance& prev = l1;
    const PhonemeInstance& next = inputs[pos];
    Evaluate eval;
    return eval(prev, next);
  }
};

template<class Evaluate>
class TransitionFunction : public CRF::TransitionFunction {
public:
  double operator()(const std::vector<CRF::Label>& labels, int pos, const std::vector<CRF::Input>&) const {
    const PhonemeInstance& prev = labels[pos - 1];
    const PhonemeInstance& next = labels[pos - 1];
    Evaluate eval;
    return eval(prev, next);
  }

  double operator()(const CRF::Label& l1, const CRF::Label& l2, int, const std::vector<CRF::Input>&) const {
    const PhonemeInstance& prev = l1;
    const PhonemeInstance& next = l2;
    Evaluate eval;
    return eval(prev, next);
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
    return std::abs(d1/d2);
  }
};

struct MFCCDist {
  double operator()(const PhonemeInstance& prev, const PhonemeInstance& next) const {
    const MfccArray& mfcc1 = prev.last().mfcc;
    const MfccArray& mfcc2 = next.first().mfcc;
    double result = 0;
    for (unsigned i = 0; i < mfcc1.length(); i++) {
      // double diff = ((12 - i) / 12) * (mfcc1[i] - mfcc2[i]);
      double diff = mfcc1[i] - mfcc2[i];
      result += diff * diff;
    }
    return result;
  }
};

std::vector<CRF::StateFunction*> state_functions() {
  std::vector<CRF::StateFunction*> result;
  result.push_back(new StateFunction<Duration>());
  result.push_back(new StateFunction<Pitch>());
  return result;
}

std::vector<CRF::TransitionFunction*> transition_functions() {
  std::vector<CRF::TransitionFunction*> result;
  //result[0] = new TransitionFunction<Duration>();
  result.push_back(new TransitionFunction<Pitch>());
  result.push_back(new TransitionFunction<MFCCDist>());
  return result;
}

#endif


