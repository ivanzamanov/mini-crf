#ifndef __TOOL_HPP__
#define __TOOL_HPP__

#include"speech_synthesis.hpp"
#include"options.hpp"
#include"crf.hpp"
#include"features.hpp"
#include"speech_mod.hpp"

namespace tool {
  extern Corpus corpus_synth, corpus_test, corpus_eval;

  extern CRF crf;
  extern BaselineCRF baseline_crf;
  extern PhonemeAlphabet alphabet_synth, alphabet_test;

  extern StringLabelProvider labels_synth;
  extern StringLabelProvider labels_test;
  extern StringLabelProvider labels_all;

  extern std::ofstream VLOG;

  bool init_tool(int argc, const char** argv, Options* opts);
}

#endif
