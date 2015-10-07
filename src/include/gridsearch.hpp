#ifndef __GRID_SEARCH_HPP__
#define __GRID_SEARCH_HPP__

#include<cassert>
#include"types.hpp"
#include"options.hpp"

namespace gridsearch {
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

    bool operator<(const Comparisons o) const {
      return ItakuraSaito < o.ItakuraSaito;
    }

    const Comparisons operator+(const Comparisons o) const {
      return Comparisons {
      ItakuraSaito: ItakuraSaito + o.ItakuraSaito,
      LogSpectrum: LogSpectrum + o.LogSpectrum,
      };
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
  };

  std::string to_text_string(const std::vector<PhonemeInstance>& vec);
  int train(const Options& opts);
}
#endif
