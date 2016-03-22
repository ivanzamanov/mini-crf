#include<fstream>
#include<algorithm>
#include<string>
#include<ios>
#include<unistd.h>
#include<iomanip>

#include"tool.hpp"
#include"crf.hpp"
#include"features.hpp"
#include"speech_mod.hpp"
#include"gridsearch.hpp"
#include"csv.hpp"

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

stime_t get_total_duration(const std::vector<PhonemeInstance>& input) {
  stime_t result = 0;
  for(auto p : input) result += p.duration;
  return result;
}

void outputStats(CRF::Values& lambda,
                 CRF::Stats& stats) {
  CSVOutput<CRF::features::size> csv("path-stats.csv");
  CRF::Values coefs;
  for(unsigned i = 0; i < PhoneticFeatures::size; i++) {
    csv.header(i, PhoneticFeatures::Names[i]);
    coefs[i] = lambda[i];
  }

  csv << coefs;
  for(auto& s : stats)
    csv << s;
}

int resynthesize(Options& opts) {
  crf.set("state-pitch", opts.get_opt<double>("state-pitch", 0));
  crf.set("state-duration", opts.get_opt<double>("state-duration", 0));
  crf.set("state-energy", opts.get_opt<double>("state-energy", 0));

  crf.set("trans-pitch", opts.get_opt<double>("trans-pitch", 0));
  crf.set("trans-mfcc", opts.get_opt<double>("trans-mfcc", 0));
  crf.set("trans-ctx", opts.get_opt<double>("trans-ctx", 0));

  unsigned index = util::parse<int>(opts.input);
  assert(index < corpus_test.size());
  std::vector<PhonemeInstance> input = corpus_test.input(index);

  std::string sentence_string = gridsearch::to_text_string(input);
  INFO("Input file: " << alphabet_test.file_data_of(input[0]).file);
  INFO("Total duration: " << get_total_duration(input));
  INFO("Input: " << sentence_string);

  INFO("Original cost: " << concat_cost(input, crf, crf.lambda, input));

  std::vector<int> path;
  auto costs = traverse_automaton<MinPathFindFunctions, CRF, 2>(input, crf, crf.lambda, &path);

  std::vector<PhonemeInstance> output = crf.alphabet().to_phonemes(path);

  SynthPrinter sp(crf.alphabet(), labels_all);
  if(opts.has_opt("verbose"))
    sp.print_synth(path, input);
  sp.print_textgrid(path, input, labels_synth, opts.text_grid);

  CRF::Stats stats;
  INFO("Resynth. cost: " << concat_cost(output, crf, crf.lambda, input, &stats));
  INFO("Second best cost: " << costs[1]);

  outputStats(crf.lambda, stats);

  std::string outputFile = opts.get_opt<std::string>("output", "resynth.wav");
  std::ofstream wav_output(outputFile);

  Wave outputSignal = SpeechWaveSynthesis(output, input, crf.alphabet())
    .get_resynthesis(opts);

  if(opts.has_opt("verbose")) {
    Wave tdSignal = SpeechWaveSynthesis(output, input, crf.alphabet())
      .get_resynthesis_td();

    double diff = 0;
    for(unsigned i = 0; i < tdSignal.length(); i++)
      diff += std::abs(outputSignal[i] - tdSignal[i]);

    INFO("Samples: " << tdSignal.length());
    INFO("TD/FD Error: " << diff);
    INFO("TD/FD Error mean: " << diff / tdSignal.length());
  }

  outputSignal.write(wav_output);

  FileData fileData = alphabet_test.file_data_of(input[0]);
  Wave sourceSignal;
  sourceSignal.read(fileData.file);
  if(opts.has_opt("verbose")) {
    gridsearch::Comparisons cmp;
    cmp.fill(outputSignal, sourceSignal);
    INFO("LogSpectrum = " << cmp.value());
  }
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
  traverse_automaton<MinPathFindFunctions>(input, baseline_crf,
                                           baseline_crf.lambda, &path);

  std::vector<PhonemeInstance> output = baseline_crf.alphabet().to_phonemes(path);

  Wave outputSignal = SpeechWaveSynthesis(output, input, baseline_crf.alphabet())
    .get_resynthesis_td();
  std::string outputFile = opts.get_opt<std::string>("output", "resynth.wav");
  std::ofstream wav_output(outputFile);
  outputSignal.write(wav_output);

  return 0;
}

bool Progress::enabled = true;

int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);
  //(std::cout << std::fixed);//.precision(8);
  //(std::cerr << std::fixed);//.precision(8);
  Options opts;
  if(!init_tool(argc, argv, &opts)) {
    print_usage();
    return 1;
  }

  crf.lambda[0] = 1000;
  crf.lambda[1] = 1;
  crf.lambda[2] = 1;
  crf.lambda[3] = 1;
  crf.lambda[4] = 1;
  crf.lambda[5] = 1;

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
