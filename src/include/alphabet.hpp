#ifndef __ALPHABET_H__
#define __ALPHABET_H__

#include"util.hpp"

template<class LabelObject>
struct LabelAlphabet {
  static const int CLASS_COUNT = 256;

  typedef std::vector<int> LabelClass;
  typedef std::vector<LabelClass::const_iterator> Iterators;
  typedef LabelObject Label;

  std::vector<LabelClass> classes;
  std::vector<LabelObject> labels;

  void build_classes() {
    classes.clear();
    classes.resize(CLASS_COUNT);
    for(id_t i = 0; i < labels.size(); i++)
      classes[labels[i].label].push_back(i);
  }

  unsigned size() const { return labels.size(); }

  LabelObject& fromInt(int i) {
    return labels[i];
  }

  const LabelObject& fromInt(int i) const {
    return labels[i];
  }

  template<class Filter>
  void iterate_sequences(const std::vector<Label>& input, Filter& filter) const {
    auto iters = std::vector<LabelClass::const_iterator>(input.size());
    auto class_indices = std::vector<int>(input.size());

    for(unsigned i = 0; i < input.size(); i++) {
      auto index = labels[input[i].label].label;
      iters[i] = classes[index].begin();
      class_indices[i] = index;
    }

    std::vector<Label> labels(input.size());
    permute(iters, class_indices, 0, labels, filter);
  }

  template<class Filter>
  void permute(Iterators& iters, std::vector<int>& class_indices, unsigned index, std::vector<Label>& labels, Filter& filter) const {
    if(index == iters.size() - 1) {
      while(iters[index] != classes[class_indices[index]].end()) {
        labels[index] = fromInt(*iters[index]);
        filter(labels);
        ++iters[index];
      }
    } else {
      while(iters[index] != classes[class_indices[index]].end()) {
        labels[index] = fromInt(*iters[index]);
        permute(iters, class_indices, index + 1, labels, filter);
        iters[index + 1] = classes[class_indices[index + 1]].begin();
        ++iters[index];
      }
    }
  }
};

#endif
