#include"crf/speech_synthesis.hpp"
#include"crf/features.hpp"
#include<algorithm>

static void print_usage(const char* main) {
  std::cout << "Usage: " << main << ": <input_file>" << std::endl;
}

Corpus<PhonemeInstance, PhonemeInstance> corpus;
PhonemeAlphabet alphabet;

void print_min_sent() {
  unsigned min = 0, minSize = corpus.input(0).size();
  for(unsigned i = 0; i < corpus.size(); i++)
    if(corpus.input(i).size() < minSize) {
      min = i;
      minSize = corpus.input(i).size();
    }
  std::cout << "Shortest sentence: " << min << std::endl;
}

void print_max_sent() {
  unsigned max = 0, maxSize = corpus.input(0).size();
  for(unsigned i = 0; i < corpus.size(); i++)
    if(corpus.input(i).size() > maxSize) {
      max = i;
      maxSize = corpus.input(i).size();
    }
  std::cout << "Longest sentence: " << max << std::endl;
}

void print_function_stats(const PhonemeAlphabet& alphabet, const std::string name,
                          double (*func)(const PhonemeInstance&, const PhonemeInstance&,
                                         int, const vector<PhonemeInstance>&)) {
  vector<PhonemeInstance> dummy;

  unsigned k = 0;
  double expect = 0;
  for(unsigned i = 0; i < alphabet.size(); i++) {
    const PhonemeInstance& p1 = alphabet.fromInt(i);

#pragma omp parallel for
    for(unsigned j = 0; j < alphabet.size(); j++) {
      if(i == j) continue;

      const PhonemeInstance& p2 = alphabet.fromInt(i);

      double value = func(p1, p2, 0, dummy);
        k++;
        expect += (value - expect) / k;
    }
  }

  std::cout << name << " expectation = " << expect << std::endl;

  k = 0;
  double dev = 0;
  for(unsigned i = 0; i < alphabet.size(); i++) {
    const PhonemeInstance& p1 = alphabet.fromInt(i);

#pragma omp parallel for
    for(unsigned j = 0; j < alphabet.size(); j++) {
      if(i == j) continue;

      const PhonemeInstance& p2 = alphabet.fromInt(i);

      double value = func(p1, p2, 0, dummy);
#pragma omp critical
      {
        k++;
        dev += (value - expect) * (value - expect);
      }
    }
  }
  std::cout << name << " deviation = " << std::sqrt(dev) * alphabet.size() << std::endl;
}

void test_performance() {
  vector<PhonemeInstance> dummy;
  std::cout << "Begin performance" << std::endl;
  const typename CRF::Alphabet::LabelClass& class1 = alphabet.classes[11];
  const typename CRF::Alphabet::LabelClass& class2 = alphabet.classes[10];
  std::cout << "Count: " << class1.size() << " x " << class2.size() << " x 70"<< std::endl;
  double result = 0;
  int m = 0;
  for(unsigned i = 0; i < 70; i++) {
    for(auto p1 : class1)
      for(auto p2 : class2) {
        result += MFCCDist(alphabet.fromInt(p1), alphabet.fromInt(p2), 0, dummy);
        m++;
      }
  }
  std::cout << "End performance " << result << " " << m << std::endl;
}

void print_transition_counts() {
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

  if(argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  std::string input_path(argv[1]);
  std::ifstream input(input_path);
  StringLabelProvider prov;
  build_data_bin(input, alphabet, corpus, prov);
  pre_process(alphabet);
  alphabet.optimize();

  print_transition_counts();
  return 0;
  test_performance();

  print_min_sent();
  print_max_sent();
  print_function_stats(alphabet, "Pitch", Pitch);
  print_function_stats(alphabet, "MFCC Transition", MFCCDist);
  return 0;
}
