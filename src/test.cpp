#include<iostream>
#include<fstream>

#include"crf/crf.hpp"
#include"crf/training.hpp"
#include"crf/speech_synthesis.hpp"
#include"crf/util.hpp"

struct TestAlphabet {
  TestAlphabet() {
    phonemes.length = 4;
    phonemes.data = new int[4]{1,2,3,4};
  }

  bool allowedTransition(int l1, int l2) const {
    return true;
  }

  bool allowedState(int l1, int l2) const {
    return true;
  }

  Array<int> phonemes;
};

typedef CRandomField<TestAlphabet> TestCRF;

class TestTransFunc : public TestCRF::TransitionFunction {
public:
  virtual double operator()(const Sequence<Label>&, int, const Sequence<Input>&) const {
    return 0;
  }
  virtual double operator()(const Label l1, const Label l2, const int, const Sequence<Input>&) const {
    return l1 == l2;
  };
};

class TestStateFunc : public TestCRF::StateFunction {
public:
  virtual double operator()(const Sequence<Label>&, int pos, const Sequence<Input>&) const {
    return pos == 0;
  }
  virtual double operator()(const Label, int pos, const Sequence<Input>&) const {
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

template<class T>
void assertEquals(std::string msg, T expected, T actual) {
  if(expected != actual) {
    std::cerr << "Expected " << expected << ", actual " << actual << std::endl;
    throw std::string("Assert failed ") + msg;
  }
}

template<class T>
void assertEquals(T expected, T actual) {
  assertEquals(std::string(""), expected, actual);
}

void testUtils() {
  std::cerr << "Multiplication: " << util::mult(-0.0001d, 0.0001d) << std::endl;
  std::cerr << "ExpMultiplication: " << util::mult_exp(0, -0.0001d) << std::endl;
  
  std::cerr << "Sum same sign: " << util::sum(0.02d, 0.01d) << std::endl; // 0.03
  std::cerr << "Sum same sign: " << util::sum(0.01d, 0.02d) << std::endl; // 0.03
  std::cerr << "Sum same sign neg: " << util::sum(-0.02d, -0.01d) << std::endl; // 0.03
  std::cerr << "Sum same sign neg: " << util::sum(-0.01d, -0.02d) << std::endl; // 0.03
  
  std::cerr << "Sum diff sign: " << util::sum(-0.02d, 0.01d) << std::endl; // -0.01
  std::cerr << "Sum diff sign: " << util::sum(-0.01d, 0.02d) << std::endl; // 0.01
  std::cerr << "Sum diff sign: " << util::sum(0.02d, -0.01d) << std::endl; // 0.01
  std::cerr << "Sum diff sign: " << util::sum(-0.01d, 0.02d) << std::endl; // 0.01
  std::cerr << "Sum diff sign: " << util::sum(0.01d, -0.02d) << std::endl; // -0.01
}

void testCRF() {
  TestCRF crf(state_functions(), transition_functions());
  Sequence<Input> x(5);
  for(int i = 0; i < 5; i++) x[i] = i;
  CoefSequence lambda(1);
  lambda[0] = 1;
  CoefSequence mu(1);
  mu[0] = 1;

  std::vector<int> path;
  max_path(x, crf, lambda, mu, &path);

  for(unsigned i = 0; i < path.size(); i++)
    std::cout << path[i] << " ";
  std::cout << std::endl;

  assertEquals(2048.0, norm_factor(x, crf, lambda, mu));
}

int main() {
  try {
    testUtils();
    testCRF();
  } catch (std::string s) {
    std::cerr << s << std::endl;
  }
}
