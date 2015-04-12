#ifndef __SPEECH_SYNTHESIS_H__
#define __SPEECH_SYNTHESIS_H__

#include<fstream>
#include<iostream>
#include<sstream>

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

  void print_synth(std::vector<int> &path) {
    std::stringstream phonemeIds;
    for(auto it = path.begin(); it != path.end(); it++) {
      PhonemeInstance* phon = &labels[*it];
      phonemeIds << *it << "=" << phon->label << " ";
      std::string file = file_of(*it);
      std::cout << "File=" << file << " ";
      std::cout << "Start=" << phon->start << " ";
      std::cout << "End=" << phon->end << " ";
      std::cout << "Label=" << phon->label << '\n';
    }

    std::cerr << phonemeIds.str() << std::endl;
  }
};

void build_data_txt(std::istream& list_input, PhonemeAlphabet* alphabet, Corpus* corpus) {
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

  alphabet->labels = to_array(phonemes);
  alphabet->build_classes();

  alphabet->files = to_array(files_map);
  alphabet->file_indices = to_array(file_indices);
  std::cerr << "End building alphabet" << '\n';
}

void build_data_bin(std::istream& input, PhonemeAlphabet& alphabet, Corpus& corpus) {
  BinaryReader r(&input);
  unsigned alphabet_size;
  r >> alphabet_size;
  alphabet.labels.length = alphabet_size;
  alphabet.labels.data = new PhonemeInstance[alphabet_size];

  alphabet.file_indices.data = new int[alphabet_size];
  alphabet.file_indices.length = alphabet_size;

  for(unsigned i = 0; i < alphabet.size(); i++)
    r >> alphabet.labels[i] >> alphabet.file_indices[i];
  std::cerr << "Read phonemes, " << r.bytes << " bytes\n";

  unsigned count;
  r >> count;
  alphabet.files.length = count;
  alphabet.files.data = new std::string[count];
  unsigned length;
  for(unsigned i = 0; i < count; i++) {
    r >> length;
    std::string str(length, ' ');
    for(unsigned j = 0; j < length; j++)
      r >> str[j];
    alphabet.files[i] = str;
  }
  std::cerr << "Read file names, " << r.bytes << " bytes\n";

  unsigned corpus_size;
  r >> corpus_size;
  for(unsigned i = 0; i < corpus_size; i++) {
    r >> length;
    vector<Input> input(length);
    for(unsigned j = 0; j < length; j++)
      r >> input[j];

    r >> length;
    vector<Label> labels(length);
    for(unsigned j = 0; j < length; j++)
      r >> labels[j];

    corpus.add(input, labels);
  }

  alphabet.build_classes();

  std::cerr << "Read " << r.bytes << " bytes" << std::endl;
}

#endif
