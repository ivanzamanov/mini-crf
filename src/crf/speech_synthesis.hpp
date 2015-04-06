#ifndef __SPEECH_SYNTHESIS_H__
#define __SPEECH_SYNTHESIS_H__

#include<fstream>
#include<iostream>

#include"crf.hpp"
#include"../praat/parser.hpp"

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

    for(int i = 0; i < labels.length; i++) {
      classes[labels[i].label].push_back(i);
    }
  }

  int size() const {
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

struct PhonemeAlphabet : LabelAlphabet<PhonemeInstance> {
  Array<std::string> files;
  Array<int> file_indices;

  std::string file_of(int phonId) {
    return files[file_indices[phonId]];
  }

  Input first_by_label(char label) {
    LabelClass& clazz = classes[label];
    return clazz.front();
  }
};

void build_data(std::istream& list_input, PhonemeAlphabet* alphabet, Corpus* corpus) {
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

  alphabet->labels = to_array(phonemes);
  alphabet->build_classes();

  alphabet->files = to_array(files_map);
  alphabet->file_indices = to_array(file_indices);
  std::cerr << "End building alphabet" << '\n';
}

#endif
