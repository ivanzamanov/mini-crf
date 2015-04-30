#include<fstream>
#include<algorithm>
#include<string>
#include<ios>

#include"options.hpp"
#include"crf/crf.hpp"
#include"crf/features.hpp"
#include"crf/training.hpp"
#include"crf/speech_synthesis.hpp"

void print_usage() {
    std::cerr << "Usage: <cmd> <options>\n";
    std::cerr << "--mode [synth|query]\n";
    std::cerr << "--database <bin_db_file>\n";
    std::cerr << "--input <input_string>\n";
    std::cerr << "--textgrid <textgrid_output>\n";
    std::cerr << "--phonid (query only)\n";
    std::cerr << "--sentence (query only)\n";
    std::cerr << "--concat-cost (query only)\n";

    std::cerr << "synth reads input from the input file path or stdin if - is passed\n";
}

CRF crf(state_functions(), transition_functions());
Corpus<PhonemeInstance, PhonemeInstance> corpus;

std::string to_text_string(const std::vector<PhonemeInstance>& vec) {
  std::string result(vec.size(), ' ');
  for(auto it = vec.begin(); it != vec.end(); it++)
    result.push_back( (*it).label);
  return result;
}

std::string to_id_string(const std::vector<PhonemeInstance>& vec) {
  std::stringstream result;
  auto it = vec.begin();
  result << (*it).id;
  for(; it != vec.end(); it++)
    result << ',' << (*it).id;
  return result.str();
}

int resynthesize(Options& opts) {
  pre_process(crf.label_alphabet);
  unsigned index = util::parse_int(opts.input);
  std::vector<PhonemeInstance> input = corpus.input(index);
  std::string sentence_string = to_text_string(input);
  std::cerr << "Input: " << sentence_string << '\n';

  std::vector<int> path;
  double resynth_cost = max_path(input, crf, crf.lambda, crf.mu, &path);

  SynthPrinter sp(crf.label_alphabet);
  sp.print_synth(path);
  sp.print_synth_input(input);
  sp.print_textgrid(path, opts.text_grid);
  std::cerr << "Original cost: " << concat_cost(input, crf, crf.lambda, crf.mu, input) << '\n';
  std::cerr << "Resynth. cost: " << resynth_cost << '\n';
  return 0;
}

int synthesize(Options& opts) {
  for(unsigned i = 0; i < opts.input.length(); i++)
    if(opts.input[i] == ' ') opts.input.at(i) = '_';

  std::vector<int> path;
  pre_process(crf.label_alphabet);

  std::istream* input_stream;
  if(opts.input == "-")
    input_stream = &std::cin;
  else {
    std::ifstream ifs(opts.input);
    input_stream = &ifs;
  }

  if(opts.phon_id) {
    std::vector<std::string> id_strings = split_string(opts.input, ',');
    path.resize(id_strings.size());
    std::transform(id_strings.begin(), id_strings.end(), path.begin(), util::parse_int);
  } else {
    std::vector<PhonemeInstance> input = parse_synth_input_csv(*input_stream);
    std::string sentence_string = to_text_string(input);
    std::cerr << "Input: " << sentence_string << '\n';
    max_path(input, crf, crf.lambda, crf.mu, &path);
  }

  SynthPrinter sp(crf.label_alphabet);
  sp.print_synth(path);
  sp.print_textgrid(path, opts.text_grid);
  return 0;
}

static const char DELIM = ',';
void output_frame(std::ostream& out, const Frame& frame) {
  // out << frame.pitch;
  // for(unsigned i = 0; i < frame.mfcc.length(); i++)
  //   out << DELIM << frame.mfcc[i];
}

void output_csv_columns(std::ostream& out) {
  out << "phon_id" << DELIM << "label_id" << DELIM << "pitch";
  for(unsigned i = 0; i < MFCC_N; i++)
    out << DELIM << "mfcc" << i + 1;
  out << '\n';
}

int query(const Options& opts) {
  std::vector<PhonemeInstance> input_phonemes;
  std::vector<int> int_ids;
  if(opts.phon_id) {
    std::vector<std::string> id_strings = split_string(opts.input, ',');
    input_phonemes.resize(id_strings.size());
    std::transform(id_strings.begin(), id_strings.end(), int_ids.begin(), util::parse_int);
  } else if(opts.text_input) {
    std::cerr << "Arbitrary text input supported only for --mode synth\n";
  } else if(opts.sentence) {
    int index = util::parse_int(opts.input);
    input_phonemes = corpus.input(index);
  } else {
    std::cerr << "No input type specified\n";
    return 1;
  }
  
  std::ostream& out = std::cout;
  out.precision(10);
  out << "id,duration,pitch\n";
  // output_csv_columns(out);
  for(unsigned i = 0; i < input_phonemes.size(); i++) {
    const PhonemeInstance& output = input_phonemes[i];
    PhonemeInstance p = to_synth_input(output);
    // out << p.label << DELIM << input_phonemes[i].id << DELIM;
    out << p.label << DELIM << p.duration() << DELIM << p.first().pitch;
    output_frame(out, p.first());
    out << '\n';
  }
  return 0;
}

void build_data(const Options& opts) {
  std::ifstream db(opts.synth_db);
  build_data_bin(db, crf.label_alphabet, corpus);
}

int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);
  if(argc < MIN_OPTS) {
    print_usage();
    return 1;
  }

  Options opts = parse_options(argc, argv);
  build_data(opts);
  switch(get_mode(opts.mode)) {
  case Mode::SYNTH:
    return synthesize(opts);
  case Mode::QUERY:
    return query(opts);
  case Mode::RESYNTH:
    return resynthesize(opts);
  default:
    std::cerr << "Unrecognized mode " << opts.mode << std::endl;
    return 1;
  }
  //trainGradientDescent<CRF>(crf, corpus);
  return 0;
}
