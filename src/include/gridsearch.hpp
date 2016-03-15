#ifndef __GRID_SEARCH_HPP__
#define __GRID_SEARCH_HPP__

#include<cassert>

#include"types.hpp"
#include"options.hpp"
#include"speech_mod.hpp"
#include"fourier.hpp"

namespace gridsearch {
  constexpr size_t FFT_SIZE = 512;
  typedef std::array<cdouble, FFT_SIZE> FrameFrequencies;

  double compare_LogSpectrum(Wave& result, Wave& original);
  double compare_LogSpectrum(Wave& result, const std::vector<FrameFrequencies>&);
  double compare_IS(Wave& result, Wave& original);

  struct Comparisons {
    Comparisons()
      :LogSpectrum(0) { }

    Comparisons(double, double ls)
      :LogSpectrum(ls) { }

    Comparisons(const Comparisons& o)
      :LogSpectrum(o.LogSpectrum) { }

    //double ItakuraSaito;
    double LogSpectrum;

    const Comparisons& fill(Wave& dist, Wave& original) {
      LogSpectrum = compare_LogSpectrum(dist, original);
      return *this;
    }

    const Comparisons& dummy(double v) {
      //ItakuraSaito =
      LogSpectrum = v; return *this;
    }

    bool operator<(const Comparisons o) const {
      return LogSpectrum < o.LogSpectrum;
    }

    bool operator<=(const Comparisons& o) const {
      return LogSpectrum <= o.LogSpectrum;
    }

    const Comparisons operator+(const Comparisons o) const {
      Comparisons result(0, LogSpectrum + o.LogSpectrum);
      return result;
    }

    double value() const {
      return LogSpectrum;
    }

    static void aggregate(const std::vector<Comparisons>& params,
                          Comparisons* sum=0,
                          Comparisons* max=0,
                          Comparisons* avg=0) {
      Comparisons sumTemp, avgTemp;
      auto maxIndex = 0;
      for(auto i = 0u; i < params.size(); i++) {
        if(params[i] < params[maxIndex])
          maxIndex = i;
        sumTemp = sumTemp + params[i];
      }
      avgTemp.LogSpectrum = sumTemp.LogSpectrum / params.size();
      //avgTemp.ItakuraSaito = sumTemp.ItakuraSaito / params.size();
      if(sum)
        *sum = sumTemp;
      if(max)
        *max = params[maxIndex];
      if(avg)
        *avg = avgTemp;
    }
  };

  struct TrainingOutput {
    Comparisons cmp;
    std::vector<int> path;
    std::array<cost, 2> bestValues;
  };

  struct TrainingOutputs : public std::vector<TrainingOutput> {
    cost value() const {
      auto count = this->size();
      std::vector<Comparisons> comps(count);
      for(unsigned i = 0; i < count; i++)
        comps[i] = (*this)[i].cmp;

      Comparisons result;
      Comparisons::aggregate(comps, 0, 0, &result);
      return result.value();
    }
  };

  struct ResynthParams {
    
    void init(int index, bool* flag,
              std::vector< std::vector<FrameFrequencies> >* precomputed) {
      this->index = index;
      this->flag = flag;
      this->precompFrames = precomputed;
    }

    int index;
    bool* flag;
    std::vector< std::vector<FrameFrequencies> >* precompFrames;
    TrainingOutput result;
  };

  std::string to_text_string(const std::vector<PhonemeInstance>& vec);
  int train(const Options& opts);
}
#endif
