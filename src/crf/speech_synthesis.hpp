#ifndef __SPEECH_SYNTHESIS_H__
#define __SPEECH_SYNTHESIS_H__

#include<fstream>
#include<iostream>

#include"crf.hpp"
#include"../praat/parser.hpp"

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
