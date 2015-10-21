#ifndef __GRID_SEARCH_HPP__
#define __GRID_SEARCH_HPP__

#include<cassert>

#include"types.hpp"
#include"options.hpp"
#include"speech_mod.hpp"
#include"fourier.hpp"

namespace gridsearch {
  typedef std::array<cdouble, (int) (0.025 * 24000)> FrameFrequencies;

  double compare_LogSpectrum(Wave& result, Wave& original);
  double compare_LogSpectrum(Wave& result, std::vector<FrameFrequencies>&);
  double compare_IS(Wave& result, Wave& original);

  struct Comparisons {
    static std::string metric;
    static std::string aggregate;

    static double aggregate_values(double v1, double v2) {
      if(Comparisons::aggregate == "sum")
        return v1 + v2;
      if(Comparisons::aggregate == "max")
        return std::max(v1, v2);
      assert(false); // metric-aggregate must be one of [sum, max]
    }

    double ItakuraSaito;
    double LogSpectrum;

    const Comparisons& dummy(double v) {
      ItakuraSaito = LogSpectrum = v; return *this;
    }
    
    const Comparisons& fill(Wave& dist, Wave& original) {
      ItakuraSaito = compare_IS(dist, original);
      LogSpectrum = compare_LogSpectrum(dist, original);
      return *this;
    }
    
    void fill(Wave& dist, std::vector<FrameFrequencies>& sourceFreqs) {
      //ItakuraSaito = compare_IS(dist, original);
      LogSpectrum = compare_LogSpectrum(dist, sourceFreqs);
    }

    void print() const {
      LOG("IS = " << ItakuraSaito);
      LOG("LogSpectrum = " << LogSpectrum);
    }
    
    bool operator<(const Comparisons o) const {
      return LogSpectrum < o.LogSpectrum;
    }
    
    bool operator<=(const Comparisons o) const {
      return *this < o || (ItakuraSaito == o.ItakuraSaito && LogSpectrum == o.LogSpectrum);
    }

    const Comparisons operator+(const Comparisons o) const {
      Comparisons result = {
        .ItakuraSaito = ItakuraSaito + o.ItakuraSaito,
        .LogSpectrum = LogSpectrum + o.LogSpectrum
      };
      return result;
    }

    double value() const {
      if(Comparisons::metric == "ItSt")
        return ItakuraSaito;
      if(Comparisons::metric == "LogS")
        return LogSpectrum;
      assert(false); // Metric must be one of [ItSt, LogS]
    }
  };
  
  struct ResynthParams {
    void init(int index, bool* flag) {
      this->index = index;
      this->flag = flag;
    }

    int index;
    bool* flag;
    Comparisons result;
    std::vector< std::vector<FrameFrequencies> >* precompFrames;
  };

  std::string to_text_string(const std::vector<PhonemeInstance>& vec);
  int train(const Options& opts);
}
#endif
