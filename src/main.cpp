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
    std::cerr << "--concat-cost (query only)\n";
    std::cerr << "--verbose\n";
    std::cerr << "synth reads input from the input file path or stdin if - is passed\n";
}

stime_t get_total_duration(const std::vector<PhonemeInstance>& input) {
  stime_t result = 0;
  for(auto p : input) result += p.duration;
  return result;
}

Corpus& get_corpus(const Options& opts) {
  return opts.has_opt("eval") ? corpus_eval : corpus_test;
}

void outputStats(CRF::Values& lambda,
                 CRF::Stats& stats,
                 const Options& opts) {
  auto file = opts.get_opt<std::string>("path-stats", "");
  if(file == "") return;
  CSVOutput<CRF::features::size> csv(file);
  CRF::Values coefs;
  for(unsigned i = 0; i < PhoneticFeatures::size; i++) {
    csv.header(i, PhoneticFeatures::Names[i]);
    coefs[i] = lambda[i];
  }

  csv << coefs;
  for(auto& s : stats)
    csv << s;
}

void outputPath(const Options& opts,
                const std::vector<PhonemeInstance>& output,
                const std::vector<PhonemeInstance>& input) {
  auto file = opts.get_opt<std::string>("path", "");
  if(file == "") return;
  CSVOutput<7> os(file);
  os.all_headers("id", "duration", "i_duration",
                 "pitch_l", "pitch_r",
                 "i_pitch_l", "i_pitch_r");
  for(auto k = 0u; k < input.size(); k++) {
    auto& o = output[k];
    auto& i = input[k];
    os.print(o.old_id,
             o.duration, i.duration,
             o.pitch_contour_original[0], o.pitch_contour_original[1],
             i.pitch_contour_original[0], i.pitch_contour_original[1]
             );
  }
}

void outputComparisons(const Options& opts,
                       const Comparisons& cmp,
                       cost baselineCost) {
  auto file = opts.get_opt<std::string>("cmp-csv", "");
  if(file == "") return;
  CSVOutput<6> os(file);
  os.all_headers("LogSpectrum", "LogSpectrumCritical", "SegSNR",
                 "MFCC", "WSS", "Baseline");
  os.print(cmp.LogSpectrum, cmp.LogSpectrumCritical, cmp.SegSNR,
           cmp.MFCC, cmp.WSS, baselineCost);
}

void readCoefOptions(const Options& opts) {
  for(auto name : PhoneticFeatures::Names) {
    crf.set(name, opts.get_opt<double>(name, 0) / opts.get_opt("norm-" + name, 1.0));
  }
}

int resynthesize(Options& opts) {
  readCoefOptions(opts);

  auto index = util::parse<unsigned>(opts.input);
  auto corpus = get_corpus(opts);
  assert(index < corpus.size());
  std::vector<PhonemeInstance> input = corpus.input(index);

  INFO("Input file: " << alphabet_test.file_data_of(input[0]).file);
  INFO("Total duration: " << get_total_duration(input));
  INFO("Original cost: " << concat_cost(input, crf, crf.lambda, input));

  std::vector<int> path;
  traverse_automaton<MinPathFindFunctions, CRF, 1>(input, crf, crf.lambda, &path);

  std::vector<PhonemeInstance> output = crf.alphabet().to_phonemes(path);

  SynthPrinter sp(crf.alphabet(), labels_all);
  if(opts.has_opt("verbose"))
    sp.print_synth(path, input);
  sp.print_textgrid(path, input, labels_synth, opts.text_grid);

  CRF::Stats stats;
  INFO("Resynth. cost: " << concat_cost(output, crf, crf.lambda, input, &stats));
  //INFO("Second best cost: " << costs[1]);
  auto baselineCost = concat_cost(output, baseline_crf, baseline_crf.lambda, input);
  INFO("Baseline cost: " << baselineCost);

  outputStats(crf.lambda, stats, opts);
  outputPath(opts, output, input);

  auto sws = SpeechWaveSynthesis(output, input, crf.alphabet());
  Wave outputSignal = sws.get_resynthesis(opts);

  outputSignal.write(opts.get_opt<std::string>("output", "resynth.wav"));

  auto sws2 = SpeechWaveSynthesis(input, input, alphabet_test);
  auto concatenation = sws2.get_concatenation();
  concatenation.write(opts.get_opt<std::string>("original", "original.wav"));

  if(opts.has_opt("verbose")) {
    Comparisons cmp;
    cmp.fill(concatenation, outputSignal);
    INFO("LogSpectrum = " << cmp.LogSpectrum);
    INFO("LogSpectrumCritical = " << cmp.LogSpectrumCritical);
    INFO("SegSNR = " << cmp.SegSNR);
    INFO("MFCC = " << cmp.MFCC);
    INFO("WSS = " << cmp.WSS);
    outputComparisons(opts, cmp, baselineCost);
  }
  return 0;
}

int baseline(const Options& opts) {
  readCoefOptions(opts);

  auto index = opts.get_opt<unsigned>("input", 0);
  auto corpus = get_corpus(opts);
  std::vector<PhonemeInstance> input = corpus.input(index);

  INFO("Input file: " << alphabet_test.file_data_of(input[0]).file);
  INFO("Total duration: " << get_total_duration(input));

  std::vector<int> path;
  traverse_automaton<MinPathFindFunctions>(input, baseline_crf,
                                           baseline_crf.lambda, &path);

  std::vector<PhonemeInstance> output = baseline_crf.alphabet().to_phonemes(path);

  Wave outputSignal = SpeechWaveSynthesis(output, input, baseline_crf.alphabet())
    .get_resynthesis_td();
  outputSignal.write(opts.get_opt<std::string>("output", "baseline.wav"));

  auto sws2 = SpeechWaveSynthesis(input, input, alphabet_test);
  auto concatenation = sws2.get_concatenation();
  concatenation.write(opts.get_opt<std::string>("original", "original.wav"));

  CRF::Stats stats;
  INFO("Baseline cost in original: " << concat_cost(output, crf, crf.lambda, input, &stats));
  auto baselineCost = concat_cost(output, baseline_crf, baseline_crf.lambda, input);
  INFO("Baseline cost:" << baselineCost);

  outputStats(crf.lambda, stats, opts);
  outputPath(opts, output, input);

  if(opts.has_opt("verbose")) {
    auto sws = SpeechWaveSynthesis(input, input, alphabet_test);
    auto concatenation = sws.get_concatenation();
    Comparisons cmp;
    cmp.fill(concatenation, outputSignal);
    INFO("LogSpectrum = " << cmp.LogSpectrum);
    INFO("LogSpectrumCritical = " << cmp.LogSpectrumCritical);
    INFO("SegSNR = " << cmp.SegSNR);
    INFO("MFCC = " << cmp.MFCC);
    INFO("WSS = " << cmp.WSS);
    outputComparisons(opts, cmp, baselineCost);
  }

  return 0;
}

int compare(const Options& opts) {
  auto inputFile = opts.get_opt<std::string>("i", "");
  auto outputFile = opts.get_opt<std::string>("o", "");
  if(inputFile == "" || outputFile == "")
    return 1;
  auto w1 = Wave(inputFile);
  auto w2 = Wave(outputFile);
  ComparisonDetails cmp;
  cmp.fill(w1, w2);
  assert(cmp.LogSpectrumValues.size() == cmp.LogSpectrumCriticalValues.size());
  assert(cmp.LogSpectrumValues.size() == cmp.SegSNRValues.size());
  assert(cmp.LogSpectrumValues.size() == cmp.MFCCValues.size());
  assert(cmp.LogSpectrumValues.size() == cmp.WSSValues.size());

  auto output = CSVOutput<6>(opts.get_opt<std::string>("output", "comparisons.csv"));
  output.all_headers("LogSpectrum",
                     "LogSpectrumCritical",
                     "SegSNR",
                     "MFCC",
                     "WSS",
                     "FrameStart");
  output.print(cmp.LogSpectrum,
               cmp.LogSpectrumCritical,
               cmp.SegSNR,
               cmp.MFCC,
               cmp.WSS,
               w2.duration());
  for(auto i = 0u; i < cmp.LogSpectrumValues.size(); i++) {
    output.print(cmp.LogSpectrumValues[i],
                 cmp.LogSpectrumCriticalValues[i],
                 cmp.SegSNRValues[i],
                 cmp.MFCCValues[i],
                 cmp.WSSValues[i],
                 cmp.times[i]);
  }
  INFO("Frames: " << cmp.LogSpectrumValues.size());
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
    Corpus& corpus = get_corpus(opts);
    phonemeInput = corpus.input(input[0]);
    INFO("Input file: " << alphabet_test.file_data_of(phonemeInput[0]).file);
  }

  auto sws = SpeechWaveSynthesis(phonemeInput, phonemeInput, alphabet_test);
  auto outputSignal = sws.get_resynthesis(opts);
  auto original = sws.get_concatenation();
  original.write("original.wav");

  outputSignal.write(opts.get_opt<std::string>("output", "psola.wav"));
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
  outputSignal.write(opts.get_opt<std::string>("output", "coupled.wav"));

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
  baseline_crf.lambda[0] = 1;

  switch(opts.get_mode()) {
  case Options::Mode::RESYNTH:
    return resynthesize(opts);
  case Options::Mode::TRAIN:
    return gridsearch::train(opts);
  case Options::Mode::BASELINE:
    return baseline(opts);
  case Options::Mode::COMPARE:
    return compare(opts);
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
