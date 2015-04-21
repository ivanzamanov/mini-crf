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
Corpus<int, int> corpus;

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

  std::vector<PhonemeInstance> input = parse_synth_input_csv(*input_stream);

  if(opts.phon_id) {
    std::vector<std::string> id_strings = split_string(opts.input, ',');
    path.resize(id_strings.size());
    std::transform(id_strings.begin(), id_strings.end(), path.begin(), util::parse_int);
  } else {
    std::vector<CRF::Input> phoneme_sequence = crf.label_alphabet.to_sequence(opts.input);
    max_path(phoneme_sequence, crf, crf.lambda, crf.mu, &path);
  }

  SynthPrinter sp(crf.label_alphabet);
  sp.print_synth(path);
  sp.print_textgrid(path, opts.text_grid);
  return 0;
}

static const char DELIM = ',';
void output_frame(std::ostream& out, const Frame& frame) {
  out << frame.pitch;
  for(unsigned i = 0; i < frame.mfcc.length(); i++)
    out << DELIM << frame.mfcc[i];
}

void output_csv_columns(std::ostream& out) {
  out << "phon_id" << DELIM << "label_id" << DELIM << "pitch";
  for(unsigned i = 0; i < MFCC_N; i++)
    out << DELIM << "mfcc" << i + 1;
  out << '\n';
}

int query(const Options& opts) {
  std::vector<int> ids;
  if(opts.phon_id) {
    std::vector<std::string> id_strings = split_string(opts.input, ',');
    ids.resize(id_strings.size());
    std::transform(id_strings.begin(), id_strings.end(), ids.begin(), util::parse_int);
  } else if (opts.sentence) {
    int index = util::parse_int(opts.input);
    ids = corpus.input(index);
  } else return 1;
  
  FunctionalAutomaton<CRF> a(crf);
  std::ostream& out = std::cout;
  out.precision(10);
  output_csv_columns(out);
  for(unsigned i = 0; i < ids.size(); i++) {
    const PhonemeInstance& p = crf.label_alphabet.fromInt(ids[i]);
    out << p.label << "<" << DELIM << ids[i] << DELIM;
    output_frame(out, p.first());
    out << '\n';
    out << p.label << ">" << DELIM << ids[i] << DELIM;
    output_frame(out, p.last());
    out << '\n';
  }
  return 0;
}

void build_data(const Options& opts, PhonemeAlphabet* alphabet, Corpus<int, int>& corpus) {
  std::ifstream db(opts.db);
  build_data_bin(db, *alphabet, corpus);
}

int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);
  if(argc < MIN_OPTS) {
    print_usage();
    return 1;
  }

  Options opts = parse_options(argc, argv);
  build_data(opts, &crf.label_alphabet, corpus);
  switch(get_mode(opts.mode)) {
  case Mode::SYNTH:
    return synthesize(opts);
  case Mode::QUERY:
    return query(opts);
  default:
    std::cerr << "Unrecognized mode " << opts.mode << std::endl;
    return 1;
  }
  //trainGradientDescent<CRF>(crf, corpus);
  return 0;
}
