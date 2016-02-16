#include<algorithm>

#include"opencl-utils.hpp"
#include"gridsearch.hpp"
#include"tool.hpp"
#include"features.hpp"

using namespace tool;
using std::vector;
using std::pair;
using std::string;

bool Progress::enabled = true;
string gridsearch::Comparisons::metric = "";
string gridsearch::Comparisons::aggregate = "";

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

bool print_label(const string label) {
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

bool extract_by_id(const PhonemeAlphabet& alphabet, id_t id, const string output) {
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

bool extract_by_label(const PhonemeAlphabet& alphabet, string label_string) {
  auto label = labels_all.convert(label_string);
  for(auto& id : alphabet.get_class(label)) {
    string output = std::to_string(id) + ".wav";
    extract_by_id(alphabet, id, output);
  }
  return true;
}

template<class Func>
bool print_stats_func_edge(const PhonemeAlphabet& alphabet, Func f, const std::string displayName) {
  double mean = 0, max = 0;
  int n = 0;
  const vector<PhonemeInstance> dummy;
  for(auto& x : alphabet.labels) {
    for(auto& y : alphabet.labels) {
      n++;
      double val = f(x, y, 0, dummy);
      mean = mean + (val - mean) / n;

      if(val > max)
        max = val;
    }
  }

  std::cout << displayName + " Max = " << max << std::endl
            << displayName + " Avg = " << mean << std::endl;
  return true;
}

template<class Field, class _FieldValueType=double>
bool print_stats_field(const PhonemeAlphabet& alphabet, string label_string, Field f) {
  auto label = labels_all.convert(label_string);
  vector<_FieldValueType> values;
  for(auto& id : alphabet.get_class(label)) {
    auto& p = alphabet.fromInt(id);
    std::cout << id << " " << f(p) << std::endl;
  }
  return true;
}

#define FUNC_AS_PARAM(x) x, #x
bool print_stats(const PhonemeAlphabet& alphabet) {
  INFO("Stats over " << alphabet.size() << " labels");
  print_stats_func_edge(alphabet, &FUNC_AS_PARAM(Features::EnergyTrans));
  print_stats_func_edge(alphabet, &FUNC_AS_PARAM(Features::Pitch));
  print_stats_func_edge(alphabet, &FUNC_AS_PARAM(Features::MFCCDist));
  return true;
}

bool print_phon_stats(const PhonemeAlphabet& alphabet) {
  std::cout << "Size: " << alphabet.size() << std::endl;
  auto totalDuration = 0.0;
  for(auto& x : alphabet.labels) {
    totalDuration += (x.end - x.start);
  }
  std::cout << "Total Duration: " << totalDuration << std::endl;
  return true;
}

bool correlate(const Options&) {
  return true;
}

bool handle(const Options& opts) {
  string db_type = opts.get_string("db");
  auto* db = &alphabet_synth;
  if(db_type == string("test"))
    db = &alphabet_test;

  if(opts.has_opt("stats"))
    return print_stats(*db);

  if(opts.has_opt("phon-stats"))
    return print_phon_stats(*db);

  if(opts.has_opt("extract")) {
    if(opts.has_opt("id")) {
      auto id = opts.get_opt<id_t>("id", 0);
      string output = opts.get_opt("output", std::to_string(id) + ".wav");
      return extract_by_id(*db, id, output);
    } else if(opts.has_opt("label")) {
        auto label = opts.get_string("label");
        return extract_by_label(*db, label);
    }
  }

  if(opts.has_opt("label-stats"))
    return print_stats_field(*db, opts.get_string("label"), [](const PhonemeInstance& p) { return p.energy; });

  if(opts.has_opt("label"))
    return print_label(opts.get_string("label"));

  if(opts.has_opt("correlate"))
    return correlate(opts);
  
  return false;
}

int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);
  Options opts;
  if(!init_tool(argc, argv, &opts))
    return 1;

  return handle(opts);
}
