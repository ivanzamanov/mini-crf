#include<fstream>
#include<algorithm>
#include<string>
#include<ios>

#include"options.hpp"
#include"crf/crf.hpp"
#include"crf/features.hpp"
#include"crf/training.hpp"
#include"crf/speech_synthesis.hpp"

CRF crf;
typename CRF::Alphabet test_alphabet;
Corpus<PhonemeInstance, PhonemeInstance> synth_corpus;
Corpus<PhonemeInstance, PhonemeInstance> test_corpus;

void build_data(const Options& opts) {
  std::cerr << "Synth db: " << opts.synth_db << std::endl;
  std::ifstream synth_db(opts.synth_db);
  build_data_bin(synth_db, crf.label_alphabet, synth_corpus);
  if(opts.test_db != "") {
    std::cerr << "Test db: " << opts.test_db << std::endl;
    std::ifstream test_db(opts.test_db);
    build_data_bin(test_db, test_alphabet, test_corpus);
  }
}

void benchmark_features() {
	std::cout << "Benchmarking mfcc..." << std::endl;
	double d = 0;
	vector<PhonemeInstance> dummy;
	auto alpha = crf.label_alphabet;
	for(unsigned i = 0; i < alpha.size(); i++) {
		for(unsigned j = 0; j < alpha.size(); j++) {
			const PhonemeInstance& p1 = alpha.fromInt(i);
			const PhonemeInstance& p2 = alpha.fromInt(j);

			//d += MFCCDist(p1, p2, 0, dummy);
			d += Pitch(p1, p2, 0, dummy);

			if(i * alpha.size() > 366 * 1000000)
				return;
		}
	}
	std::cout << d << std::endl;
}

int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);

  crf.mu[0] = 1000;
  crf.mu[1] = 1;
  crf.lambda[0] = 1000;
  crf.lambda[1] = 1;

  for(int i = 0; i < argc; i++)
	std::cerr << argv[i] << " ";

  std::cerr << std::endl;

  Options opts = parse_options(argc, argv);
  build_data(opts);

  pre_process(crf.label_alphabet);
  pre_process(crf.label_alphabet, synth_corpus);
  pre_process(test_alphabet);
  pre_process(test_alphabet, test_corpus);

  benchmark_features();

  return 0;
}
