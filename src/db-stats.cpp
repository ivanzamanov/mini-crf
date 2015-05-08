#include"crf/speech_synthesis.hpp"
#include"crf/features.hpp"
#include<algorithm>

static void print_usage(const char* main) {
  std::cout << "Usage: " << main << ": <input_file>";
}

Corpus<PhonemeInstance, PhonemeInstance> corpus;
PhonemeAlphabet alphabet;

template<class Func>
void print_function_stats(const PhonemeAlphabet& alphabet, const char* name) {

  Progress prog(alphabet.size() * (alphabet.size() - 1) / 10000);
  unsigned k = 0;
  double expect = 0;
  for(unsigned i = 0; i < alphabet.size(); i++) {
    const PhonemeInstance& p1 = alphabet.fromInt(i);

    for(unsigned j = 0; j < alphabet.size(); j++) {
      if(i == j) continue;

      const PhonemeInstance& p2 = alphabet.fromInt(i);

      Func f;
      double value = f(p1, p2);
      
      k++;
      expect += (value - expect) / k;
      if(k % 10000 == 0)
      prog.update();
    }
  }
  prog.finish();

  //std::cout << name << " mean = " << mean << std::endl;
  std::cout << name << " expc = " << expect << std::endl;
}

int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);

  if(argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  std::string input_path(argv[1]);
  std::ifstream input(input_path);
  build_data_bin(input, alphabet, corpus);
  pre_process(alphabet);
  
  print_function_stats<MFCCDist>(alphabet, "MFCC");
  print_function_stats<Pitch>(alphabet, "Pitch");
  return 0;
}