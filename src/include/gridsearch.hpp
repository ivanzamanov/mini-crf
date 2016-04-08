#ifndef __GRID_SEARCH_HPP__
#define __GRID_SEARCH_HPP__

#include<cassert>
#include<utility>

#include"options.hpp"
#include"speech_mod.hpp"
#include"comparisons.hpp"

namespace gridsearch {

  struct TrainingOutput {
    double cmp;
    std::vector<int> path;
    std::array<cost, 2> bestValues;
  };

  struct TrainingOutputs : public std::vector<TrainingOutput> {
    cost value() const {
      auto count = this->size();
      std::vector<double> comps(count);
      for(unsigned i = 0; i < count; i++)
        comps[i] = (*this)[i].cmp;

      double result;
      Comparisons::aggregate(comps, 0, 0, &result);
      return result;
    }

    typedef vector<std::pair<int, cost> > Diffs;
    Diffs findDifferences(const TrainingOutputs& other) const {
      Diffs diffs;
      for(auto i = 0u; i < this->size(); i++) {
        auto to1 = (*this)[i],
          to2 = other[i];

        // check if path is different
        for(auto j = 0ul; j < to1.path.size(); j++)
          if(to1.path[j] != to2.path[j]) {
            diffs.push_back(std::make_pair(i, to1.bestValues[0] - to2.bestValues[0]));
            break;
          }
      }
      return diffs;
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
