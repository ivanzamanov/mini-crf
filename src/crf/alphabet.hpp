#ifndef __ALPHABET_H__
#define __ALPHABET_H__

#include"util.hpp"

template<class LabelObject>
struct LabelAlphabet {
  static const int CLASS_COUNT = 256;
  typedef std::vector<int> LabelClass;
  typedef std::vector<LabelClass::const_iterator> Iterators;
  typedef LabelObject Label;

  Array<LabelClass> classes;
  Array<LabelObject> labels;

  void build_classes() {
    const int length = CLASS_COUNT;
    if(classes.data != 0)
      delete[] classes.data;

    classes.data = new LabelAlphabet::LabelClass[length];
    classes.length = length;

    for(unsigned i = 0; i < labels.length; i++)
      classes[labels[i].label].push_back(i);
  }

  void optimize() {
    build_classes();
    Array<LabelObject> new_labels;
    new_labels.init(labels.length);

    unsigned index = 0;
    for(unsigned i = 0; i < CLASS_COUNT; i++) {
      for(auto it = classes[i].begin(); it != classes[i].end(); it++) {
        LabelObject obj = fromInt(*it);
        new_labels[index] = obj;
        obj.id = index;
        index++;
      }
    }
    delete[] labels.data;
    labels.data = new_labels.data;
    build_classes();
  }

  unsigned size() const { return labels.length; }

  bool allowedState(const LabelObject& l1, const LabelObject& l2) const {
    return l1.label == l2.label;
  }

  LabelObject& fromInt(int i) {
    return labels[i];
  }

  const LabelObject& fromInt(int i) const {
    return labels[i];
  }

  template<class Filter>
  void iterate_sequences(const std::vector<Label>& input, Filter& filter) const {
    std::vector<LabelClass::const_iterator> iters(input.size());
    std::vector<int> class_indices(input.size());

    for(unsigned i = 0; i < input.size(); i++) {
      int index = labels[input[i].label].label;
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
