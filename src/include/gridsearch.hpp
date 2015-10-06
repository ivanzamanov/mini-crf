#ifndef __GRID_SEARCH_HPP__
#define __GRID_SEARCH_HPP__

#include"types.hpp"
#include"options.hpp"

namespace gridsearch {
  struct Comparisons {
    double ItakuraSaito;
    double LogSpectrum;
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
