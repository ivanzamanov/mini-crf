#include<iostream>
#include<fstream>

#include"crf/crf.hpp"
#include"crf/features.hpp"
#include"crf/training.hpp"
#include"crf/speech_synthesis.hpp"

const char* const test_file = "/home/ivo/corpus-features/files-list";
int main() {
  CRF crf(state_functions(), transition_functions());

  std::ifstream stream(test_file);
  Corpus* corpus = new Corpus();

  build_data(stream, &crf.label_alphabet, corpus);

  std::cout << "Size: " << crf.label_alphabet.phonemes.length << std::endl;
  std::cout << "Size: " << corpus->length() << std::endl;

  trainGradientDescent<CRF>(crf, *corpus);
  return 0;
}
