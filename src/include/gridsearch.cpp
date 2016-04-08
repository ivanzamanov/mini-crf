#include<algorithm>
#include<chrono>
#include<thread>
#include<valarray>
#include<utility>
#include<cmath>

#include"gridsearch.hpp"
#include"threadpool.h"
#include"crf.hpp"
#include"tool.hpp"

//#define DEBUG_TRAINING

static constexpr auto FC = PhoneticFeatures::size;
typedef std::valarray<coefficient> Params;

Params make_params() {
  return Params(FC);
}

struct Range {
  Range(): Range("", 0, 0, 1) { }
  Range(std::string feature, double from, double to, double step)
    :from(from), to(to), step(step), current(from), feature(feature) {
    assert(step > 0 && from <= to);
  }

  bool has_next() const { return (current + step) <= to; }
  void next() { current += step; }

  void reset() { current = from; }

  double from, to, step, current;
  std::string feature;

  std::string to_string() const {
    std::stringstream str;
    str << "[" << feature << ", " << from << ", " << to << ", " << step << "]";
    return str.str();
  }
};
typedef std::array<Range, FC> Ranges;

namespace gridsearch {

  std::string to_text_string(const std::vector<PhonemeInstance>& vec) {
    std::string result(1, ' ');
    for(auto it = vec.begin(); it != vec.end(); it++) {
      result.push_back('|');
      result.append(labels_all.convert((*it).label) );
      result.push_back('|');
    }
    return result;
  }

  Comparisons doCompare(ResynthParams* params,
                         const std::vector<PhonemeInstance>& input,
                         std::vector<int>& outputPath) {
    auto index = params->index;
    std::vector<PhonemeInstance> output = crf.alphabet().to_phonemes(outputPath);
    auto SWS = SpeechWaveSynthesis(output, input, crf.alphabet());
    Wave resultSignal = SWS.get_resynthesis_td();
    auto& frames = *(params->precompFrames);

    Comparisons result(0, compare_LogSpectrum(resultSignal, frames[index]));
    return result;
  }

  void doResynthIndex(ResynthParams* params) {
    auto index = params->index;
    const auto& input = corpus_test.input(index);
    std::vector<int> path;

    auto bestValues = traverse_automaton<MinPathFindFunctions,
                                         CRF, 2>(input, crf, crf.lambda, &path);
    auto result = doCompare(params, input, path);
    params->result = {
      .cmp = result,
      .bestValues = bestValues,
      .path = path
    };
  }

  void resynthIndex(ResynthParams* params) {
    doResynthIndex(params);
    *(params->flag) = true;
  }

  void findMaxPaths(ResynthParams* params) {
    auto index = params->index;
    const auto& input = corpus_test.input(index);
    std::vector<int> path;

    auto bestValues = traverse_automaton<MaxPathFindFunctions,
                                         CRF, 2>(input, crf, crf.lambda, &path);
    params->result = {
      .cmp = Comparisons(),
      .bestValues = bestValues,
      .path = path
    };
    *(params->flag) = true;
  }

  void findMinPaths(ResynthParams* params) {
    auto index = params->index;
    const auto& input = corpus_test.input(index);
    std::vector<int> path;

    auto bestValues = traverse_automaton<MinPathFindFunctions,
                                         CRF, 2>(input, crf, crf.lambda, &path);
    params->result = {
      .cmp = Comparisons(),
      .bestValues = bestValues,
      .path = path
    };
    *(params->flag) = true;
  }

  void wait_done(bool* flags, unsigned count) {
    auto done = false;
    while(!done) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      done = true;
      for(unsigned i = 0; i < count; i++)
        done &= flags[i];
      DEBUG(
            cerr << "Unfinished: ";
            for(unsigned i = 0; i < count; i++)
              if(!flags[i])
                cerr << i << " ";
            cerr << std::endl;
            );
    }
  }

  struct FFTPrecomputeParams {
    void init(int index, bool* flag, std::string file, std::vector< std::vector<FrameFrequencies> >* precompFrames) {
      this->index = index;
      this->flag = flag;
      this->file = file;
      this->precompFrames = precompFrames;
    }

    std::string file;
    int index;
    bool* flag;

    std::vector< std::vector<FrameFrequencies> >* precompFrames;
  };

  void precomputeSingleFrames(FFTPrecomputeParams* params) {
    auto SWS = SpeechWaveSynthesis(corpus_test.input(params->index),
                                   corpus_test.input(params->index), alphabet_test);
    auto sourceSignal = SWS.get_resynthesis_td();
    auto frames = toFFTdFrames(sourceSignal);

    (*params->precompFrames)[params->index] = frames;
    *(params->flag) = 1;
  }

  void precomputeFrames(std::vector< std::vector<FrameFrequencies> >& precompFrames,
                        ThreadPool& tp) {
    INFO("Precomputing FFTs");
    auto count = corpus_test.size();
    bool flags[count];
    std::fill(flags, flags + count, false);

    auto params = new FFTPrecomputeParams[count];
    for(auto i = 0u; i < count; i++) {
      FileData fileData = alphabet_test.file_data_of(corpus_test.input(i)[0]);

      params[i].init(i, &flags[i], fileData.file, &precompFrames);

      tp.add_task(new ParamTask<FFTPrecomputeParams>(&precomputeSingleFrames, &params[i]));
    }
    wait_done(flags, count);

    delete[] params;
  }

  void csvPrintHeaders(std::ofstream& csvOutput, std::string& csvFile,
                       Ranges& ranges) {
    if(csvFile != std::string("")) {
      util::join_output(csvOutput, ranges, [](const Range& r) { return r.feature; }, " ")
        << " value" << std::endl;
    }
  }

  void csvPrint(std::ofstream& csvOutput, std::string& csvFile,
                Ranges& ranges, cost result) {
    if(csvFile != std::string("")) {
      util::join_output(csvOutput, ranges, [](const Range& r) { return r.current; }, " ")
        << " " << result << std::endl;
    }
  }

  struct BruteSearch {
    BruteSearch(int maxPasses): maxPasses(maxPasses), stop(false) { }

    int maxPasses, pass = 0;
    bool stop;

    void bootstrap(Ranges& ranges, Params& current,
                   Params& delta, Params& p_delta) {
      for(auto i = 0u; i < ranges.size(); i++) {
        current[i] = ranges[i].from;
        delta[i] = p_delta[i] = 0;
      }
      current[0] = 1;
      delta[1] = 1;
    }

    template<class Function>
    cost nextStep(Function f, Ranges& ranges, Params& current,
                         Params& delta, Params&) {
      // first non-0 index
      auto i = 0u;
      while(i < delta.size() && delta[i] == 0)
        i++;

      if(current[i] < ranges[i].to) {
        // Do nothing, step and axis don't change
        INFO(ranges[i].feature << ": " << current[i] << " -> " << (current[i] + delta[i]));
      } else {
        // Stop moving along this axis
        delta[i] = 0;
        // Pick next axis
        auto nextIndex = (i + 1) % delta.size();
        if(nextIndex == 0) {// If a new round has begun...
          pass++;
          if(pass > maxPasses) {
            stop = true;
            return 0;
          }
        }

        // Pick density of new axis
        delta[nextIndex] = ranges[nextIndex].step;
        // Reset value along new axis
        current[nextIndex] = ranges[nextIndex].from;

        INFO(ranges[i].feature << " -> " << ranges[nextIndex].feature);
      }

      current += delta;
      return f(current);
    }
  };

  template<class Diffs>
  void printDiffs(Diffs diffs) {
    INFO("Diffs:");
    auto flag = false;
    for(auto& d : diffs) {
      std::cerr << "[" << d.first << "," << d.second << "]" << " ";
      flag = true;
    }
    if(flag) std::cerr << std::endl;
  }

  struct DescentSearch {
    DescentSearch(): stop(false), lastResult(-1) { }
    bool stop;
    cost lastResult;

    cost findSomeOtherValue(vector<int>& excludedPath, Params& params, int index) {
      vector<int> somePath(excludedPath);
      auto someIndex = somePath.size() / 2;
      auto phonId = somePath[someIndex];
      auto matching = alphabet_synth.get_class(alphabet_synth.fromInt(phonId));
      for(auto& m : matching)
        if(m != phonId) {
          somePath[someIndex] = m;
          break;
        }

      auto y = alphabet_synth.to_phonemes(somePath);
      CRF::Values param_vals;
      for(auto i = 0u; i < param_vals.size(); i++)
        param_vals[i] = params[i];
      return concat_cost(y, crf, param_vals, corpus_test.input(index));
    }

    template<class Function>
    std::pair<coefficient, coefficient>
    stepSize(TrainingOutputs& atCurrent, Function f, Params& delta, Params& current) {
      std::vector<cost> quotients;
      for(auto& output : atCurrent) {
        auto val = output.bestValues[1] - output.bestValues[0];
        assert(val >= 0);
        quotients.push_back(val);
      }

      auto atDeltaMax = f.findMinOrMax(delta, findMaxPaths);
      auto atDeltaMin = f.findMinOrMax(delta, findMinPaths);
      for(auto i = 0u; i < atDeltaMax.size(); i++)
        assert(atDeltaMin[i].bestValues[0] < atDeltaMax[i].bestValues[0]);

      CompareAccumulator<cost, int, true> acc;
      auto denomAtMin = 0u;
      for(auto i = 0u; i < atCurrent.size(); i++) {
        auto thetaHatAtDelta = f.costOf(atCurrent[i].path, delta, i);

        auto options = std::array<cost, 4> {{ std::abs(thetaHatAtDelta - atDeltaMin[i].bestValues[0]),
                                              std::abs(thetaHatAtDelta + atDeltaMax[i].bestValues[0]),
                                              std::abs(thetaHatAtDelta + atDeltaMin[i].bestValues[0]),
                                              std::abs(thetaHatAtDelta - atDeltaMax[i].bestValues[0]) }};

        auto quot = quotients[i];
        auto denom = (*std::max_element(std::begin(options), std::end(options)));
        auto val = quot / denom;
        if(acc.compare(val, i))
          denomAtMin = denom;
      }

      INFO("k_min = " << acc.bestValue);
      auto someOtherValue = findSomeOtherValue(atCurrent[acc.bestIndex].path,
                                               current, acc.bestIndex) / denomAtMin;
      return std::make_pair(acc.bestValue, someOtherValue);
    }

    void bootstrap(Ranges& ranges, Params& current,
                   Params& delta, Params& p_delta) {
      for(auto i = 0u; i < ranges.size(); i++) {
        current[i] = 0.0001;
        delta[i] = p_delta[i] = 0;
      }

      delta[0] = 1;
    }

    template<class Function>
    CompareAccumulator<double, double>
    locateStep(const Params& current,
               const Params& delta,
               coefficient kLowerBound,
               coefficient kUpperBound,
               Function f,
               TrainingOutputs& outputAtCurrentParams,
               int mult) {
      auto epsilon = 1;

      auto top = kUpperBound * mult,
        bottom = kLowerBound * mult;
      auto valueAtCurrentParams = outputAtCurrentParams.value();
      CompareAccumulator<double, double> acc;
      acc.add(valueAtCurrentParams, 0);

      assert( acc.add(f(current + top * delta).value(), top)
              != acc.add(f(current + bottom * delta).value(), bottom)
              );

      TrainingOutputs outputAtLastK = outputAtCurrentParams;
      INFO("Searching k in " << bottom << " " << top);

      while (std::abs(top - bottom) >= epsilon) {
        (std::cerr << ".").flush();
        auto currentK = (top + bottom) / 2;
        auto outputAtCurrentK = f(current + currentK * delta);
        auto valueAtCurrentK = outputAtCurrentK.value();
        acc.compare(valueAtCurrentK, currentK);

        if(std::abs(valueAtCurrentK - valueAtCurrentParams) > 0.0001)
          top = currentK;
        else
          bottom = currentK;

        //printDiffs(outputAtLastK.findDifferences(outputAtCurrentK));
        outputAtLastK = outputAtCurrentK;
      }
      std::cerr << std::endl;
      INFO("Best k = " << acc.bestIndex);
      INFO("F: " << valueAtCurrentParams << " -> " << acc.bestValue);
      return acc;
    }

    template<class Function>
    cost nextStep(Function f, Ranges& ranges,
                  Params& current, Params& delta, Params&) {
      // first non-0 index
      auto i = 0u;
      while(i < delta.size() && delta[i] == 0)
        i++;

      auto tries = 0u;
      cost result = -1;
      INFO("Last result = " << lastResult);
      while(tries < current.size() && (lastResult >= result)) {
        auto axisIndex = (i + tries++) % delta.size();

        std::fill(std::begin(delta), std::end(delta), 0);
        delta[axisIndex] = 1;

        auto feature = ranges[axisIndex].feature;

        // A list of minimums
        auto atCurrent = f(current);
        if(lastResult == -1)
          lastResult = atCurrent.value();
        auto kPair = stepSize(atCurrent, f, delta, current);

        auto k = kPair.first,
          kBound = kPair.second;

        INFO("--- " << feature);
        auto acc = locateStep(current, delta, k, kBound, f, atCurrent, 1);
        acc.compare(locateStep(current, delta, k, kBound, f, atCurrent, -1));

        if(acc.bestValue < lastResult) {
          k = acc.bestIndex;
          current += k * delta;
          INFO("Choose k " << k);
          result = acc.bestValue;
          break;
        } else {
          INFO("Yielded no improvement");
        }
      }

      if(lastResult == result) { INFO("No improvement") };
      if(result < 0) { INFO("Negative " << result) };
      stop = result < 0 || (lastResult == result);
      lastResult = result;
      return result;
    }
  };

  template<class State, class Function>
  cost descentSearch(State state,
                     Function& f,
                     Ranges& ranges,
                     int maxIterations,
                     std::string csvFile) {
    std::ofstream csvOutput(csvFile);
    csvPrintHeaders(csvOutput, csvFile, ranges);

    Params current = make_params(),
      bestParams = make_params(),
      delta = make_params(),
      p_delta = make_params();

    state.bootstrap(ranges, current, delta, p_delta);
    auto iteration = 1;
    LOG(" --- Iteration " << iteration++);
    auto result = f(current).value(),
      bestResult = result;

    LOG(" --- Value " << result);
    bestParams = current;

    while(iteration <= maxIterations && !state.stop) {
      LOG(" --- Iteration " << iteration++);
      csvPrint(csvOutput, csvFile, ranges, result);

      result = state.nextStep(f, ranges, current, delta, p_delta);

      LOG(" --- Value " << result);
      if(!state.stop && result < bestResult) {
        bestResult = result;
        bestParams = current;

        for(auto i = 0u; i < ranges.size(); i++)
          LOG(ranges[i].feature << "=" << current[i]);
      }
    }

    for(auto i = 0u; i < ranges.size(); i++)
      ranges[i].current = bestParams[i];
    return bestResult;
  }

  struct TrainingFunction {
    TrainingFunction(std::vector< std::vector<FrameFrequencies> >& precomputed,
                     ThreadPool& tp,
                     Ranges& ranges): precomputed(precomputed),
                                      tp(tp),
                                      ranges(ranges)
    { }

    std::vector< std::vector<FrameFrequencies> > & precomputed;
    ThreadPool& tp;
    Ranges& ranges;

    template<class Params>
    TrainingOutputs operator()(const Params& params) const {
#ifdef DEBUG_TRAINING
      return Comparisons::dummy();
#endif

      for(auto i = 0u; i < ranges.size(); i++)
        crf.set(i, params[i]);

      auto count = corpus_test.size();
      bool flags[count];
      std::fill(flags, flags + count, 0);
      auto taskParams = std::vector<ResynthParams>(count);
      for(unsigned i = 0; i < count; i++) {
        taskParams[i].init(i, &flags[i], &precomputed);
        tp.add_task(new ParamTask<ResynthParams>(&resynthIndex, &taskParams[i]));
      }
      wait_done(flags, count);

      TrainingOutputs outputs;
      std::for_each(taskParams.begin(), taskParams.end(), [&](ResynthParams& p) {
          outputs.push_back(p.result);
        });
      return outputs;
    }

    template<class Params, class Func>
    TrainingOutputs findMinOrMax(const Params& params, Func f) {
      for(auto i = 0u; i < ranges.size(); i++)
        crf.set(i, params[i]);

      auto count = corpus_test.size();
      bool flags[count];
      std::fill(flags, flags + count, 0);
      auto taskParams = std::vector<ResynthParams>(count);
      for(unsigned i = 0; i < count; i++) {
        taskParams[i].init(i, &flags[i], &precomputed);
        tp.add_task(new ParamTask<ResynthParams>(f, &taskParams[i]));
      }
      wait_done(flags, count);

      TrainingOutputs outputs;
      std::for_each(taskParams.begin(), taskParams.end(), [&](ResynthParams& p) {
          outputs.push_back(p.result);
        });
      return outputs;
    }

    template<class Params>
    cost costOf(const std::vector<int>& path, const Params& params, int index) {
      auto phons = crf.alphabet().to_phonemes(path);
      CRF::Values param_vals;
      for(auto i = 0u; i < param_vals.size(); i++)
        param_vals[i] = params[i];
      return concat_cost<CRF>(phons, crf, param_vals, corpus_test.input(index));
    }
  };

  int train(const Options& opts) {
    Progress::enabled = false;

    //#pragma omp parallel for
    ThreadPool tp(opts.get_opt<int>("thread-count", 8));
    if (tp.initialize_threadpool() < 0) {
      ERROR("Failed to initialize thread pool");
      return -1;
    }

    Ranges ranges;
    if(opts.has_opt("ranges")) {
      auto names = util::split_string(opts.get_string("ranges"), ',');
      auto i = 0u;
      for(auto& name : names)
        ranges[i++] = Range(name, 0, 1, 0.1);
      assert(i == ranges.size());
    } else {
      ranges = {{
          Range("trans-ctx", 0, 300, 1),
          Range("trans-mfcc", 0, 2, 0.01),
          Range("trans-pitch", 0, 300, 1),
          Range("state-pitch", 0, 300, 1),
          Range("state-duration", 0, 300, 1),
          Range("state-energy", 0, 300, 1)
        }};
    }
    for(auto& it : ranges)
      INFO("Range " << it.to_string());

    // Pre-compute FFTd frames of source signals
    std::vector< std::vector<FrameFrequencies> > precompFrames(corpus_test.size());

#ifndef DEBUG_TRAINING
    precomputeFrames(precompFrames, tp);
#endif
    INFO("Done");

    auto Function = TrainingFunction(precompFrames, tp, ranges);

    //auto searchAlgo = BruteSearch(opts.get_opt<unsigned>("training-passes", 3));
    auto searchAlgo = DescentSearch();
    auto result = descentSearch(searchAlgo, Function, ranges,
                                opts.get_opt<int>("max-iterations", 9999999),
                                opts.get_string("csv-file"));

    INFO("Best at: ");
    for(auto& r : ranges)
      LOG(r.feature << "=" << r.current);
    INFO("Value: " << result);

    return 0;
  }
}
