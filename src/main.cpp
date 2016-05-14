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

  auto sws = SpeechWaveSynthesis(output, input, crf.alphabet());
  Wave outputSignal = sws.get_resynthesis(opts);

  std::string outputFile = opts.get_opt<std::string>("output", "resynth.wav");
  std::ofstream wav_output(outputFile);
  outputSignal.write(wav_output);

  auto sws2 = SpeechWaveSynthesis(input, input, alphabet_test);
  auto concatenation = sws2.get_concatenation(opts);
  concatenation.write("original.wav");

  FileData fileData = alphabet_test.file_data_of(input[0]);
  if(opts.has_opt("verbose")) {
    Comparisons cmp;
    cmp.fill(concatenation, outputSignal);
    INFO("LogSpectrum = " << cmp.LogSpectrum);
    INFO("LogSpectrumCritical = " << cmp.LogSpectrumCritical);
    INFO("SegSNR = " << cmp.SegSNR);
    INFO("MFCC = " << cmp.MFCC);
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
  std::string outputFile = opts.get_opt<std::string>("output", "baseline.wav");
  std::ofstream wav_output(outputFile);
  outputSignal.write(wav_output);

  return 0;
}

int psola(const Options& opts) {
  auto inputString = opts.get_opt<std::string>("input", "");
  auto inputPhonemes = util::split_string(inputString, ',');

  std::vector<int> input(inputPhonemes.size());
  std::transform(inputPhonemes.begin(), inputPhonemes.end(), input.begin(), util::parse<int>);

  std::vector<PhonemeInstance> phonemeInput;
  if(input.size() > 1)
    phonemeInput = alphabet_test.to_phonemes(input);
  else {
    phonemeInput = corpus_test.input(input[0]);
    INFO("Input file: " << alphabet_test.file_data_of(phonemeInput[0]).file);
  }

  auto sws = SpeechWaveSynthesis(phonemeInput, phonemeInput, alphabet_test);
  auto outputSignal = sws.get_resynthesis(opts);
  auto original = sws.get_concatenation(opts);
  original.write("original.wav");

  auto outputFile = opts.get_opt<std::string>("output", "psola.wav");
  std::ofstream wav_output(outputFile);
  outputSignal.write(wav_output);

  return 0;
}

int couple(const Options& opts) {
  auto inputString = opts.get_opt<std::string>("input", "");
  auto inputPhonemes = util::split_string(inputString, ',');

  std::vector<int> input(inputPhonemes.size());
  std::transform(inputPhonemes.begin(), inputPhonemes.end(), input.begin(), util::parse<int>);

  std::vector<PhonemeInstance> phonemeInput;
  if(input.size() > 1)
    phonemeInput = crf.alphabet().to_phonemes(input);
  else {
    phonemeInput = corpus_synth.input(input[0]);
    INFO("Input file: " << crf.alphabet().file_data_of(phonemeInput[0]).file);
  }

  auto outputSignal = SpeechWaveSynthesis(phonemeInput, phonemeInput, crf.alphabet())
    .get_coupling(opts);
  auto outputFile = opts.get_opt<std::string>("output", "coupled.wav");
  std::ofstream wav_output(outputFile);
  outputSignal.write(wav_output);

  return 0;
}

bool Progress::enabled = true;

int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);
  (std::cout << std::fixed).precision(10);
  (std::cerr << std::fixed).precision(10);
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
  case Options::Mode::COUPLE:
    return couple(opts);
  case Options::Mode::PSOLA:
    return psola(opts);
  default:
    ERROR("Unrecognized mode " << opts.mode);
    return 1;
  }

  return 0;
}
