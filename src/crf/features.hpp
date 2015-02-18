#ifndef __FEATURES_H__
#define __FEATURES_H__

#include"crf.hpp"
#include"speech_synthesis.hpp"

typedef CRandomField<LabelAlphabet> CRF;

template<class Evaluate>
class TransitionFunction : public CRF::TransitionFunction {
  virtual double operator()(const Sequence<Label> labels, int pos, const Sequence<Input>& inputs) {
    const PhonemeInstance& prev = alphabet->fromInt(labels[pos] - 1);
    const PhonemeInstance& next = alphabet->fromInt(labels[pos]);
    Evaluate eval;
    return eval(prev, next);
  }
};

struct Duration {
  double operator()(const PhonemeInstance& prev, const PhonemeInstance& next) {
    double d1 = prev.end - prev.start;
    double d2 = next.end - next.start;
    return d1 - d2;
  }
};

Sequence<CRF::StateFunction> state_functions() {
  Sequence<CRF::StateFunction> result(0);
  return result;
}

Sequence<CRF::TransitionFunction> transition_functions() {
  Sequence<CRF::TransitionFunction> result(1);
  result[0] = TransitionFunction<Duration>();
  return result;
}

#endif
