#include<fstream>

#include"crf/crf.hpp"
#include"crf/features.hpp"
#include"crf/training.hpp"
#include"crf/speech_synthesis.hpp"

void build_data(PhonemeAlphabet* alphabet, Corpus& corpus) {
  std::ifstream stream("db.bin");
  build_data_bin(stream, *alphabet, corpus);
}

std::vector<Input> to_sequence(PhonemeAlphabet& alphabet, const char* str) {
  int length = strlen(str);
  std::vector<Input> result(length);
  std::cerr << "Input phoneme ids: ";
  for(int i = 0; i < length; i++) {
    result[i] = alphabet.first_by_label(str[i]);
    std::cerr << result[i] << " ";
  }
  std::cerr << std::endl;
  return result;
}

int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);
  if(argc < 2) {
    std::cerr << "Usage: <sentence>" << std::endl;
    return 1;
  }

  const char* input = argv[1];
  std::cerr << "Sentence: " << input << std::endl;
  CRF crf(state_functions(), transition_functions());
  Corpus corpus;
  build_data(&crf.label_alphabet, corpus);

  std::cerr << "Alphabet size: " << crf.label_alphabet.size() << std::endl;
  std::cerr << "Corpus size: " << corpus.size() << std::endl;

  std::vector<Input> phoneme_sequence = to_sequence(crf.label_alphabet, input);

  std::vector<int> path;
  max_path(phoneme_sequence, crf, crf.lambda, crf.mu, &path);
  crf.label_alphabet.print_synth(path);
  //trainGradientDescent<CRF>(crf, corpus);
  return 0;
}
