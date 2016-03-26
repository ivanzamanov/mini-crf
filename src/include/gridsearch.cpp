#include<algorithm>
#include<chrono>
#include<thread>
#include<valarray>
#include<utility>

#include"gridsearch.hpp"
#include"threadpool.h"
#include"crf.hpp"
#include"tool.hpp"
#include"parser.hpp"

//#define DEBUG_TRAINING

extern double hann(double i, int size);

static bool APPLY_WINDOW_CMP = false;
static float WINDOW_OVERLAP = 0.1;

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

template<int FeatureCount>
struct _ValueCache {
  _ValueCache(std::string path): path(path) {
    init();
  }

  std::string path;
  std::vector<gridsearch::Comparisons> values;
  std::vector<std::array<double, FeatureCount> > args;

  bool load(std::array<Range, FeatureCount>& ranges, gridsearch::Comparisons& result) const {
    for(unsigned i = 0; i < args.size(); i++) {
      bool found = true;
      for(unsigned j = 0; j < args[i].size(); j++)
        found = found && (args[i][j] == ranges[j].current);
      if(found) {
        result = values[i];
        return true;
      }
    }
    return false;
  }

  void save(std::array<Range, FeatureCount>& ranges, const gridsearch::Comparisons& result) {
    values.push_back(result);
    std::array<double, FeatureCount> new_args;
    for(unsigned i = 0; i < ranges.size(); i++)
      new_args[i] = ranges[i].current;
    args.push_back(new_args);
  }

  void persist() {
    std::ofstream str(path);
    BinaryWriter w(&str);
    unsigned size = values.size();
    w << size;
    for(unsigned i = 0; i < size; i++) {
      for(unsigned j = 0; j < FeatureCount; j++)
        w << args[i][j];
      //w << values[i].ItakuraSaito;
      w << values[i].LogSpectrum;
    }
    INFO("Persisted " << size << " values in " << path);
  }

private:
  void init() {
    std::ifstream str(path);
    BinaryReader reader(&str);
    unsigned size; reader >> size;
    if(!reader.ok()) {
      INFO("No existing value cache");
      return;
    }
    INFO("Values in cache " << path << " " << size);
    for(unsigned i = 0; i < size; i++) {
      std::array<double, FeatureCount> args;
      gridsearch::Comparisons values;
      double arg;
      for(unsigned j = 0; j < FeatureCount; j++) {
        reader >> arg;
        args[j] = arg;
      }
      reader >> values.LogSpectrum;
      this->args.push_back(args);
      this->values.push_back(values);
    }
  }
};
typedef _ValueCache<FC> ValueCache;

namespace gridsearch {

  std::vector<FrameFrequencies> toFFTdFrames(Wave& wave) {
    auto frameOffset = 0u;
    std::vector<FrameFrequencies> result;

    std::valarray<cdouble> buffer(gridsearch::FFT_SIZE);
    FrameFrequencies values;

    auto overlap = buffer.size() * WINDOW_OVERLAP;
    while(frameOffset + buffer.size() < wave.length()) {

      WaveData frame = wave.extractBySample(frameOffset, frameOffset + buffer.size());
      frameOffset += (frame.size() - overlap);

      std::fill(std::begin(buffer), std::end(buffer), 0);
      for(auto i = 0u; i < frame.size(); i++) {
        buffer[i] = frame[i];
        if(APPLY_WINDOW_CMP)
          buffer[i] *= hann(i, frame.size());
      }

      ft::fft(buffer);
      for(auto j = 0u; j < values.size(); j++)
        values[j] = buffer[j];

      result.push_back(values);
    }
    assert(result.size() > 0);
    //INFO(result.size() << " from " << wave.duration() << " in " << wave.length() << " samples");
    return result;
  }

  // TODO
  double compare_IS(Wave& result, Wave& original) {
    return 0;
    double value = 0;
    each_frame(result, original, 0.05, [&](WaveData, WaveData) {
        value++;
      });
    assert(false); // ItakuraSaito not yet implemented
    return value;
  }

  //__attribute__ ((optnone))
  double compare_LogSpectrum(Wave& result, std::vector<FrameFrequencies>& frames2) {
    auto frames1 = toFFTdFrames(result);
    bool check = std::abs((int) frames1.size() - (int) frames2.size()) <= 2;
    if(!check) {
      ERROR(frames1.size() << " and " << frames2.size());
    }
    assert(check);

    int minSize = std::min(frames1.size(), frames2.size());
    double value = 0;
    for(auto j = 0; j < minSize; j++) {
      double diff = 0;
      auto& freqs1 = frames1[j];
      auto& freqs2 = frames2[j];
      for(auto i = 0u; i < freqs1.size(); i++) {
        auto m1 = std::norm(freqs1[i]);
        m1 = m1 ? m1 : 1;
        auto m2 = std::norm(freqs2[i]);
        m2 = m2 ? m2 : 1;
        diff += std::pow( std::log10( m2 / m1), 2 );
      }
      value += diff;
    }
    return value / minSize;
  }

  double compare_LogSpectrum(Wave& result, Wave& original) {
    auto frames2 = toFFTdFrames(original);
    auto value = compare_LogSpectrum(result, frames2);
    //auto value2 = compare_LogSpectrum(result, frames2);
    //assert(value == value2);
    return value;
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

  Comparisons doCompare(ResynthParams* params,
                         const std::vector<PhonemeInstance>& input,
                         std::vector<int>& outputPath) {
    auto index = params->index;
    std::vector<PhonemeInstance> output = crf.alphabet().to_phonemes(outputPath);
    auto SWS = SpeechWaveSynthesis(output, input, crf.alphabet());
    Wave resultSignal = SWS.get_resynthesis_td();
    /*Wave rs2 = SWS.get_resynthesis_td();
    for(auto i = 0u; i < resultSignal.length(); i++)
      assert(resultSignal[i] == rs2[i]);
    */

    auto& frames = *(params->precompFrames);

    Comparisons result(0, compare_LogSpectrum(resultSignal, frames[index]));
    /*auto LS2 = compare_LogSpectrum(resultSignal, frames[index]);
    assert(LS2 == result.value());
    */
    return result;
  }

  void doResynthIndex(ResynthParams* params) {
    auto index = params->index;
    const auto& input = corpus_test.input(index);
    std::vector<int> path;

    auto bestValues = traverse_automaton<MinPathFindFunctions,
                                         CRF, 2>(input, crf, crf.lambda, &path);
    auto result = doCompare(params, input, path);

    /*std::vector<int> path2;
    traverse_automaton<MinPathFindFunctions>(input, crf, crf.lambda, &path2);
    for(auto i = 0u; i < path.size(); i++)
      assert(path[i] == path2[i]);
    */

    /*
    auto result2 = doCompare(params, input, path);
    assert(result.value() == result2.value());
    */

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

  double randDouble() {
    static int c = 0;
    c++;
    return - (c * c) % 357;
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

    template<class Function>
    std::pair<coefficient, coefficient>
    stepSize(TrainingOutputs& atCurrent, Function f, Params& delta) {
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

      cost minValue = 10000000000;
      cost maxQuot = minValue;
      for(auto i = 0u; i < atCurrent.size(); i++) {
        auto thetaHatAtDelta = f.costOf(atCurrent[i].path, delta, i);

        auto options = std::array<cost, 4> {{ std::abs(thetaHatAtDelta - atDeltaMin[i].bestValues[0]),
                                              std::abs(thetaHatAtDelta + atDeltaMax[i].bestValues[0]),
                                              std::abs(thetaHatAtDelta + atDeltaMin[i].bestValues[0]),
                                              std::abs(thetaHatAtDelta - atDeltaMax[i].bestValues[0]) }};

        auto quot = quotients[i];
        auto denom = (*std::max_element(std::begin(options), std::end(options)));
        auto val = quot / denom;

        if(val < minValue) minValue = val;

        if(maxQuot > quotients[i])
          maxQuot = quotients[i];
      }

      INFO("k = " << minValue);
      return std::make_pair(minValue, maxQuot);
    }

    void bootstrap(Ranges& ranges, Params& current,
                   Params& delta, Params& p_delta) {
      for(auto i = 0u; i < ranges.size(); i++) {
        current[i] = (ranges[i].from + ranges[i].to) / 2;
        delta[i] = p_delta[i] = 0;
      }

      delta[0] = 1;
    }

    template<class Function>
    std::pair<cost, coefficient>
    locateStep(const Params& current,
               const Params& delta,
               coefficient kLowerBound,
               coefficient kUpperBound,
               Function f,
               TrainingOutputs& outputAtCurrentParams,
               int mult) {
      auto epsilon = 1;

      auto top = kUpperBound,
        bottom = kLowerBound;
      auto valueAtCurrentParams = outputAtCurrentParams.value();
      auto bestValue = valueAtCurrentParams;
      auto bestK = kLowerBound;

      TrainingOutputs outputAtLastK = outputAtCurrentParams;
      while (top - bottom >= epsilon) {
        INFO("Searching k in " << bottom << " " << top);
        auto currentK = (top + bottom) / 2;
        auto outputAtCurrentK = f(current + currentK * mult * delta);
        auto valueAtCurrentK = outputAtCurrentK.value();
        INFO("k: " << currentK << " -> " << valueAtCurrentK);

        if(valueAtCurrentK < bestValue) {
          bestValue = valueAtCurrentK;
          bestK = currentK;
        }

        if(std::abs(valueAtCurrentK - valueAtCurrentParams) > 0.0001) {
          INFO("diff = " << std::abs(valueAtCurrentK - valueAtCurrentParams));
          top = currentK;
        }
        else
          bottom = currentK;

        printDiffs(outputAtLastK.findDifferences(outputAtCurrentK));
        outputAtLastK = outputAtCurrentK;
      }
      return std::make_pair(bestK * mult, bestValue);
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
        auto prevValue = current[axisIndex];

        // A list of minimums
        auto atCurrent = f(current);
        auto kPair = stepSize(atCurrent, f, delta);

        auto k = kPair.first,
          kBound = kPair.second;

        INFO("--- " << feature);
        auto plus = locateStep(current, delta, k, kBound, f, atCurrent, 1);
        auto minus = locateStep(current, delta, k, kBound, f, atCurrent, -1);

        auto k1 = plus.first,
          k2 = minus.first;

        auto v1 = plus.second,
          v2 = minus.second;

        INFO((prevValue + k1) << ", " << v1 <<
             " vs " <<
             (prevValue - k2) << ", " << v2);
        if(v1 < v2) {
          INFO(feature << ": " <<
               prevValue << " -> " << prevValue + k);
          current += k1 * delta;
          result = v1;
          break;
        } else if (v2 < v1) {
          INFO(feature << ": " <<
               prevValue << " -> " << prevValue - k);
          current -= k * delta;
          result = v2;
          break;
        }
      }

      lastResult = result;
      stop = result < 0 || (lastResult == result);
      return result;
    }
  };

  template<class State, class Function>
  cost descentSearch(State state,
                     Function& f,
                     Ranges& ranges,
                     int maxIterations,
                     ValueCache&,
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
      return Comparisons().dummy(randDouble());
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
    APPLY_WINDOW_CMP = opts.get_opt("apply-window-cmp", false);
    WINDOW_OVERLAP = opts.get_opt("window-overlap", 0.1);
    ValueCache vc(opts.get_opt<std::string>("value-cache", "value-cache.bin"));

    //#pragma omp parallel for
    ThreadPool tp(opts.get_opt<int>("thread-count", 8));
    if (tp.initialize_threadpool() < 0) {
      ERROR("Failed to initialize thread pool");
      return -1;
    }

    Ranges ranges = {{
        Range("trans-ctx", 0, 300, 1),
        Range("trans-pitch", 0, 300, 1),
        Range("state-pitch", 0, 300, 1),
        Range("trans-mfcc", 0, 2, 0.01),
        Range("state-duration", 0, 300, 1),
        Range("state-energy", 0, 300, 1)
      }};
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
                                vc,
                                opts.get_string("csv-file"));

    INFO("Best at: ");
    for(auto& r : ranges)
      LOG(r.feature << "=" << r.current);
    INFO("Value: " << result);

    return 0;
  }
}
