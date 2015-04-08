#ifndef __ALPHABET_H__
#define __ALPHABET_H__

#include"util.hpp"

typedef int Label;
typedef int Input;

template<class LabelObject>
struct LabelAlphabet {
  typedef std::vector<int> LabelClass;
  typedef std::vector<LabelClass::const_iterator> Iterators;

  Array<LabelClass> classes;
  Array<LabelObject> labels;
  
  void build_classes() {
    const int length = 256;
    classes.data = new LabelAlphabet::LabelClass[length];
    classes.length = length;

    for(unsigned i = 0; i < labels.length; i++) {
      classes[labels[i].label].push_back(i);
    }
  }

  unsigned size() const {
    return labels.length;
  }

  bool allowedState(int l1, int l2) const {
    return fromInt(l1).label == fromInt(l2).label;
  }

  const LabelObject& fromInt(int i) const {
    return labels[i];
  }

  template<class Filter>
  void iterate_sequences(const std::vector<Input>& input, Filter& filter) const {
    std::vector<LabelClass::const_iterator> iters(input.size());
    std::vector<int> class_indices(input.size());

    for(unsigned i = 0; i < input.size(); i++) {
      int index = labels[input[i]].label;
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
        labels[index] = *iters[index];
        filter(labels);
        ++iters[index];
      }
    } else {
      while(iters[index] != classes[class_indices[index]].end()) {
        labels[index] = *iters[index];
        permute(iters, class_indices, index + 1, labels, filter);
        iters[index + 1] = classes[class_indices[index + 1]].begin();
        ++iters[index];
      }
    }
  }
};

#endif
