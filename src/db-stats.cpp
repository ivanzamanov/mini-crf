#include<algorithm>

#include"opencl-utils.hpp"
#include"gridsearch.hpp"
#include"tool.hpp"

using namespace tool;

bool Progress::enabled = true;
std::string gridsearch::Comparisons::metric = "";
std::string gridsearch::Comparisons::aggregate = "";

bool print_stats() { return true; }

bool print_by_id_short(const PhonemeAlphabet& alphabet, id_t phon_id) {
  if(phon_id >= alphabet.size())
    return false;
  auto& phon = alphabet.fromInt(phon_id);

  LOG(phon_id
      << " " << alphabet.file_data_of(phon).file
      << " " << phon.start
      << " " << phon.end);

  return true;
}

bool print_label(const std::string label) {
  auto label_id = labels_all.convert(label);
  LOG("Label: " << label);

  LOG("Test:");
  for(auto& id : alphabet_test.get_class(label_id))
    print_by_id_short(alphabet_test, id);

  LOG("Synth:");
  for(auto& id : alphabet_synth.get_class(label_id))
    print_by_id_short(alphabet_synth, id);
  return true;
}

bool extract_by_id(const PhonemeAlphabet& alphabet, id_t id, const std::string output) {
  if(id >= alphabet.size()) {
    ERROR("No such instance: " << id);
    return false;
  }

  const auto& p = alphabet.fromInt(id);
  WaveBuilder wb(WaveHeader::default_header());
  Wave w;
  w.read(alphabet.file_data_of(p));
  auto wd = w.extractByTime(p.start, p.end);
  wb.append( wd );
  wb.build().write( output );
  return false;
}

bool extract_by_label(const PhonemeAlphabet& alphabet, std::string label_string) {
  auto label = labels_all.convert(label_string);
  for(auto& id : alphabet.get_class(label)) {
    std::string output = std::to_string(id) + ".wav";
    extract_by_id(alphabet, id, output);
  }
  return true;
}

bool handle(const Options& opts) {
  if(opts.has_opt("stats"))
    return print_stats();

  if(opts.has_opt("extract")) {
    std::string db_type = opts.get_string("db");
    auto* db = &alphabet_synth;
    if(db_type == std::string("test"))
      db = &alphabet_test;

    if(opts.has_opt("id")) {
      auto id = opts.get_opt<id_t>("id", 0);
      std::string output = opts.get_opt("output", std::to_string(id) + ".wav");
      return extract_by_id(*db, id, output);
    } else if(opts.has_opt("label")) {
        auto label = opts.get_string("label");
        return extract_by_label(*db, label);
    }
  }

  if(opts.has_opt("label"))
    return print_label(opts.get_string("label"));

  return false;
}

int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);
  Options opts;
  if(!init_tool(argc, argv, &opts))
    return 1;

  return handle(opts);
}
