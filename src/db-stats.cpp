#include<algorithm>

#include"features.hpp"
#include"tool.hpp"

using namespace tool;

void print_min_sent(Corpus corpus) {
  unsigned min = 0, minSize = corpus.input(0).size();
  for(unsigned i = 0; i < corpus.size(); i++)
    if(corpus.input(i).size() < minSize) {
      min = i;
      minSize = corpus.input(i).size();
    }
  std::cout << "Shortest sentence: " << min << std::endl;
}

void print_max_sent(tool::Corpus corpus) {
  unsigned max = 0, maxSize = corpus.input(0).size();
  for(unsigned i = 0; i < corpus.size(); i++)
    if(corpus.input(i).size() > maxSize) {
      max = i;
      maxSize = corpus.input(i).size();
    }
  std::cout << "Longest sentence: " << max << std::endl;
}

void print_transition_counts(PhonemeAlphabet alphabet, Corpus corpus) {
  unsigned result = 0;
  FeatureValuesCacheProvider prov;
  for(unsigned s = 0; s < corpus.size(); s++) {
    auto sent = corpus.label(s);
    for(unsigned i = 1; i < sent.size(); i++) {
      auto class1 = alphabet.get_class( sent[i - 1] );
      auto class2 = alphabet.get_class( sent[i] );
      FeatureValuesCache& cache = prov.get(sent[i-1].label, sent[i].label);

      if(cache.values == 0) {
        result += class1.size() * class2.size();
        std::cout << "So far: " << result << std::endl;
        cache.init(1, 1);
      }
    }
  }

  unsigned classCount = 0;
  for(auto cl : alphabet.classes) {
    if(cl.size() > 0)
      classCount++;
  }
  std::cout << "Total classes: " << classCount << std::endl;
  std::cout << "Class pairs: " << prov.values.size() << std::endl;
  std::cout << "Transitions: " << result << std::endl;
}

int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);

  if(!init_tool(argc, argv))
    return 1;

  std::cout << "Size: " << alphabet_synth.size() << std::endl;
  std::cout << "Corpus: " << corpus_synth.size() << std::endl;
  print_transition_counts(alphabet_synth, corpus_test);
  //return 0;
  //print_min_sent();
  //print_max_sent();
  //print_transition_counts();
  //return 0;

  //test_performance();
  //print_function_stats(alphabet_synth, "Pitch", Pitch);
  //print_function_stats(alphabet_synth, "MFCC Transition", MFCCDist);
  return 0;
}
