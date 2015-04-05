#ifndef __FEATURES_H__
#define __FEATURES_H__

#include<cmath>
#include"crf.hpp"
#include"speech_synthesis.hpp"

typedef CRandomField<LabelAlphabet> CRF;

template<class Evaluate>
class StateFunction : public CRF::StateFunction {
public:
  double operator()(const Sequence<Label>& labels, int pos, const Sequence<Input>& inputs) const {
    const PhonemeInstance& prev = alphabet->fromInt(labels[pos]);
    const PhonemeInstance& next = alphabet->fromInt(inputs[pos]);
    Evaluate eval;
    return eval(prev, next);
  }

  double operator()(const Label l1, int pos, const Sequence<Input>& inputs) const {
    const PhonemeInstance& prev = alphabet->fromInt(l1);
    const PhonemeInstance& next = alphabet->fromInt(inputs[pos]);
    Evaluate eval;
    return eval(prev, next);
  }
};

template<class Evaluate>
class TransitionFunction : public CRF::TransitionFunction {
public:
  double operator()(const Sequence<Label>& labels, int pos, const Sequence<Input>&) const {
    const PhonemeInstance& prev = alphabet->fromInt(labels[pos] - 1);
    const PhonemeInstance& next = alphabet->fromInt(labels[pos]);
    Evaluate eval;
    return eval(prev, next);
  }

  double operator()(const Label l1, const Label l2, int, const Sequence<Input>&) const {
    const PhonemeInstance& prev = alphabet->fromInt(l1);
    const PhonemeInstance& next = alphabet->fromInt(l2);
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
    double d1 = prev.pitch[prev.pitch.length - 1];
    double d2 = next.pitch[0];
    return std::abs(d1 - d2);
  }
};

struct MFCCDist {
  double operator()(const PhonemeInstance& prev, const PhonemeInstance& next) const {
    MfccArray* mfcc1 = prev.mfcc;
    MfccArray* mfcc2 = next.mfcc;
    double result = 0;
    for(int i = 0; i < mfcc1->length(); i++) {
      double diff = (*mfcc1)[i] - (*mfcc2)[i];
      result += diff * diff;
    }
    return result;
  }
};

Sequence<CRF::StateFunction*> state_functions() {
  Sequence<CRF::StateFunction*> result(0);
  return result;
}

Sequence<CRF::TransitionFunction*> transition_functions() {
  Sequence<CRF::TransitionFunction*> result(2);
  //result[0] = new TransitionFunction<Duration>();
  result[1] = new TransitionFunction<Pitch>();
  result[2] = new TransitionFunction<MFCCDist>();
  return result;
}

#endif


