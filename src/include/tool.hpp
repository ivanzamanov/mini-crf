#ifndef __TOOL_HPP__
#define __TOOL_HPP__

#include"speech_synthesis.hpp"
#include"options.hpp"
#include"crf.hpp"
#include"features.hpp"

namespace tool {
  Corpus corpus_synth, corpus_test;
  
  CRF crf;
  BaselineCRF baseline_crf;
  PhonemeAlphabet alphabet_synth, alphabet_test;

  StringLabelProvider labels_synth;
  StringLabelProvider labels_test;
  StringLabelProvider labels_all;
  
  void build_data(const Options& opts);
  void consolidate_labels(PhonemeAlphabet& alphabet, StringLabelProvider& original,
                          StringLabelProvider& cons);
  void remap(PhonemeAlphabet& alph, Corpus& corp);

  bool init_tool(int argc, const char** argv, Options* opts) {
    *opts = parse_options(argc, argv);
    if(!has_required(*opts))
      return false;
    crf.label_alphabet = &alphabet_synth;
    build_data(*opts);

    pre_process(alphabet_synth, corpus_synth);
    pre_process(alphabet_test, corpus_test);
  
    alphabet_synth.optimize();
    remap(alphabet_synth, corpus_synth);

    alphabet_test.optimize();
    remap(alphabet_test, corpus_test);

    return true;
  }

  void build_data(const Options& opts) {
    std::cerr << "Synth db: " << opts.synth_db << std::endl;
    std::ifstream synth_db(opts.synth_db);
    build_data_bin(synth_db, alphabet_synth, corpus_synth, labels_synth);

    std::cerr << "Test db: " << opts.test_db << std::endl;
    std::ifstream test_db(opts.test_db);
    build_data_bin(test_db, alphabet_test, corpus_test, labels_test);

    consolidate_labels(alphabet_synth, labels_synth, labels_all);
    consolidate_labels(alphabet_test, labels_test, labels_all);
  }

  void consolidate_labels(PhonemeAlphabet& alphabet, StringLabelProvider& original,
                          StringLabelProvider& cons) {
    auto it = alphabet.labels.begin();
    while(it != alphabet.labels.end()) {
      PhonemeInstance& p = *it;
      std::string label = original.convert(p.label);
      p.label = cons.convert(label);
      p.ctx_left = (p.ctx_left == INVALID_LABEL) ? INVALID_LABEL : cons.convert( original.convert(p.ctx_left));
      p.ctx_right = (p.ctx_right == INVALID_LABEL) ? INVALID_LABEL : cons.convert( original.convert(p.ctx_right));
      ++it;
    }
  }
  
  void remap(PhonemeAlphabet& alph, Corpus& corp) {
    for(unsigned i = 0; i < corp.size(); i++) {
      auto& labels = corp.label(i);
      for(auto& p : labels)
        p.id = alph.new_id(p.id);

      auto& inputs = corp.input(i);
      for(auto& p : inputs)
        p.id = alph.new_id(p.id);
    }
  }
}

#endif
