#include<fstream>
#include<algorithm>
#include<string>
#include<ios>
#include<unistd.h>

#include"tool.hpp"
#include"crf.hpp"
#include"features.hpp"
#include"threadpool.h"
#include"speech_mod.hpp"

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
    std::cerr << "synth reads input from the input file path or stdin if - is passed\n";
}

std::string to_text_string(const std::vector<PhonemeInstance>& vec) {
  std::string result(1, ' ');
  for(auto it = vec.begin(); it != vec.end(); it++) {
    result.push_back('|');
    result.append(labels_all.convert((*it).label) );
    result.push_back('|');
  }
  return result;
}

stime_t get_total_duration(std::vector<PhonemeInstance> input) {
  stime_t result = 0;
  for(auto p : input) result += p.duration;
  return result;
}

int resynthesize(Options& opts) {
  PlainStreamConfig conf(std::cin);
  coefficient val;
  crf.mu[0] = conf.get("state-pitch", val);
  crf.mu[1] = conf.get("state-duration", val);
  crf.lambda[0] = conf.get("trans-pitch", val);
  crf.lambda[1] = conf.get("trans-mfcc", val);
  crf.lambda[2] = conf.get("trans-ctx", val);

  unsigned index = util::parse_int(opts.input);
  std::vector<PhonemeInstance> input = corpus_test.input(index);

  std::string sentence_string = to_text_string(input);
  std::cerr << "Input file: " << alphabet_test.file_data_of(input[0]).file << std::endl;
  std::cerr << "Total duration: " << get_total_duration(input) << std::endl;
  std::cerr << "Input: " << sentence_string << '\n';

  std::cerr << "Original trans cost: " << concat_cost(input, crf, crf.lambda, crf.mu, input) << '\n';
  std::cerr << "Original state cost: " << state_cost(input, crf, crf.lambda, crf.mu, input) << '\n';
  std::vector<int> path;
  cost resynth_cost = max_path(input, crf, crf.lambda, crf.mu, &path);

  std::vector<PhonemeInstance> output = crf.alphabet().to_phonemes(path);

  SynthPrinter sp(crf.alphabet(), labels_all);
  if(opts.has_opt("--verbose"))
    sp.print_synth(path, input);
  sp.print_textgrid(path, input, labels_synth, opts.text_grid);
  std::cerr << "Resynth. cost: " << resynth_cost << '\n';
  std::cerr << "Resynth. trans cost: " << concat_cost(output, crf, crf.lambda, crf.mu, input) << '\n';
  std::cerr << "Resynth. state cost: " << state_cost(output, crf, crf.lambda, crf.mu, input) << std::endl;

  std::string outputFile = opts.get_opt("--output");
  std::ofstream wav_output(outputFile);
  SpeechWaveSynthesis(output, input, crf.alphabet())
    .get_resynthesis()
    .write(wav_output);
  return 0;
}

struct ResynthParams {
  void init(int index, std::ostream* os, bool* flag) {
    this->index = index;
    this->os = os;
    this->flag = flag;
  }

  int index;
  std::ostream *os;
  bool* flag;
};

void resynth_index(ResynthParams* params) {
  int index = params->index;
  
  std::vector<PhonemeInstance> input = corpus_test.input(index);
  std::string sentence_string = to_text_string(input);
  std::vector<int> path;

  cost cost = max_path(input, crf, crf.lambda, crf.mu, &path);
  std::vector<PhonemeInstance> output = crf.alphabet().to_phonemes(path);

  std::cerr << "Cost " << index << ": " << cost << std::endl;

  std::stringstream outputFile;
  outputFile << "/tmp/resynth-" << params->index << ".wav";

  std::ofstream wav_output(outputFile.str());
  SpeechWaveSynthesis(output, input, crf.alphabet())
    .get_resynthesis()
    .write(wav_output);

  std::string input_file = alphabet_test.file_data_of(input[0]).file;
  std::ostream& outputStream = *(params->os);
  outputStream << input_file << " " << outputFile.str();
  outputStream.flush();

  *(params->flag) = 1;
}

int train(const Options&) {
  Progress::enabled = false;
  PlainStreamConfig conf(std::cin);

  coefficient val;
  crf.mu[0] = conf.get("state-pitch", val);
  crf.mu[1] = conf.get("state-duration", val);
  crf.lambda[0] = conf.get("trans-pitch", val);
  crf.lambda[1] = conf.get("trans-mfcc", val);
  crf.lambda[2] = conf.get("trans-ctx", val);

  unsigned count = corpus_test.size();
  std::stringstream streams[count];
  bool flags[count];
  //#pragma omp parallel for
  ThreadPool tp(8);
  int ret = tp.initialize_threadpool();
  if (ret == -1) {
    cerr << "Failed to initialize thread pool!" << endl;
    return 0;
  }

  ResynthParams params[count];
  for(unsigned i = 0; i < count; i++) {
    flags[i] = 0;
    params[i].init(i, &streams[i], &flags[i]);
    Task* t = new ParamTask<ResynthParams>(&resynth_index, &params[i]);
    tp.add_task(t);
  }

  bool done;
  do {
    sleep(5);
    done = true;
    for(unsigned i = 0; i < count; i++)
      done &= flags[i];
    cerr << "Unfinished: ";
    for(unsigned i = 0; i < count; i++)
      if(!flags[i])
        cerr << i << " ";
    cerr << std::endl;
  } while(!done);

  tp.destroy_threadpool();

  const std::string delim = "\n";
  std::cout << streams[0].str() << std::endl;
  for(unsigned i = 1; i < count; i++) {
    std::cout << delim << streams[i].str() << std::endl;
  }

  return 0;
}

int baseline(const Options& opts) {
  unsigned index = util::parse_int(opts.input);
  std::vector<PhonemeInstance> input = corpus_test.input(index);

  std::string sentence_string = to_text_string(input);
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
int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);
  if(!init_tool(argc, argv)) {
    print_usage();
    return 1;
  }

  crf.mu[0] = 1000;
  crf.mu[1] = 1;
  crf.lambda[0] = 1000;
  crf.lambda[1] = 1;
  crf.lambda[2] = 1;

  Options opts = parse_options(argc, argv);
  switch(get_mode(opts.mode)) {
  case Mode::RESYNTH:
    return resynthesize(opts);
  case Mode::TRAIN:
    return train(opts);
  case Mode::BASELINE:
    return baseline(opts);
  default:
    std::cerr << "Unrecognized mode " << opts.mode << std::endl;
    return 1;
  }

  return 0;
}
