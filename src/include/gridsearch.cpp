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

static std::string METRIC = "MFCC";
static int MAX_PER_DELTA = 100;

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

  double doCompare(ResynthParams* params,
                   const std::vector<PhonemeInstance>& input,
                   std::vector<int>& outputPath) {
    if(!params->compare)
      return 0;
    auto index = params->index;
    std::vector<PhonemeInstance> output = crf.alphabet().to_phonemes(outputPath);
    auto SWS = SpeechWaveSynthesis(output, input, crf.alphabet());
    Wave resultSignal = SWS.get_resynthesis_td();
    auto& frames = *(params->precompFrames);

    return Comparisons::compare(resultSignal, frames[index], METRIC);
  }

  template<class Functions>
  void findPaths(ResynthParams* params) {
    auto index = params->index;
    const auto& input = corpus_test.input(index);
    std::vector<int> path;

    auto bestValues = traverse_automaton<Functions,
                                         CRF, 2>(input, crf, crf.lambda, &path);
    auto cmp = 0.0;
    if(params -> compare)
      cmp = doCompare(params, input, path);
    params->result = {
      .cmp = cmp,
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

  struct SearchBase {
    SearchBase(): stop(false) { }
    TrainingOutputs outputAtLastPoint;
    bool stop;

    template<class Function>
    std::pair<double, TrainingOutputs>
    locateStep(const Params& current,
               const Params& delta,
               coefficient kLowerBound,
               coefficient kUpperBound,
               Function f,
               const TrainingOutputs& outputAtCurrentParams) {
      auto constexpr Epsilon = 0.000000001;
      auto top = kUpperBound,
        bottom = kLowerBound;

      INFO("Searching k in " << bottom << " " << top << ":");

      auto searchIn = [](double bottom, double top) {
        return (bottom + top) / 2;
      };
      auto currentK = searchIn(bottom, top);
      TrainingOutputs outputAtCurrentK;
      auto constexpr MAX_ITS = 20;
      auto i = 0u;
      while (i++ < MAX_ITS && std::abs(top - bottom) >= Epsilon) {
        currentK = searchIn(bottom, top);
        outputAtCurrentK = f(current + currentK * delta, false);

        if(outputAtCurrentK == outputAtCurrentParams) {
          bottom = currentK;
          (std::cerr << "-").flush();
        } else {
          top = currentK;
          (std::cerr << "_").flush();
        }
      }
      std::cerr << std::endl;
      INFO("k = " << top);
      return std::make_pair(top, f(current + top * delta, true));
    }

    template<class Function>
    std::pair<coefficient, coefficient>
    stepSize(TrainingOutputs& atCurrent, Function f,
             const Params& delta, const Params& current) {
      auto atDeltaMax = f.findMinOrMax(delta, findPaths<MinPathFindFunctions>, false);

      CompareAccumulator<cost, int, true> acc;
      for(auto i = 0u; i < atCurrent.size(); i++) {
        auto& yHat = atCurrent[i].path;
        auto& yMax = atDeltaMax[i].path;
        if(yHat == yMax) {
          INFO("yHat == yMax");
          continue;
        }

        auto delta_Ymax = atDeltaMax[i].pathCost();
        auto delta_yHat = f.costOf(yHat, delta, i);
        auto theta_yHat = atCurrent[i].pathCost();
        auto theta_yMax = f.costOf(yMax, current, i);

        auto kBound = (theta_yMax - theta_yHat) / (delta_yHat - delta_Ymax);
        if(kBound > 0 &&
           theta_yMax - theta_yHat != 0 &&
           delta_yHat - delta_Ymax)
          acc.compare(kBound, i);

        INFO("Delta.Ymax = " << delta_Ymax);
        INFO("Delta.Yhat = " << delta_yHat);
        INFO("Delta.Yhat - Delta.Ymax = " << delta_yHat - delta_Ymax);

        INFO("Theta.Yhat = " << theta_yHat);
        INFO("Theta.Ymax = " << theta_yMax);
        INFO("Theta.Ymax - Theta.Yhat = " << theta_yMax - theta_yHat);

        INFO("Kbound = " << kBound);
      }

      INFO("k_max = " << acc.bestValue << " at index " << acc.bestIndex);
      return std::make_pair(0, acc.bestValue);
    }

    template<class Function>
    std::pair<double, TrainingOutputs>
    findMinimalStep(TrainingOutputs& atCurrent, Function f,
                    const Params& delta, const Params& current) {
      auto kBounds = stepSize(atCurrent, f, delta, current);
      auto kPair = locateStep(current, delta, kBounds.first, kBounds.second, f, atCurrent);
      outputAtLastPoint = kPair.second;
      return kPair;
    }
  };

  struct BruteSearch : public SearchBase {
    BruteSearch(int maxPasses): SearchBase(), maxPasses(maxPasses) { }

    int maxPasses, pass = 0, nextAxis = 0;

    template<class Function>
    cost bootstrap(Ranges& ranges, Params& current,
                   Params& delta, Params& p_delta, Function f) {
      for(auto i = 0u; i < ranges.size(); i++) {
        current[i] = 1;
        delta[i] = p_delta[i] = 0;
      }
      delta[1] = 1;
      outputAtLastPoint = f(current);
      return outputAtLastPoint.value();
    }

    template<class Function>
    cost nextStep(Function f, Ranges& ranges, Params& current,
                  Params& delta, Params&) {
      auto axis = nextAxis;
      if(axis == 0)
        pass++;
      if(pass > maxPasses)
        stop = true;
      nextAxis = (nextAxis + 1) % ranges.size();
      INFO("--- " << ranges[axis].feature);

      std::fill(std::begin(delta), std::end(delta), 0);

      TrainingOutputs bestOutput = outputAtLastPoint;
      auto bestParams = current;
      auto doNextStep = [&]() {
        auto newParams = current;
        for (auto iteration = 0; iteration < MAX_PER_DELTA; iteration++) {
          auto stepPair = findMinimalStep(outputAtLastPoint, f, delta, newParams);
          auto k = stepPair.first;
          newParams = newParams + (k * delta);

          if(k != 0) {
            if(bestOutput.value() > stepPair.second.value()) {
              bestOutput = stepPair.second;
              bestParams = newParams;
            }
          } else {
            break;
          }
        }
      };

      delta[axis] = 1;
      doNextStep();
      delta[axis] = -1;
      doNextStep();

      outputAtLastPoint = bestOutput;
      current = bestParams;
      return bestOutput.value();
    }
  };

  struct DescentSearch : public SearchBase {
    DescentSearch(): SearchBase(), lastResult(-1) { }
    cost lastResult;

    template<class Function>
    cost bootstrap(Ranges& ranges, Params& current,
                   Params& delta, Params& p_delta, Function f) {
      for(auto i = 0u; i < ranges.size(); i++) {
        current[i] = 0.0001;
        delta[i] = p_delta[i] = 0;
      }

      delta[0] = 1;
      outputAtLastPoint = f(current);
      lastResult = outputAtLastPoint.value();
      return outputAtLastPoint.value();
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
        INFO("--- " << ranges[axisIndex].feature);
        std::fill(std::begin(delta), std::end(delta), 0);
        delta[axisIndex] = 1;

        auto atCurrent = outputAtLastPoint;
        auto stepPair = findMinimalStep(atCurrent, f, delta, current);
        auto k = stepPair.first;
        auto plusKValue = stepPair.second.value();

        INFO("At Theta " << lastResult << ", At DeltaPlusK " << plusKValue);
        if(plusKValue < lastResult) {
          current += k * delta;
          INFO("Choose k " << k);
          result = plusKValue;
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
                     int maxIterations) {
    Params current = make_params(),
      bestParams = make_params(),
      delta = make_params(),
      p_delta = make_params();

    auto iteration = 1;
    LOG(" --- Iteration " << iteration++);
    auto result = state.bootstrap(ranges, current, delta, p_delta, f);
    auto bestResult = result;

    LOG(" --- Value " << result);
    bestParams = current;

    while(iteration <= maxIterations && !state.stop) {
      LOG(" --- Iteration " << iteration++);

      result = state.nextStep(f, ranges, current, delta, p_delta);

      LOG(" --- Value " << result);
      if(!state.stop && result < bestResult) {
        bestResult = result;
        bestParams = current;
      }
      for(auto i = 0u; i < ranges.size(); i++)
        LOG(ranges[i].feature << "=" << current[i]);
    }

    for(auto i = 0u; i < ranges.size(); i++)
      ranges[i].current = bestParams[i];
    return bestResult;
  }

  struct TrainingFunction {
    TrainingFunction(std::vector< std::vector<FrameFrequencies> >& precomputed,
                     ThreadPool& tp,
                     Ranges& ranges,
                     const CRF::Values& norms)
      : precomputed(precomputed),
        tp(tp),
        ranges(ranges),
        norms(norms)
    { }

    std::vector< std::vector<FrameFrequencies> > & precomputed;
    ThreadPool& tp;
    Ranges& ranges;
    CRF::Values norms;

    template<class Params>
    void set_params(const Params& params) const {
      for(auto i = 0u; i < ranges.size(); i++)
        crf.set(ranges[i].feature, params[i] / norms[i]);
    }

    template<class TaskParams, class Params>
    TrainingOutputs get_outputs(TaskParams& taskParams, const Params& params) const {
      TrainingOutputs outputs;
      std::for_each(taskParams.begin(), taskParams.end(), [&](ResynthParams& p) {
          outputs.push_back(p.result);
        });
      VLOG << "Params:";
      for(auto i = 0u; i < ranges.size(); i++)
        VLOG << '\t' << ranges[i].feature << "=" << params[i];
      VLOG << std::endl;

      auto i = 0;
      for(auto& to : outputs) {
        VLOG << i << ':';
        for(auto y : to.path)
          VLOG << '\t' << y;
        VLOG << "\t=" << to.cmp;
        VLOG << std::endl;
      }
      return outputs;
    }

    template<class Params, class Func>
    TrainingOutputs findMinOrMax(const Params& params, Func f, bool compare=true) const {
      set_params(params);
      auto count = corpus_test.size();
      bool flags[count];
      std::fill(flags, flags + count, 0);
      auto taskParams = std::vector<ResynthParams>(count);
      for(auto i = 0u; i < count; i++) {
        taskParams[i].init(i, &flags[i], &precomputed, compare);
        tp.add_task(new ParamTask<ResynthParams>(f, &taskParams[i]));
      }
      wait_done(flags, count);
      return get_outputs(taskParams, params);
    }

    template<class Params>
    cost costOf(const std::vector<int>& path, const Params& params, int index) {
      auto phons = crf.alphabet().to_phonemes(path);
      set_params(params);
      return concat_cost<CRF>(phons, crf, crf.lambda, corpus_test.input(index));
    }

    template<class Params>
    TrainingOutputs operator()(const Params& params, bool compare=true) const {
      return findMinOrMax(params, findPaths<MinPathFindFunctions>, compare);
    }
  };

  int train(const Options& opts) {
    Progress::enabled = false;

    METRIC = opts.get_opt<std::string>("metric", "MFCC");
    MAX_PER_DELTA = opts.get_opt<unsigned>("max-per-delta", 100);
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

    auto i = 0u;
    CRF::Values norms;
    for(auto& it : ranges) {
      auto norm = opts.get_opt<double>("norm-" + it.feature, 1.0);
      norms[i++] = norm;
    }

    // Pre-compute FFTd frames of source signals
    std::vector< std::vector<FrameFrequencies> > precompFrames(corpus_test.size());

    precomputeFrames(precompFrames, tp);
    INFO("Done");

    auto Function = TrainingFunction(precompFrames, tp, ranges, norms);

    auto searchAlgo = BruteSearch(opts.get_opt<unsigned>("training-passes", 3));
    //auto searchAlgo = DescentSearch();
    auto result = descentSearch(searchAlgo, Function, ranges,
                                opts.get_opt<int>("max-iterations", 9999999));

    INFO("Best at: ");
    for(auto& r : ranges)
      LOG(r.feature << "=" << r.current);
    INFO("Value: " << result);

    return 0;
  }
}
