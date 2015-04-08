#include<iostream>
#include<fstream>

#include"crf/speech_synthesis.hpp"

static void print_usage(const char* main) {
  std::cout << "Usage: " << main << ": [<input_file>|-] <output_file>\n"; 
}

void transfer_data(std::istream& input, std::ofstream& output);

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
  std::ofstream output(output_path, std::ios::binary);

  transfer_data(*input, output);

  if(is_stdin)
    delete input;
}

void transfer_data(std::istream& input, std::ofstream& output) {
  Corpus corpus;
  PhonemeAlphabet alphabet;
  build_data(input, &alphabet, &corpus);

  output << alphabet.size();
  for(unsigned i = 0; i < alphabet.size(); i++) {
    output << alphabet.fromInt(i);
    output << alphabet.file_indices[i];
  }

  for(unsigned i = 0; i < alphabet.files.length; i++) {
    std::string &file = alphabet.files[i];
    output << file.length() << file.c_str();
  }
}
