#include<cassert>

#include"gridsearch.hpp"
#include"threadpool.h"
#include"crf.hpp"
#include"tool.hpp"
#include"fourier.hpp"

struct Range {
  Range(): Range("", 0, 0, 1) { }
  Range(std::string feature, double from, double to, double step)
    :from(from), to(to), step(step), current(from), feature(feature) {
    assert(step > 0 && from <= to);
  }

  bool has_next() { return (current + step) <= to; }
  void next() { current += step; }
  
  double from, to, step, current;
  std::string feature;

  std::string to_string() const {
    std::stringstream str;
    str << "[" << feature << ", " << from << ", " << to << ", " << step << "]";
    return str.str();
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
    int sampleWidth = values.size();
    double* td_values = new double[sampleWidth];
    while(frameOffset < length) {
      for(int i = 0; i < sampleWidth; i++) td_values[i] = 0;

      int frameLength = std::min(sampleWidth, length - frameOffset);
      if(frameLength <= 0)
        break;

      WaveData frame = wave.extractBySample(frameOffset, frameOffset + frameLength);
      frameOffset += sampleWidth;
      for(unsigned i = 0; i < frame.size(); i++) td_values[i] = frame[i];

      ft::FT(td_values, frame.size(), values, values.size());
      result.push_back(values);
    }
    delete td_values;
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
    assert(frames1.size() == frames2.size());

    double value = 0;
    for(unsigned j = 0; j < frames1.size(); j++) {
      double diff = 0;
      auto& freqs1 = frames1[j];
      auto& freqs2 = frames2[j];
      for(unsigned i = 0; i < freqs1.size(); i++) {
        double m1 = freqs1[i].magn();
        m1 = m1 ? m1 : 1;
        double m2 = freqs2[i].magn();
        m2 = m2 ? m2 : 1;
        diff += std::abs( std::log( m2 / m1) );
      }
      value += diff;
    }
    return value;
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

  void resynth_index(ResynthParams* params) {
    int index = params->index;
  
    std::vector<PhonemeInstance> input = corpus_test.input(index);
    std::vector<int> path;

    max_path(input, crf, crf.lambda, crf.mu, &path);
    std::vector<PhonemeInstance> output = crf.alphabet().to_phonemes(path);

    Wave resultSignal = SpeechWaveSynthesis(output, input, crf.alphabet())
      .get_resynthesis();

    FileData fileData = alphabet_test.file_data_of(input[0]);
    Wave sourceSignal;
    sourceSignal.read(fileData.file);

    auto& frames = *(params->precompFrames);

    params->result.ItakuraSaito = compare_IS(resultSignal, sourceSignal);
    params->result.LogSpectrum = compare_LogSpectrum(resultSignal, frames[index]);
    
    *(params->flag) = 1;
  }

  void wait_done(bool* flags, unsigned count) {
    bool done;
    do {
      sleep(1);
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
    } while(!done);
  }

  void aggregate(std::vector<Comparisons> params,
                 Comparisons* sum=0,
                 Comparisons* max=0) {
    Comparisons sumTemp;
    int maxIndex = -1;
    for(unsigned i = 0; i < params.size(); i++) {
      if(maxIndex == -1 || params[i] < params[maxIndex])
        maxIndex = i;
      sumTemp = sumTemp + params[i];
    }
    if(sum)
      *sum = sumTemp;
    if(max)
      *max = params[maxIndex];
  }

  Comparisons do_train(ThreadPool& tp,
                       std::vector< std::vector<FrameFrequencies> > *precompFrames) {
    unsigned count = corpus_test.size();
    bool flags[count];

    ResynthParams params[count];
    for(unsigned i = 0; i < count; i++) {
      flags[i] = 0;
      params[i].init(i, &flags[i]);

      params[i].precompFrames = precompFrames;
      
      Task* t = new ParamTask<ResynthParams>(&resynth_index, &params[i]);
      tp.add_task(t);
    }
    wait_done(flags, count);

    std::vector<Comparisons> comps;
    for(unsigned i = 0; i < count; i++) comps.push_back(params[i].result);

    Comparisons result;
    aggregate(comps, &result);
    return result;
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
        Range("trans-ctx", 100, 100, 1),
        Range("trans-pitch", 1, 100, 1),
        Range("state-pitch", 0, 100, 1),
        Range("trans-mfcc", 0, 10, 0.1),
        Range("state-duration", 0, 100, 1)
      }};
    for(auto it : ranges)
      INFO("Range " << it.to_string());

    // Pre-compute FFTd frames of source signals
    INFO("Precomputing FFTs");
    std::vector< std::vector<FrameFrequencies> > precompFrames;
    for(unsigned i = 0; i < corpus_test.size(); i++) {
      FileData fileData = alphabet_test.file_data_of(corpus_test.input(i)[0]);
      Wave sourceSignal;
      sourceSignal.read(fileData.file);
      auto frames = toFFTdFrames(sourceSignal);
      precompFrames.push_back(frames);
    }
    INFO("Done");

    int iteration = 0;
    int maxIterations = opts.get_opt<int>("max-iterations", 9999999);
    std::vector<TrainingOutput> outputs;
    for(unsigned i = 1; i < ranges.size(); i++) {
      std::vector<Comparisons> comps;
      double bestCoef = -1;
      Comparisons bestVals;
      while(ranges[i].has_next()) {
        iteration++;
        if(iteration > maxIterations)
          break;
        // Update coefficient
        crf.set(ranges[i].feature, ranges[i].current);
        // Log
        LOG("");
        INFO("Iteration: " << iteration);
        for(unsigned k = 0; k < ranges.size(); k++)
          LOG(ranges[k].feature << "=" << ranges[k].current);

        // And actual work...
        Comparisons result = do_train(tp, &precompFrames);
        if(bestCoef == -1 || result < bestVals)
          bestCoef = ranges[i].current;

        // Advance
        ranges[i].next();

        INFO("Value: " << result.value());
      }
      INFO(ranges[i].feature << " best value = " << bestCoef);
      ranges[i].current = bestCoef;
    }

    INFO("Best at: ");
    for(unsigned k = 0; k < ranges.size(); k++)
      LOG(ranges[k].feature << "=" << ranges[k].current);

    tp.destroy_threadpool();

    return 0;
  }
}
