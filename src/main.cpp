#include<fstream>
#include<algorithm>
#include<string>
#include<ios>
#include<unistd.h>

#include"tool.hpp"
#include"crf.hpp"
#include"features.hpp"
#include"speech_mod.hpp"
#include"gridsearch.hpp"

void print_usage() {
    std::cerr << "Usage: <cmd> <options>\n";
    std::cerr << "--mode [synth|query]\n";
    std::cerr << "--synth-database <bin_db_file>\n";
    std::cerr << "--test-database <bin_db_file>\n";
    std::cerr << "--input <input_string>\n";
    std::cerr << "--textgrid <textgrid_output>\n";
    std::cerr << "--phonid (query only)\n";
    std::cerr << "--sentence (query only)\n";
    std::cerr << "--concat-cost (query only)\n";
    std::cerr << "--verbose\n";
    std::cerr << "synth reads input from the input file path or stdin if - is passed\n";
}

stime_t get_total_duration(std::vector<PhonemeInstance> input) {
  stime_t result = 0;
  for(auto p : input) result += p.duration;
  return result;
}

int resynthesize(Options& opts) {
  crf.mu[0] = opts.get_opt<double>("state-pitch", 0);
  crf.mu[1] = opts.get_opt<double>("state-duration", 0);
  crf.lambda[0] = opts.get_opt<double>("trans-pitch", 0);
  crf.lambda[1] = opts.get_opt<double>("trans-mfcc", 0);
  crf.lambda[2] = opts.get_opt<double>("trans-ctx", 0);

  unsigned index = util::parse<int>(opts.input);
  std::vector<PhonemeInstance> input = corpus_test.input(index);

  std::string sentence_string = gridsearch::to_text_string(input);
  INFO("Input file: " << alphabet_test.file_data_of(input[0]).file);
  INFO("Total duration: " << get_total_duration(input));
  INFO("Input: " << sentence_string);

  INFO("Original trans cost: " << concat_cost(input, crf, crf.lambda, crf.mu, input));
  INFO("Original state cost: " << state_cost(input, crf, crf.lambda, crf.mu, input));
  std::vector<int> path;
  cost resynth_cost = max_path(input, crf, crf.lambda, crf.mu, &path);

  std::vector<PhonemeInstance> output = crf.alphabet().to_phonemes(path);

  SynthPrinter sp(crf.alphabet(), labels_all);
  if(opts.has_opt("verbose"))
    sp.print_synth(path, input);
  sp.print_textgrid(path, input, labels_synth, opts.text_grid);
  INFO("Resynth. cost: " << resynth_cost);
  INFO("Resynth. trans cost: " << concat_cost(output, crf, crf.lambda, crf.mu, input));
  INFO("Resynth. state cost: " << state_cost(output, crf, crf.lambda, crf.mu, input));

  std::string outputFile = opts.get_opt<std::string>("output", "resynth.wav");
  std::ofstream wav_output(outputFile);
  Wave outputSignal = SpeechWaveSynthesis(output, input, crf.alphabet())
    .get_resynthesis();
  outputSignal.write(wav_output);
  
  FileData fileData = alphabet_test.file_data_of(input[0]);
  Wave sourceSignal;
  sourceSignal.read(fileData.file);
  gridsearch::Comparisons cmp; cmp.fill(outputSignal, sourceSignal); cmp.print();
  return 0;
}

int baseline(const Options& opts) {
  unsigned index = opts.get_opt<int>("input", 0);
  std::vector<PhonemeInstance> input = corpus_test.input(index);

  std::string sentence_string = gridsearch::to_text_string(input);
  std::cerr << "Input file: " << alphabet_test.file_data_of(input[0]).file << std::endl;
  std::cerr << "Total duration: " << get_total_duration(input) << std::endl;
  std::cerr << "Input: " << sentence_string << '\n';

  std::vector<int> path;
  baseline_crf.lambda[0] = 1;
  max_path(input, baseline_crf, baseline_crf.lambda, baseline_crf.mu, &path);

  std::vector<PhonemeInstance> output = baseline_crf.alphabet().to_phonemes(path);

  SynthPrinter sp(baseline_crf.alphabet(), labels_test);
  sp.print_synth(path, input);
  return 0;
}

bool Progress::enabled = true;
std::string gridsearch::Comparisons::metric = "";
std::string gridsearch::Comparisons::aggregate = "";

int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);
  Options opts;
  if(!init_tool(argc, argv, &opts)) {
    print_usage();
    return 1;
  }

  crf.mu[0] = 1000;
  crf.mu[1] = 1;
  crf.lambda[0] = 1000;
  crf.lambda[1] = 1;
  crf.lambda[2] = 1;

  switch(opts.get_mode()) {
  case Options::Mode::RESYNTH:
    return resynthesize(opts);
  case Options::Mode::TRAIN:
    return gridsearch::train(opts);
  case Options::Mode::BASELINE:
    return baseline(opts);
  default:
    ERROR("Unrecognized mode " << opts.mode);
    return 1;
  }

  return 0;
}
