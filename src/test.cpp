#include<iostream>
#include<fstream>
#include<sstream>
#include<cstdlib>

#include"praat/parser.hpp"
#include"crf/speech_synthesis.hpp"
#include"crf/crf.hpp"

struct TestObject {
  TestObject() { };
  TestObject(int l): label(l) { }
  int label = 0;
};

static const int CLASSES = 3;

struct TestAlphabet : LabelAlphabet<TestObject> {
  TestAlphabet() {
    labels.data = new TestObject[size()];
    for(int i = 0; i < size(); i++)
      labels[i].label = i % CLASSES;
    labels.length = size();
    build_classes();
  }

  bool allowedState(const Label& l1, const Label& l2) const {
    return l1.label % CLASSES == l2.label % CLASSES;
  }

  int size() const {
    return 9;
  }
};

typedef CRandomField<TestAlphabet, int> TestCRF;

class TestTransFunc : public TestCRF::TransitionFunction {
public:
  vector<int> best_path;

  TestTransFunc(): best_path() { }
  
  TestTransFunc(vector<int> best_path): best_path(best_path) { }

  virtual double operator()(const vector<TestCRF::Label>& labels, int pos, const vector<TestCRF::Input>&) const {
    if(best_path[pos - 1] == labels[pos - 1].label && best_path[pos] == labels[pos].label)
      return -1;
    else
      return rand();
  }

  virtual double operator()(const TestCRF::Label& l1, const TestCRF::Label l2, const int pos, const vector<TestCRF::Input>&) const {
    if(best_path[pos - 1] == l1.label && best_path[pos] == l2.label)
      return -1;
    else
      return rand();
  };
};

class TestStateFunc : public TestCRF::StateFunction {
public:
  vector<int> best_path;
  
  TestStateFunc(): best_path() { }
  TestStateFunc(vector<int> best_path): best_path(best_path) { }

  virtual double operator()(const vector<TestCRF::Label>& labels, int pos, const vector<TestCRF::Input>&) const {
    if(best_path[pos] == labels[pos].label)
      return -1;
    else
      return rand();
  }

  virtual double operator()(const TestCRF::Label label, int pos, const vector<TestCRF::Input>&) const {
    if(best_path[pos] == label.label)
      return -1;
    else
      return rand();
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
  std::cerr << "Sum same sign neg: " << util::sum(-0.02d, -0.01d) << std::endl; // -0.03
  std::cerr << "Sum same sign neg: " << util::sum(-0.01d, -0.02d) << std::endl; // -0.03
  
  std::cerr << "Sum diff sign: " << util::sum(-0.02d, 0.01d) << std::endl; // -0.01
  std::cerr << "Sum diff sign: " << util::sum(-0.01d, 0.02d) << std::endl; // 0.01
  std::cerr << "Sum diff sign: " << util::sum(0.02d, -0.01d) << std::endl; // 0.01
  std::cerr << "Sum diff sign: " << util::sum(-0.01d, 0.02d) << std::endl; // 0.01
  std::cerr << "Sum diff sign: " << util::sum(0.01d, -0.02d) << std::endl; // -0.01
  */
}

vector<int> to_ints(const vector<TestCRF::Label>& labels) {
  vector<int> result;
  for(auto it = labels.begin(); it != labels.end(); it++)
    result.push_back( (*it).label);
  return result;
}

void test_path(TestCRF* crf, const vector<TestCRF::Label>& labels) {
  ((TestTransFunc*) crf->f[0])->best_path = to_ints(labels);
  ((TestStateFunc*) crf->g[0])->best_path = to_ints(labels);

  FunctionalAutomaton<TestCRF> a(crf->label_alphabet);
  a.lambda = crf->lambda;
  a.mu = crf->mu;
  a.f = crf->f;
  a.g = crf->g;
  a.x = to_ints(labels);

  vector<int> best_path;

  /*std::cerr << "Input path: ";
  for(unsigned i = 0; i < labels.size(); i++)
    std::cerr << labels[i] << " ";
  std::cerr << std::endl;*/

  a.traverse(&best_path);

  /*std::cerr << "Output path: ";
  for(unsigned i = 0; i < best_path.size(); i++)
    std::cerr << best_path[i] << " ";
  std::cerr << std::endl;*/

  assertEquals(labels.size(), best_path.size());
  for(unsigned i = 0; i < labels.size(); i++) {
    assertEquals(best_path[i], labels[i].label);
    assertEquals(best_path[i] % CLASSES, labels[i].label % CLASSES);
  }
}

struct TestFilter {
  TestCRF *crf;

  void operator()(const vector<TestCRF::Label>& labels) {
    test_path(crf, labels);
  }
};

void testCRF() {
  srand(5);

  TestCRF crf(state_functions(), transition_functions());
  std::vector<double> lambda;
  lambda.push_back(1.0);
  std::vector<double> mu;
  mu.push_back(1.0);

  vector<TestObject> x;
  for(int i = 0; i < crf.label_alphabet.size(); i++)
    x.push_back(i);

  TestFilter filter;
  filter.crf = &crf;
  crf.label_alphabet.iterate_sequences(x, filter);
}

void testSynthInputCSV() {
  const std::string csv("id,duration,pitch\np,0.5,251.12");
  std::stringstream csv_str(csv);
  vector<PhonemeInstance> phons = parse_synth_input_csv(csv_str);

  assertEquals('p', phons[0].label);
  assertEquals(0.5, phons[0].duration());
  assertEquals(251.12, phons[0].first().pitch);

  csv_str.seekp(0); csv_str.seekg(0);
  print_synth_input_csv(csv_str, phons);
  assertEquals(csv, csv_str.str());
}

int main() {
  try {
    testUtils();
    testCRF();
    testSynthInputCSV();

    std::cout << "All tests passed\n";
  } catch (std::string s) {
    std::cerr << s << std::endl;
  }
}
