#include<iostream>
#include<fstream>

#include"crf/crf.hpp"
#include"crf/training.hpp"
#include"speech_synthesis.hpp"

typedef CRandomField<LabelAlphabet> CRF;

struct TestFilter {
  void operator()(const Sequence<int>& labels) {
    // for(int i = 0; i < labels.length(); i++) {
    //   std::cout << labels[i] << ' ';
    // }
    // std::cout << std::endl;
  }
};

const char* const test_file = "/home/ivo/praat-tmp/output.txt";
int main() {
  int size;
  std::ifstream stream(test_file);
  parse_file(stream, size);
  std::cout << "Size: " << size << std::endl;
  Sequence<StateFunction> state_functions(1);
  Sequence<TransitionFunction> transition_functions(1);

  CRF crf(state_functions, transition_functions);
  const int N = 10;
  Sequence<int> input(N);
  Sequence<int> labels(N);
  for (int i = 0; i < N; i++) {
    input[i] = i;
    labels[i] = i;
  }
  Corpus corpus;
  corpus.add(input, labels);

  Sequence<int> inpt(5);
  for(int i = 0; i < inpt.length(); i++) {
    inpt[i] = i * 2;
  }
  LabelAlphabet alphabet;
  TestFilter test_filter;
  alphabet.iterate_sequences(inpt, test_filter);

  trainGradientDescent<CRF>(crf, corpus);
  return 0;
}
