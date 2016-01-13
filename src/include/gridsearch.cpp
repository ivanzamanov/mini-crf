#include <chrono>
#include <thread>

#include"gridsearch.hpp"
#include"threadpool.h"
#include"crf.hpp"
#include"tool.hpp"
#include"parser.hpp"

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

template<int FeatureCount>
struct ValueCache {
  ValueCache(std::string path): path(path) {
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
    return;
    std::ofstream str(path);
    BinaryWriter w(&str);
    unsigned size = values.size();
    w << size;
    for(unsigned i = 0; i < size; i++) {
      for(unsigned j = 0; j < FeatureCount; j++)
        w << args[i][j];
      w << values[i].ItakuraSaito;
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
      reader >> values.ItakuraSaito;
      reader >> values.LogSpectrum;
      this->args.push_back(args);
      this->values.push_back(values);
    }
  }
};

static const int FC = PhoneticFeatures::ESIZE + PhoneticFeatures::VSIZE;
struct TrainingOutput {
  TrainingOutput(std::array<Range, FC> ranges, gridsearch::Comparisons result)
    :ranges(ranges), result(result)
  { }

  std::array<Range, FC> ranges;
  gridsearch::Comparisons result;
};

namespace gridsearch {

  std::vector<FrameFrequencies> toFFTdFrames(Wave& wave) {
    int length = wave.length();
    int frameOffset = 0;
    std::vector<FrameFrequencies> result;
    FrameFrequencies values;
    int sampleWidth = values.size() * 2 + 1;
    auto td_values = new double[sampleWidth];

    while(frameOffset < length) {
      for(int i = 0; i < sampleWidth; i++) td_values[i] = 0;

      int frameLength = std::min(sampleWidth, length - frameOffset);
      if(frameLength <= 0)
        break;

      WaveData frame = wave.extractBySample(frameOffset, frameOffset + frameLength);
      frameOffset += sampleWidth;
      for(unsigned i = 0; i < frame.size(); i++) td_values[i] = frame[i];

      int binCount = (frameLength - 1) / 2;
      ft::FT(td_values, frame.size(), values, binCount);

      // If the frame is shorter, need to adjust frequency bins
      if((int) frame.size() < sampleWidth) {
        // scale >= 1
        double scale = (double) sampleWidth / frame.size();
        for(int i = 0; i < binCount; i++) {
          cdouble val = values[i];
          values[i] = cdouble(0, 0);
          values[i / scale] += val;
        }
      }
      result.push_back(values);
    }
    delete[] td_values;
    assert(result.size() > 0);
    return result;
  }

  // TODO
  double compare_IS(Wave& result, Wave& original) {
    return 0;
    double value = 0;
    each_frame(result, original, 0.05, [&](WaveData f1, WaveData f2) {
        value++;
      });
    assert(false); // ItakuraSaito not yet implemented
    return value;
  }

  //__attribute__ ((optnone))
  double compare_LogSpectrum(Wave& result, std::vector<FrameFrequencies>& frames2) {
    auto frames1 = toFFTdFrames(result);
    assert(std::abs((int) frames1.size() - (int) frames2.size()) <= 1);

    int minSize = std::min(frames1.size(), frames2.size());
    double value = 0;
    for(auto j = 0; j < minSize; j++) {
      double diff = 0;
      auto& freqs1 = frames1[j];
      auto& freqs2 = frames2[j];
      for(auto i = 0; i < minSize; i++) {
        double m1 = freqs1[i].magn();
        m1 = m1 ? m1 : 1;
        double m2 = freqs2[i].magn();
        m2 = m2 ? m2 : 1;
        diff += std::pow( std::log10( m2 / m1), 2 );
      }
      value += diff;
    }
    return value / minSize;
  }

  double compare_LogSpectrum(Wave& result, Wave& original) {
    auto frames2 = toFFTdFrames(original);
    return compare_LogSpectrum(result, frames2);
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

  void do_resynth_index(ResynthParams* params) {
    int index = params->index;
    std::vector<PhonemeInstance> input = corpus_test.input(index);
    std::vector<int> path;

    max_path(input, crf, crf.lambda, crf.mu, &path);
    std::vector<PhonemeInstance> output = crf.alphabet().to_phonemes(path);

    Options opts;
    Wave resultSignal = SpeechWaveSynthesis(output, input, crf.alphabet())
      .get_resynthesis(opts);

    FileData fileData = alphabet_test.file_data_of(input[0]);
    Wave sourceSignal;
    sourceSignal.read(fileData.file);

    auto& frames = *(params->precompFrames);

    params->result.ItakuraSaito = compare_IS(resultSignal, sourceSignal);
    params->result.LogSpectrum = compare_LogSpectrum(resultSignal, frames[index]);
  }

  void resynth_index(ResynthParams* params) {
    do_resynth_index(params);
    *(params->flag) = 1;
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

  void aggregate(std::vector<Comparisons> params,
                 Comparisons* sum=0,
                 Comparisons* max=0,
                 Comparisons* avg=0) {
    Comparisons sumTemp;
    Comparisons avgTemp;
    int maxIndex = -1;
    for(unsigned i = 0; i < params.size(); i++) {
      if(maxIndex == -1 || params[i] < params[maxIndex])
        maxIndex = i;
      sumTemp = sumTemp + params[i];
    }
    avgTemp.LogSpectrum = sumTemp.LogSpectrum / params.size();
    avgTemp.ItakuraSaito = sumTemp.ItakuraSaito / params.size();
    if(sum)
      *sum = sumTemp;
    if(max)
      *max = params[maxIndex];
    if(avg)
      *avg = avgTemp;
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
    Wave sourceSignal;
    sourceSignal.read(params->file);
    auto frames = toFFTdFrames(sourceSignal);
    (*params->precompFrames)[params->index] = frames;
    *(params->flag) = 1;
  }

  void precomputeFrames(std::vector< std::vector<FrameFrequencies> >& precompFrames,
                        ThreadPool& tp) {
    unsigned count = corpus_test.size();
    bool flags[count];

    FFTPrecomputeParams *params = new FFTPrecomputeParams[count];
    for(unsigned i = 0; i < count; i++) {
      flags[i] = 0;
      FileData fileData = alphabet_test.file_data_of(corpus_test.input(i)[0]);
      
      params[i].init(i, &flags[i], fileData.file, &precompFrames);

      Task* t = new ParamTask<FFTPrecomputeParams>(&precomputeSingleFrames, &params[i]);
      tp.add_task(t);
    }
    wait_done(flags, count);

    delete[] params;
  }
  
  Comparisons do_train(ThreadPool& tp,
                       std::vector< std::vector<FrameFrequencies> > *precompFrames) {
    //return Comparisons().dummy(randDouble());
    unsigned count = corpus_test.size();
    bool flags[count];
    for(unsigned i = 0; i < count; i++)
      flags[i] = 0;

    auto params = new ResynthParams[count];
    for(unsigned i = 0; i < count; i++) {
      params[i].init(i, &flags[i]);

      params[i].precompFrames = precompFrames;
      
      Task* t = new ParamTask<ResynthParams>(&resynth_index, &params[i]);
      tp.add_task(t);
    }
    wait_done(flags, count);

    std::vector<Comparisons> comps;
    for(unsigned i = 0; i < count; i++) comps.push_back(params[i].result);

    Comparisons result;
    aggregate(comps, 0, 0, &result);
    delete[] params;
    return result;
  }
  
  void executeTraining(unsigned passes, std::array<Range, FC>& ranges, int maxIterations, std::vector< std::vector<FrameFrequencies> > &precompFrames, ThreadPool& tp, ValueCache<FC>& vc) {
    int iteration = 0;
    for (unsigned passNumber = 1; passNumber <= passes && iteration < maxIterations; passNumber++) {
      INFO("Pass " << passNumber);

      for(unsigned i = (passNumber == 1); i < ranges.size() && iteration < maxIterations; i++) {
        Range& range = ranges[i];
        range.reset();
        std::vector<Comparisons> comps;
        double bestCoef = -1;
        Comparisons bestVals;
        while(range.has_next() && iteration < maxIterations) {
          iteration++;

          // Update coefficient
          crf.set(range.feature, range.current);

          INFO("Pass: " << passNumber << ", iteration: " << iteration);
          INFO("Trying " << range.feature << " = " << range.current);
          DEBUG(for(unsigned k = 0; k < ranges.size(); k++)
                  LOG(ranges[k].feature << "=" << ranges[k].current););

          Comparisons result;
          result.LogSpectrum = 0;
          bool inCache;
          inCache = vc.load(ranges, result);

          if(!inCache)
            // And actual work...
            result = do_train(tp, &precompFrames);

          if(bestCoef == -1 || result < bestVals) {
            bestCoef = ranges[i].current;
            bestVals = result;
          }

          if(!inCache) {
            vc.save(ranges, result);
            vc.persist();
          }

          // Advance
          range.next();

          INFO("Value: " << result.value());
        }

        if(bestCoef != -1) {
          INFO(range.feature << " best value = " << bestCoef << " with " << bestVals.LogSpectrum);
          range.current = bestCoef;
        } else {
          INFO("Skipped " << range.feature);
        }
      }
    }    
  }

  int train(const Options& opts) {
    Progress::enabled = false;
    Comparisons::metric = opts.get_opt<std::string>("metric", "");

    //#pragma omp parallel for
    int threads = opts.get_opt<int>("thread-count", 8);
    ThreadPool tp(threads);
    int ret = tp.initialize_threadpool();
    if (ret == -1) {
      ERROR("Failed to initialize thread pool");
      return ret;
    }

    std::array<Range, FC> ranges = {{
        Range("trans-ctx", 1, 200, 1),
        Range("trans-pitch", 0, 200, 1),
        Range("state-pitch", 0, 200, 1),
        Range("trans-mfcc", 0, 2, 0.01),
        Range("state-duration", 0, 200, 1),
        Range("state-energy", 0, 200, 1)
      }};
    for(auto it : ranges)
      INFO("Range " << it.to_string());

    std::string valueCachePath = opts.get_opt<std::string>("value-cache", "value-cache.bin");
    ValueCache<FC> vc(valueCachePath);

    // Pre-compute FFTd frames of source signals
    INFO("Precomputing FFTs");
    std::vector< std::vector<FrameFrequencies> > precompFrames;
    precompFrames.resize(corpus_test.size());
    precomputeFrames(precompFrames, tp);
    INFO("Done");

    std::vector<TrainingOutput> outputs;
    unsigned passes = opts.get_opt<int>("training-passes", 3);
    int maxIterations = opts.get_opt<int>("max-iterations", 9999999);

    executeTraining(passes, ranges, maxIterations, precompFrames, tp, vc);

    INFO("Best at: ");
    for(unsigned k = 0; k < ranges.size(); k++)
      LOG(ranges[k].feature << "=" << ranges[k].current);

    return 0;
  }
}
