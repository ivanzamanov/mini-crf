#include<fstream>
#include<sstream>

#include"crf/crf.hpp"
#include"crf/features.hpp"
#include"crf/training.hpp"
#include"crf/speech_synthesis.hpp"

const char* const files_list = "/home/ivo/corpus-features/files-list";

void build_data(LabelAlphabet* alphabet, Corpus& corpus) {
  std::ifstream stream(files_list);
  build_data(stream, alphabet, &corpus);
}

Sequence<Input> to_sequence(LabelAlphabet& alphabet, const char* str) {
  int length = strlen(str);
  Sequence<Input> result(length);
  std::cerr << "Input phoneme ids: ";
  for(int i = 0; i < length; i++) {
    result[i] = alphabet.first_by_label(str[i]);
    std::cerr << result[i] << " ";
  }
  std::cerr << std::endl;
  return result;
}

void from_path(LabelAlphabet& alphabet, vector<int>& path) {
  std::stringstream phonemeIds;
  for(auto it = path.begin(); it != path.end(); it++) {
    PhonemeInstance* phon = &alphabet.phonemes[*it];
    phonemeIds << *it << "=" << phon->label << " ";
    std::string file = alphabet.file_of(*it);
    std::cout << "File=" << file << " ";
    std::cout << "Start=" << phon->start << " ";
    std::cout << "End=" << phon->end << '\n';
  }
  std::cerr << phonemeIds.str() << std::endl;
}

int main(int argc, const char** argv) {
  if(argc < 2) {
    std::cerr << "Usage: <sentence>" << std::endl;
    return 1;
  }

  std::ios_base::sync_with_stdio(false);
  const char* input = argv[1];
  std::cerr << "Sentence: " << input << std::endl;
  CRF crf(state_functions(), transition_functions());
  Corpus corpus;
  build_data(&crf.label_alphabet, corpus);

  Sequence<Input> phoneme_sequence = to_sequence(crf.label_alphabet, input);

  std::cerr << "Alphabet size: " << crf.label_alphabet.phonemes.length << std::endl;
  std::cerr << "Corpus size: " << corpus.length() << std::endl;

  std::vector<int> path;
  max_path(phoneme_sequence, crf, crf.lambda, crf.mu, &path);
  from_path(crf.label_alphabet, path);
  //trainGradientDescent<CRF>(crf, corpus);
  return 0;
}
