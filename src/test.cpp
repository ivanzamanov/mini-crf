#include<iostream>
#include<fstream>

#include"crf/crf.hpp"
#include"crf/training.hpp"
#include"crf/speech_synthesis.hpp"

struct TestAlphabet {
  TestAlphabet() {
    phonemes.length = 4;
    phonemes.data = new int[4]{1,2,3,4};
  }

  bool allowedTransition(int l1, int l2) const {
    return true;
  }
  
  Array<int> phonemes;
};

typedef CRandomField<TestAlphabet> CRF;

class TestTransFunc : public CRF::TransitionFunction {
public:
  virtual double operator()(const Sequence<Label>&, int, const Sequence<Input>&) const {
    return 1;
  }
  virtual double operator()(const Label, const Label, const int, const Sequence<Input>&) const {
    return 1;
  };
};

Sequence<CRF::StateFunction> state_functions() {
  Sequence<CRF::StateFunction> result(0);
  return result;
}

Sequence<CRF::TransitionFunction> transition_functions() {
  Sequence<CRF::TransitionFunction> result(1);
  result[0] = TestTransFunc();
  return result;
}

int main() {
  CRF crf(state_functions(), transition_functions());
  Sequence<Input> x(5);
  for(int i = 0; i < 5; i++) x[i] = i;
  CoefSequence lambda(1);
  lambda[0] = 1;
  CoefSequence mu(0);

  std::cout << norm_factor(x, crf, lambda, mu) << std::endl;
}
