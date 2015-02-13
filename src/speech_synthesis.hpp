#ifndef __SPEECH_SYNTHESIS_H__
#define __SPEECH_SYNTHESIS_H__

#include<vector>

#include"crf/crf.hpp"
#include"praat/parser.hpp"

class LabelAlphabet {
public:
  LabelAlphabet(): classes(10) {
    for(int i=0; i < 10; i++) {
      for(int j = 0; j < 5; j++) {
        classes[i].push_back(i * 10 + j);
      }
    }
  }

private:
  typedef std::vector<Label> LabelClass;
  std::vector<LabelClass> classes;
  typedef std::vector<LabelClass::const_iterator> Iterators;

public:
  int toInt(const Label& label) {
    return label;
  }

  template<class Filter>
  void iterate_sequences(const Sequence<Input>& input, Filter& filter) {
    std::vector<LabelClass::const_iterator> iters(input.length());
    std::vector<int> class_indices(input.length());
    for(int i = 0; i < input.length(); i++) {
      int index = toInt(input[i]);
      iters[i] = classes[index].begin();
      class_indices[i] = index;
    }
    Sequence<Label> labels(input.length());
    permute(iters, class_indices, 0, labels, filter);
  }

  template<class Filter>
  void permute(Iterators& iters, std::vector<int>& class_indices, unsigned index, Sequence<Label>& labels, Filter& filter) {
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

LabelAlphabet* build_alphabet() {
  LabelAlphabet* result = new LabelAlphabet();
  return result;
}

#endif
