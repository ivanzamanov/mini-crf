#include"crf/speech_synthesis.hpp"

static void print_usage(const char* main) {
  std::cout << "Usage: " << main << ": [<input_file>|-] <output_file>\n"; 
}

Corpus corpus;
PhonemeAlphabet alphabet;

void transfer_data(std::istream& input, std::ofstream *output) {
  build_data_txt(input, &alphabet, &corpus);
  BinaryWriter w(output);

  std::cout << "Writing data" << std::endl;
  w << alphabet.size();
  for(unsigned i = 0; i < alphabet.size(); i++)
    w << alphabet.fromInt(i) << alphabet.file_indices[i];
  std::cout << "Written phonemes, " << w.bytes << " bytes\n";

  w << alphabet.files.length;
  for(unsigned i = 0; i < alphabet.files.length; i++) {
    std::string &file = alphabet.files[i];
    unsigned len = file.length();
    w << len;
    for(unsigned j = 0; j < len; j++)
      w << file[j];
  }
  std::cout << "Written file names, " << w.bytes << " bytes\n";

  unsigned len;
  len = corpus.size();
  w << len;
  for(unsigned i = 0; i < corpus.size(); i++) {
    vector<Input> input = corpus.input(i);
    vector<Label> labels = corpus.label(i);
    len = input.size();
    w << len;
    for(unsigned j = 0; j < input.size(); j++)
      w << input[j];

    len = labels.size();
    w << len;
    for(unsigned j = 0; j < labels.size(); j++)
      w << labels[j];
  }

  std::cout << "Written " << w.bytes << " bytes" << std::endl;
}

void compare_corpus(Corpus& c1, Corpus& c2) {
  std::cout << "Comparing corpuses\n";
  bool same = c1.size() == c2.size();
  for(unsigned i = 0; i < c1.size(); i++) {
    vector<Input> i1 = c1.input(i);
    vector<Input> i2 = c2.input(i);

    vector<Label> l1 = c1.label(i);
    vector<Label> l2 = c2.label(i);
    same &= compare(i1, i2);
    same &= compare(l1, l2);
  }
  std::cout << "Same " << same << std::endl;
}

void compare_alphabet(PhonemeAlphabet& a1, PhonemeAlphabet& a2) {
  bool same = compare(a1.classes, a2.classes);
  same &= compare(a1.labels, a2.labels);
  same &= compare(a1.file_indices, a2.file_indices);
  same &= compare(a1.files, a2.files);

  std::cout << "Same alphabets: " << same << std::endl;
}

void validate_data(std::ifstream& input) {
  Corpus n_corpus;
  PhonemeAlphabet n_alphabet;

  std::cout << "Validating data" << std::endl;

  build_data_bin(input, n_alphabet, n_corpus);
  compare_corpus(corpus, n_corpus);
  compare_alphabet(alphabet, n_alphabet);
}

int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);

  if(argc < 3) {
    print_usage(argv[0]);
    return 1;
  }

  std::string input_path(argv[1]);
  std::string output_path(argv[2]);

  std::istream* input;
  bool is_stdin = input_path == "-";
  if(is_stdin) {
    input = &std::cin;
  } else {
    input = new std::ifstream(input_path);
  }
  std::ofstream output(output_path);

  transfer_data(*input, &output);
  output.close();
  std::ifstream bin_input(output_path);
  validate_data(bin_input);

  if(is_stdin)
    delete input;
}
