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

typedef CRandomField<TestAlphabet> TestCRF;

class TestTransFunc : public TestCRF::TransitionFunction {
public:
  virtual double operator()(const Sequence<Label>&, int, const Sequence<Input>&) const {
    return 1;
  }
  virtual double operator()(const Label, const Label, const int, const Sequence<Input>&) const {
    return 1;
  };
};

class TestStateFunc : public TestCRF::StateFunction {
public:
  virtual double operator()(const Sequence<Label>& labelSeq, int pos, const Sequence<Input>& seq) const {
    return pos == 0;
  }
  virtual double operator()(const Label l, int pos, const Sequence<Input>& seq) const {
    return pos == 0;
  };
};

Sequence<TestCRF::StateFunction*> state_functions() {
  Sequence<TestCRF::StateFunction*> result(1);
  result[0] = new TestStateFunc();
  return result;
}

Sequence<TestCRF::TransitionFunction*> transition_functions() {
  Sequence<TestCRF::TransitionFunction*> result(1);
  result[0] = new TestTransFunc();
  return result;
}

int main() {
  TestCRF crf(state_functions(), transition_functions());
  Sequence<Input> x(5);
  for(int i = 0; i < 5; i++) x[i] = i;
  CoefSequence lambda(1);
  lambda[0] = 1;
  CoefSequence mu(1);
  mu[0] = 1;

  std::cout << norm_factor(x, crf, lambda, mu) << std::endl;
}
