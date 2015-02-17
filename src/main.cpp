#include<iostream>
#include<fstream>

#include"crf/crf.hpp"
#include"crf/training.hpp"
#include"speech_synthesis.hpp"

typedef CRandomField<LabelAlphabet> CRF;

struct TestFilter {
  void operator()(const Sequence<Label>& labels) {
    for(int i = 0; i < labels.length(); i++) {
      std::cout << labels[i] << ' ';
    }
    std::cout << std::endl;
  }
};

const char* const test_file = "/home/ivo/corpus-features/files-list";
int main() {
  std::ifstream stream(test_file);
  LabelAlphabet* alphabet = build_alphabet(stream);
  std::cout << "Size: " << alphabet->phonemes.length << std::endl;
  // Sequence<StateFunction> state_functions(1);
  // Sequence<TransitionFunction> transition_functions(1);

  // CRF crf(state_functions, transition_functions);
  // const int N = 10;
  // Sequence<int> input(N);
  // Sequence<int> labels(N);
  // for (int i = 0; i < N; i++) {
  //   input[i] = i;
  //   labels[i] = i;
  // }
  // Corpus corpus;
  // corpus.add(input, labels);

  Sequence<int> inpt(5);
  inpt[0] = 95;
  inpt[1] = 97;
  inpt[2] = 98;
  inpt[3] = 99;
  inpt[4] = 100;
  TestFilter test_filter;
  alphabet->iterate_sequences(inpt, test_filter);

  // trainGradientDescent<CRF>(crf, corpus);
  return 0;
}
