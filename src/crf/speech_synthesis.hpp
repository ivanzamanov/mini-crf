#ifndef __SPEECH_SYNTHESIS_H__
#define __SPEECH_SYNTHESIS_H__

#include<fstream>
#include<iostream>

#include"crf.hpp"
#include"../praat/parser.hpp"

struct LabelAlphabet {
  typedef std::vector<Label> LabelClass;
  typedef std::vector<LabelClass::const_iterator> Iterators;

  Array<PhonemeInstance> phonemes;
  Array<std::string> files;
  Array<int> file_indices;
  Array<LabelClass> classes;

  std::string file_of(int phonId) {
    return files[file_indices[phonId]];
  }

  Input first_by_label(char label) {
    LabelClass& clazz = classes[label];
    return clazz.front();
  }

  int* possible_labels(int inp, int& n) {
    const PhonemeInstance& phon = fromInt(inp);
    n = classes[phon.label].size();
    int* result = new int[n];
    int* idx = result;
    for(auto it = classes[phon.label].begin(); it != classes[phon.label].end(); it++, idx++)
      *idx = *it;
    return result;
  }

  bool allowedTransition(int, int) const {
    return true;
  }

  bool allowedState(int l1, int l2) const {
    return fromInt(l1).label == fromInt(l2).label;
  }

  int toInt(const Input& label) const {
    return fromInt(label).label;
  }

  const PhonemeInstance& fromInt(int i) const {
    return phonemes[i];
  }

  template<class Filter>
  void iterate_sequences(const std::vector<Input>& input, Filter& filter) const {
    std::vector<LabelClass::const_iterator> iters(input.size());
    std::vector<int> class_indices(input.size());

    for(unsigned i = 0; i < input.size(); i++) {
      int index = toInt(input[i]);
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

Array<LabelAlphabet::LabelClass> build_classes(std::vector<PhonemeInstance> phonemes) {
  Array<LabelAlphabet::LabelClass> result;
  const int length = 256;
  result.data = new LabelAlphabet::LabelClass[length];
  result.length = length;
  auto it = phonemes.begin();
  int i = 0;

  while(it != phonemes.end()) {
    PhonemeInstance& phon = *it;
    result[phon.label].push_back(i);
    i++;
    ++it;
  }

  return result;
}

void build_data(std::istream& list_input,
                LabelAlphabet* alphabet,
                Corpus* corpus) {

  std::cerr << "Building label alphabet" << '\n';
  std::string buffer;
  std::vector<PhonemeInstance> phonemes;
  // phonemes[i] came from file files_map[file_indices[i]]
  std::vector<int> file_indices;
  std::vector<std::string> files_map;

  while(list_input >> buffer) {
    std::ifstream stream(buffer.c_str());
    int size;
    PhonemeInstance* phonemes_from_file = parse_file(stream, size);
    list_input >> buffer;
    files_map.push_back(buffer);

    std::vector<Input> inputs;
    std::vector<Label> labels;

    for(int i = 0; i < size; i++) {
      int phoneme_index = phonemes.size();
      phonemes.push_back(phonemes_from_file[i]);
      file_indices.push_back(files_map.size() - 1);

      inputs.push_back(phoneme_index);
      labels.push_back(phoneme_index);
    }

    corpus->add(inputs, labels);
  }

  /*bool hasA = false, hasB = false;
  for(auto it = phonemes.begin(); it != phonemes.end();) {
    if((*it).label == 'a' && !hasA) {
      hasA = true;
      it++;
    } else if((*it).label == 's' && !hasB) {
      hasB = true;
      it++;
    } else {
      it = phonemes.erase(it);
    }
  }*/

  alphabet->phonemes = to_array(phonemes);
  alphabet->files = to_array(files_map);
  alphabet->file_indices = to_array(file_indices);
  alphabet->classes = build_classes(phonemes);
  std::cerr << "End building alphabet" << '\n';
}

#endif
