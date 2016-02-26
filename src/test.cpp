#include<fstream>
#include<sstream>
#include<cstdlib>

#include"gridsearch.hpp"
#include"parser.hpp"
#include"speech_synthesis.hpp"
#include"crf.hpp"

struct TestObject {
  TestObject() { };
  explicit TestObject(int l): label(l) { }
  int label = 0;
  int id = 0;
};

static const int CLASSES = 3;

struct TestAlphabet : LabelAlphabet<TestObject> {
  TestAlphabet() {
    labels.resize(size());
    for(int i = 0; i < size(); i++) {
      labels[i].label = i % CLASSES;
      labels[i].id = i;
    }
    build_classes();
  }

  const LabelClass& get_class(unsigned int integer) const {
    return classes[integer % CLASSES];
  }

  bool allowedState(const Label& l1, const Label& l2) const {
    return l1.label % CLASSES == l2.label % CLASSES;
  }

  int size() const {
    return 9;
  }
};

struct TestTransFunc {
  static const bool is_state = true;
  cost operator()(int x,
                  const TestObject& y) {
    return (x == y.id) ? -1 : 1;
  }
};

struct TestFeatures {
  static constexpr auto Functions =
    std::make_tuple(TestTransFunc{});

  static constexpr auto Names =
    std::make_tuple("size");

  static constexpr auto size = std::tuple_size<decltype(Functions)>::value;
};
typedef CRandomField<TestAlphabet, int, TestFeatures> TestCRF;


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
  std::cerr << "Multiplication: " << util::mult(-0.0001, 0.0001) << std::endl;
  std::cerr << "ExpMultiplication: " << util::mult_exp(0, -0.0001) << std::endl;
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

void test_path(TestCRF* crf, const vector<TestCRF::Label>& labels) {
  vector<int> x;
  for(auto& label : labels)
    x.push_back(label.id);

  vector<int> best_path, second_best_path;
  std::cerr << "Input path: ";
  for(unsigned i = 0; i < labels.size(); i++)
    std::cerr << x[i] << ' ';
  std::cerr << std::endl;

  cost expected_cost = concat_cost(labels, *crf, crf->lambda, x);
  cost cost = traverse_automaton<MinPathFindFunctions>(x, *crf, crf->lambda,
                                                       &best_path,
                                                       &second_best_path);

  std::cerr << "Cost = " << cost << ", path = ";
  for(auto i : best_path)
    std::cerr << i << ' ';
  std::cerr  << std::endl;

  std::cerr << "Second best path = ";
  for(auto i : second_best_path)
    std::cerr << i << ' ';
  std::cerr  << std::endl;

  assertEquals(0.0f - x.size(), expected_cost);
  assertEquals(expected_cost, cost);
  assertEquals(labels.size(), best_path.size());
  for(unsigned i = 0; i < labels.size(); i++) {
    assertEquals(labels[i].id, best_path[i]);
    assertEquals(labels[i].label % CLASSES, best_path[i] % CLASSES);
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

  TestCRF crf;
  crf.label_alphabet = new TestAlphabet();
  TestCRF::Values lambda;
  lambda[0] = 1.0;

  crf.lambda = lambda;

  vector<TestObject> x;
  for(int i = 0; i < crf.alphabet().size(); i++)
    x.push_back(TestObject(i));

  TestFilter filter;
  filter.crf = &crf;
  crf.alphabet().iterate_sequences(x, filter);
}

void testCrfSecondBestPath() {
  TestCRF crf;
  crf.label_alphabet = new TestAlphabet();
  crf.lambda = {{1.0f}};

  vector<int> x{0, 1, 2};

  vector<int> best_path, second_best_path;
  cost cost = traverse_automaton<MinPathFindFunctions>(x, crf, crf.lambda,
                                                       &best_path,
                                                       &second_best_path);
  assertEquals(0.0f - x.size(), cost);
  assertEquals(0.0f - x.size(), cost);
  std::cerr << "Second best path = ";
  for(auto i : second_best_path)
    std::cerr << i << ' ';
  std::cerr  << std::endl;
}

void testCrfPathLength1() {
  TestCRF crf;
  crf.label_alphabet = new TestAlphabet();
  crf.lambda = {{1.0f}};
  vector<int> x{0};

  vector<int> best_path, second_best_path;
  cost cost = traverse_automaton<MinPathFindFunctions>(x, crf, crf.lambda,
                                                       &best_path, &second_best_path);
  assertEquals(-1.0f, cost);
  assertEquals(x[0], best_path[0]);
  std::cerr << second_best_path[0] << std::endl;
}

void testSynthInputCSV() {
  const std::string csv("id,duration,pitch\np,0.5,251.12");
  std::stringstream csv_str(csv);
  vector<PhonemeInstance> phons = parse_synth_input_csv(csv_str);

  //assertEquals('p', phons[0].label);
  assertEquals(0.5f, phons[0].duration);
  assertEquals(251.12f, phons[0].first().pitch);

  csv_str.seekp(0); csv_str.seekg(0);
  print_synth_input_csv(csv_str, phons);
  assertEquals(csv, csv_str.str());
}

bool Progress::enabled = true;
std::string gridsearch::Comparisons::metric = "";
std::string gridsearch::Comparisons::aggregate = "";

int main() {
  try {
    testUtils();
    testCrfPathLength1();
    //testCRF();
    testCrfSecondBestPath();
    testSynthInputCSV();

    std::cout << "All tests passed\n";
  } catch (std::string s) {
    std::cerr << s << std::endl;
    return 1;
  }
}
