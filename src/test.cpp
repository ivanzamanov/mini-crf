#include<iostream>
#include<fstream>

#include"crf/crf.hpp"

struct TestAlphabet {
  TestAlphabet() { }

  bool allowedState(int l1, int l2) const {
    return true;
  }

  int size() const {
    return 4;
  }
};

typedef CRandomField<TestAlphabet> TestCRF;

class TestTransFunc : public TestCRF::TransitionFunction {
public:
  vector<int> best_path;

  TestTransFunc(): best_path() { }
  
  TestTransFunc(vector<int> best_path): best_path(best_path) { }

  virtual double operator()(const vector<Label>& labels, int pos, const vector<Input>&) const {
    return best_path[pos - 1] == labels[pos - 1] && best_path[pos] == labels[pos];
  }

  virtual double operator()(const Label l1, const Label l2, const int pos, const vector<Input>&) const {
    return best_path[pos - 1] == l1 && best_path[pos] == l2;
  };
};

class TestStateFunc : public TestCRF::StateFunction {
public:
  vector<int> best_path;
  
  TestStateFunc(): best_path() { }
  TestStateFunc(vector<int> best_path): best_path(best_path) { }

  virtual double operator()(const vector<Label>& labels, int pos, const vector<Input>&) const {
    return best_path[pos] == labels[pos];
  }

  virtual double operator()(const Label label, int pos, const vector<Input>&) const {
    return best_path[pos] == label;
  };
};

vector<TestCRF::StateFunction*> state_functions() {
  vector<TestCRF::StateFunction*> result;
  result.push_back(new TestStateFunc());
  return result;
}

vector<TestCRF::TransitionFunction*> transition_functions() {
  vector<TestCRF::TransitionFunction*> result;
  result.push_back(new TestTransFunc());
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

  /*
  std::cerr << "Sum same sign: " << util::sum(0.02d, 0.01d) << std::endl; // 0.03
  std::cerr << "Sum same sign: " << util::sum(0.01d, 0.02d) << std::endl; // 0.03
  std::cerr << "Sum same sign neg: " << util::sum(-0.02d, -0.01d) << std::endl; // 0.03
  std::cerr << "Sum same sign neg: " << util::sum(-0.01d, -0.02d) << std::endl; // 0.03
  
  std::cerr << "Sum diff sign: " << util::sum(-0.02d, 0.01d) << std::endl; // -0.01
  std::cerr << "Sum diff sign: " << util::sum(-0.01d, 0.02d) << std::endl; // 0.01
  std::cerr << "Sum diff sign: " << util::sum(0.02d, -0.01d) << std::endl; // 0.01
  std::cerr << "Sum diff sign: " << util::sum(-0.01d, 0.02d) << std::endl; // 0.01
  std::cerr << "Sum diff sign: " << util::sum(0.01d, -0.02d) << std::endl; // -0.01
  */
}

void testCRF() {
  TestCRF crf(state_functions(), transition_functions());
  vector<Input> x;
  for(int i = 0; i < 5; i++) x.push_back(i);
  std::vector<double> lambda;
  lambda.push_back(1.0);
  std::vector<double> mu;
  mu.push_back(1.0);

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
