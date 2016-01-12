#include"tool.hpp"

void consolidate_labels(PhonemeAlphabet& alphabet, StringLabelProvider& original,
                        StringLabelProvider& cons) {
  for(auto& p : alphabet.labels) {
    std::string label = original.convert(p.label);
    p.label = cons.convert(label);
    p.ctx_left = (p.ctx_left == INVALID_LABEL) ? INVALID_LABEL : cons.convert( original.convert(p.ctx_left));
    p.ctx_right = (p.ctx_right == INVALID_LABEL) ? INVALID_LABEL : cons.convert( original.convert(p.ctx_right));
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

void build_data(const Options& opts) {
  std::ifstream synth_db(opts.synth_db);
  build_data_bin(synth_db, alphabet_synth, corpus_synth, labels_synth);

  std::ifstream test_db(opts.test_db);
  build_data_bin(test_db, tool::alphabet_test, tool::corpus_test, tool::labels_test);

  consolidate_labels(tool::alphabet_synth, tool::labels_synth, tool::labels_all);
  consolidate_labels(tool::alphabet_test, tool::labels_test, tool::labels_all);
}

namespace tool {
  Corpus corpus_synth, corpus_test;

  CRF crf;
  BaselineCRF baseline_crf;
  PhonemeAlphabet alphabet_synth, alphabet_test;

  StringLabelProvider labels_synth;
  StringLabelProvider labels_test;
  StringLabelProvider labels_all;

  bool init_tool(int argc, const char** argv, Options* opts) {
    *opts = Options::parse_options(argc, argv);
    if(!Options::has_required(*opts))
      return false;
    COLOR_ENABLED = !opts->has_opt("no-color");
    FORCE_SCALE = opts->has_opt("force-scale");
    SMOOTH = opts->has_opt("smooth");
    SCALE_ENERGY = opts->has_opt("energy");
    PRINT_SCALE = opts->has_opt("print-scale");
    REPORT_PROGRESS = opts->has_opt("progress");
    EXTRA_TIME = opts->get_opt("extra-time", 0.0);

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
}
