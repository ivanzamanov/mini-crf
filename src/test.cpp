#include<fstream>
#include<sstream>
#include<cstdlib>

#include"gridsearch.hpp"
#include"parser.hpp"
#include"speech_synthesis.hpp"
#include"crf.hpp"

using namespace gridsearch;

struct TestObject {
  TestObject() { };
  explicit TestObject(int l): label(l), id(l) { }
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

struct TestStateFunc {
  static const bool is_state = true;
  cost operator()(int x,
                  const TestObject& y) const {
    return (x == y.id) ? -1 : 1;
  }
};

struct TestFeatures {
  static constexpr auto Functions =
    std::make_tuple(TestStateFunc{});

  static constexpr auto Names =
    std::make_tuple("size");

  static constexpr auto size = std::tuple_size<decltype(Functions)>::value;
};
typedef CRandomField<TestAlphabet, int, TestFeatures> TestCRF;


template<class T>
void assertEquals(std::string msg, T expected, T actual) {
  if(expected != actual) {
    ERROR("Expected " << expected << ", actual " << actual);
    ERROR("Assert failed: " + msg); assert(false);
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

void verifyPath(const vector<TestObject>& labels, const vector<int>& path) {
  assertEquals("Path size", labels.size(), path.size());
  for(unsigned i = 0; i < labels.size(); i++) {
    assertEquals("Path member", labels[i].id, path[i]);
    assertEquals("Path member", labels[i].label % CLASSES, path[i] % CLASSES);
  }
}

void verifyPath(const vector<int>& x, const vector<int>& path) {
  vector<TestObject> labels;
  for(auto l : x)
    labels.push_back(TestObject(l));
  verifyPath(labels, path);
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

  cost conc_cost = concat_cost(labels, *crf, crf->lambda, x);
  cost cost = traverse_automaton<MinPathFindFunctions>(x, *crf, crf->lambda,
                                                       &best_path)[0];
  assertEquals("Traverse cost", 0.0 - x.size(), conc_cost);
  assertEquals("Concat cost", conc_cost, cost);
  verifyPath(labels, best_path);
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
  crf.lambda = {{1.0f}};

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

  vector<int> x{0, 1};

  vector<int> best_path;
  auto costs = traverse_automaton<MinPathFindFunctions, TestCRF, 2>(x, crf,
                                                                    crf.lambda,
                                                                    &best_path);
  assertEquals("Cost", 0.0 - x.size(), costs[0]);
  verifyPath(x, best_path);
  assertEquals("Cost 2", 0.0 - x.size() + 1, costs[1]);
  std::cerr  << std::endl;
}

void testCrfPathLength1() {
  TestCRF crf;
  crf.label_alphabet = new TestAlphabet();
  crf.lambda = {{1.0f}};
  vector<int> x{0};

  vector<int> best_path;
  auto costs = traverse_automaton<MinPathFindFunctions>(x, crf, crf.lambda,
                                                        &best_path);
  assertEquals("Cost", -1.0, costs[0]);
  assertEquals("Element", x[0], best_path[0]);
}

extern void printGridPoint(std::string file, const Params& params, const TrainingOutputs& result);
extern GridPoints parseGridPoints(std::string file);

void testGridPrint() {
  Params params = ParamsFactory::make();
  for(auto i = 0u; i < params.size(); i++)
    params[i] = i;
  TrainingOutputs result;
  TrainingOutput to1, to2;
  to1.path = {{ 10, 20, 30}};
  to2.path = {{ 40, 50, 60}};

  result.resize(2);
  result[0] = to1;
  result[1] = to2;

  std::string file = "test-path.txt";
  std::ofstream(file).close();
  printGridPoint(file, params, result);
  printGridPoint(file, params, result);
  auto points = parseGridPoints(file);
  assertEquals(2ul, points.size());

  for(auto& pair : points) {
    Params checkParams = pair.first;
    TrainingOutputs checkResult = pair.second;
    for(auto i = 0u; i < params.size(); i++)
      assertEquals(params[i], checkParams[i]);

    assertEquals(result.size(), checkResult.size());
    for(auto i = 0u; i < result.size(); i++) {
      assertEquals(result[i].path.size(), checkResult[i].path.size());
      for(auto j = 0u; j < result[i].path.size(); j++) {
        assertEquals(result[i].path[j], checkResult[i].path[j]);
      }
    }
  }
}

bool Progress::enabled = true;
int main() {
  try {
    testGridPrint();
    testUtils();
    testCrfPathLength1();
    testCrfSecondBestPath();
    testCRF();

    std::cout << "All tests passed\n";
  } catch (std::string s) {
    std::cerr << s << std::endl;
    return 1;
  }
}
