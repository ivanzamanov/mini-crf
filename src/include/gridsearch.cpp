#include"gridsearch.hpp"
#include"threadpool.h"
#include"crf.hpp"
#include"tool.hpp"
#include"speech_mod.hpp"

// TODO
static double compare_IS(Wave result, Wave original) {
  return 1;
}

// TODO
static double compare_LogSpectrum(Wave result, Wave original) {
  return 1;
}

struct Range {
  Range(std::string feature, int index, double from, double to, double step)
    :from(from), to(to), step(step), index(index), feature(feature) {
    current = from - step;
  }

  bool has_next() { return current < to; }
  void next() { current += step; }
  
  double from, to, step, current;
  int index;
  std::string feature;
};

static const int FC = 5;
struct TrainingOutput {
  Range ranges[FC];
  gridsearch::Comparisons result;
};

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

  void resynth_index(ResynthParams* params) {
    int index = params->index;
  
    std::vector<PhonemeInstance> input = corpus_test.input(index);
    std::string sentence_string = to_text_string(input);
    std::vector<int> path;

    cost cost = max_path(input, crf, crf.lambda, crf.mu, &path);
    std::vector<PhonemeInstance> output = crf.alphabet().to_phonemes(path);

    std::cerr << "Cost " << index << ": " << cost << std::endl;

    std::stringstream outputFile;
    outputFile << "/tmp/resynth-" << params->index << ".wav";

    std::ofstream wav_output(outputFile.str());
    Wave resultSignal = SpeechWaveSynthesis(output, input, crf.alphabet())
      .get_resynthesis();

    FileData fileData = alphabet_test.file_data_of(input[0]);
    Wave sourceSignal;
    sourceSignal.read(fileData.file);

    params->result.ItakuraSaito = compare_IS(resultSignal, sourceSignal);
    params->result.LogSpectrum = compare_LogSpectrum(resultSignal, sourceSignal);
    
    *(params->flag) = 1;
  }

  void wait_done(bool* flags, unsigned count) {
    bool done;
    do {
      sleep(5);
      done = true;
      for(unsigned i = 0; i < count; i++)
        done &= flags[i];
      cerr << "Unfinished: ";
      for(unsigned i = 0; i < count; i++)
        if(!flags[i])
          cerr << i << " ";
      cerr << std::endl;
    } while(!done);
  }

  TrainingOutput do_train(ThreadPool& tp) {
    unsigned count = corpus_test.size();
    bool flags[count];

    ResynthParams params[count];
    for(unsigned i = 0; i < count; i++) {
      flags[i] = 0;
      params[i].init(i, &flags[i]);
      Task* t = new ParamTask<ResynthParams>(&resynth_index, &params[i]);
      tp.add_task(t);
    }

    wait_done(flags, count);
  }

  void populate(Range r) {
    for(unsigned i = 0; i < crf.lambda.size(); i++)
      if(crf.features.enames[i] == r.feature)
        crf.lambda[i] = r.current;

    for(unsigned i = 0; i < crf.mu.size(); i++)
      if(crf.features.vnames[i] == r.feature)
        crf.mu[i] = r.current;
  }

  TrainingOutput train_on_ranges(Range* ranges, int n, ThreadPool& tp) {
    for(int i = 0; i < n; i++)
      populate(ranges[i]);
    return do_train(tp);
  }

  int train(const Options& opts) {
    Progress::enabled = false;
    //#pragma omp parallel for
    int threads = opts.get_opt<int>("thread-count", 8);
    ThreadPool tp(threads);
    int ret = tp.initialize_threadpool();
    if (ret == -1) {
      cerr << "Failed to initialize thread pool" << endl;
      return ret;
    }

    crf.mu[0] = opts.get_opt<coefficient>("state-pitch", 0);
    crf.mu[1] = opts.get_opt<coefficient>("state-duration", 0);
    crf.lambda[0] = opts.get_opt<coefficient>("trans-pitch", 0);
    crf.lambda[1] = opts.get_opt<coefficient>("trans-mfcc", 0);
    crf.lambda[2] = opts.get_opt<coefficient>("trans-ctx", 0);

    Range ranges[FC] = {
      Range("trans-ctx", 2, 100, 100, 1),
      Range("trans-pitch", 0, 1, 100, 1),
      Range("state-pitch", 0, 1, 100, 0),
      Range("trans-mfcc", 1, 1, 10, 0.1),
      Range("state-duration", 1, 1, 100, 1)
    };
    
    for(int i = 1; i < FC; i++) {
      while(ranges[i].has_next()) {
        ranges[i].next();
        TrainingOutput output = train_on_ranges(ranges, FC);
      }
    }

    int result = do_train(tp);

    tp.destroy_threadpool();
    return result;
  }
}
