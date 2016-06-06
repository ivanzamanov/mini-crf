#include<algorithm>

#include"gridsearch.hpp"
#include"tool.hpp"
#include"features.hpp"

using namespace tool;
using std::vector;
using std::pair;
using std::string;

bool Progress::enabled = true;

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
bool print_stats_func_edge(const PhonemeAlphabet& alphabet, const Func f, const std::string displayName) {
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

void compare_maxes(CRF::Values& maxes, const CRF::Values& newVals) {
  for(auto i = 0u; i < maxes.size(); i++)
    maxes[i] = std::max(maxes[i], newVals[i]);
}

bool print_stats() {
  vector<std::pair<PhoneticLabel, PhoneticLabel> > presentLabelPairs;
  vector<PhoneticLabel> presentLabels;
  for(auto index = 0u; index < corpus_test.size(); index++) {
    const auto& labels = corpus_test.label(index);

    for(auto label : labels) {
      bool notPresent = std::find(presentLabels.begin(), presentLabels.end(),
                                  label.label) == presentLabels.end();
      if(notPresent)
        presentLabels.push_back(label.label);
    }

    for(auto i = 1u; i < labels.size(); i++) {
      auto pair = std::make_pair(labels[i - 1].label, labels[i].label);
      auto notPresent = std::find(presentLabelPairs.begin(), presentLabelPairs.end(),
                                  pair) == presentLabelPairs.end();
      if(notPresent)
        presentLabelPairs.push_back(pair);
    }
  }

  INFO("Labels: " << presentLabels.size());
  INFO("Label Pairs: " << presentLabelPairs.size());
  FunctionalAutomaton<CRF, MinPathFindFunctions> a(crf);
  std::fill(a.lambda.begin(), a.lambda.end(), 1);

  CRF::Values maxes;
  std::fill(maxes.begin(), maxes.end(), 0);

  INFO("Calculating maximal values...");
  for(auto index = 0u; index < corpus_test.size(); index++) {
    a.x = corpus_test.input(index);

    Progress p(a.x.size(), "Sequence " + std::to_string(index + 1)
               + " of " + std::to_string(corpus_test.size()) + " ");
    int i = a.x.size() - 1;
    auto labels = alphabet_synth.get_class(a.x[i]);
    for(auto& pi : labels) {
      auto vals = a.template calculate_values<false>(alphabet_synth.fromInt(pi),
                                                     alphabet_synth.fromInt(pi),
                                                     i);
      compare_maxes(maxes, vals);
    }
    p.update();

    for(i--; i >= 0; i--) {
      auto labels1 = alphabet_synth.get_class(a.x[i]);
      auto labels2 = alphabet_synth.get_class(a.x[i + 1]);
      for(auto& pi1 : labels1) {
        for(auto& pi2 : labels2) {
          auto vals = a.template calculate_values<true>(alphabet_synth.fromInt(pi1),
                                                        alphabet_synth.fromInt(pi2),
                                                        i);
          compare_maxes(maxes, vals);
        }
      }
      p.update();
    }
    p.finish();
  }

  for(auto i = 0u; i < maxes.size(); i++) {
    std::cerr << "norm-" + CRF::features::Names[i] << "=" << maxes[i] << std::endl;
  }
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

int compare_dbs(const Options& opts) {
  
}

bool handle(const Options& opts) {
  if(opts.has_opt("compare-dbs"))
    return compare_dbs(opts);
  
  string db_type = opts.get_string("db");
  auto* db = &alphabet_synth;
  if(db_type == string("test"))
    db = &alphabet_test;

  if(opts.has_opt("stats"))
    return print_stats();

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

  REPORT_PROGRESS = true;
  return handle(opts);
}
