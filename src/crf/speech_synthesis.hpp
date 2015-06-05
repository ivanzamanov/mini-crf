#ifndef __SPEECH_SYNTHESIS_H__
#define __SPEECH_SYNTHESIS_H__

#include<fstream>
#include<iostream>
#include<sstream>

#include"crf.hpp"
#include"../praat/textgrid.hpp"
#include"../praat/parser.hpp"

struct PhonemeAlphabet : LabelAlphabet<PhonemeInstance> {
  Array<std::string> files;
  Array<int> file_indices;

  const LabelAlphabet<PhonemeInstance>::LabelClass& get_class(const PhonemeInstance& phon) const {
    return classes[phon.label];
  }

  std::string file_of(int phonId) {
    return files[file_indices[phonId]];
  }

  int first_by_label(char label) {
    LabelClass& clazz = classes[label];
    return clazz.front();
  }

  std::vector<Label> to_sequence(const std::string str) {
    std::vector<Label> result(str.length());
    std::cerr << "Input phoneme ids: ";
    for(unsigned i = 0; i < str.length(); i++) {
      int first = first_by_label(str[i]);
      result[i] = fromInt(first);
      std::cerr << first << " ";
    }
    std::cerr << std::endl;
    return result;
  }

  std::vector<PhonemeInstance> to_phonemes(const std::vector<int>& ids) {
    std::vector<PhonemeInstance> result;
    for(auto it = ids.begin(); it != ids.end(); it++)
      result.push_back(fromInt(*it));
    return result;
  }

  std::vector<int> phonemes_of_file(int file_index) {
    std::vector<int> result;
    for(unsigned i = 0; i < file_indices.length && (file_indices[i] <= file_index); i++) {
      if(file_indices[i] == file_index)
        result.push_back(i);
    }
    return result;
  }
};

struct SynthPrinter {
  SynthPrinter(PhonemeAlphabet& alphabet): alphabet(alphabet) { }

  PhonemeAlphabet& alphabet;

  void print_synth(std::vector<int> &path, const std::vector<PhonemeInstance>& desired) {
    print_synth(path, desired, std::cout);
  }

  void print_synth(std::vector<int> &path, const std::vector<PhonemeInstance>& desired, const std::string file) {
    std::ofstream out(file);
    print_synth(path, desired, out);
  }

  void print_synth(std::vector<int> &path, const std::vector<PhonemeInstance>& desired, std::ostream& out) {
    std::stringstream run_lengths;
    std::stringstream phonemeIds;
    for(unsigned i = 0; i < path.size(); i++) {
      int id = path[i];
      const PhonemeInstance& phon = alphabet.fromInt(id);
      std::string file = alphabet.file_of(id);
      out << "File=" << file << " ";
      out << "Start=" << phon.start << " ";
      out << "End=" << phon.end << " ";
      out << "Label=" << PhoneticLabelUtil::fromInt(phon.label) << " ";
      out << "Pitch=" << desired_pitch(desired[i]) << " ";
      out << "Duration=" << desired[i].duration << '\n';

      phonemeIds << id << "=" << phon.label << " ";
      if(i > 0 && id != (path[i-1] + 1))
        run_lengths << "|";
      run_lengths << PhoneticLabelUtil::fromInt(phon.label);
    }

    //std::cerr << phonemeIds.str() << std::endl;
    std::cerr << run_lengths.str() << std::endl;
  }

  const std::string desired_pitch(const PhonemeInstance& p) {
    std::stringstream str;
    double mid = 0;
    for(unsigned i = 0; i < p.frames.length; i++)
      mid += p.frames[i].pitch;
    mid = mid / p.frames.length;
    str << mid;
    return str.str();
  }

  void print_textgrid(std::vector<int> &path, std::vector<PhonemeInstance> &input, const std::string file) {
    std::ofstream out(file);
    print_textgrid(path, input, out);
  }

  void print_textgrid(std::vector<int> &path, std::vector<PhonemeInstance> &input, std::ostream& out) {
    TextGrid grid(path.size());
    unsigned i = 0;
    double time_offset = 0;
    for(i = 0; i < path.size(); i++) {
      const PhonemeInstance& phon = alphabet.fromInt(path[i]);
      const PhonemeInstance& desired = input[i];

      grid[i].xmin = time_offset;
      time_offset += desired.duration;
      grid[i].xmax = time_offset;

      std::stringstream str;
      str << phon.label << "= " << path[i] << ", sp= " << phon.first().pitch << ", ep= " << phon.last().pitch << ", dp=" << desired_pitch(input[i]);
      grid[i].text = str.str();
    }
    out << grid;
  }
};

PhonemeInstance to_synth_input(const PhonemeInstance& p) {
  PhonemeInstance result;
  result.start = p.start;
  result.end = p.end;
  result.label = p.label;
  Frame frame;
  double pitch = 0;

  for(unsigned i = 0; i < p.frames.length; i++)
    pitch += p.at(i).pitch;

  frame.pitch = pitch;
  singleton_array(result.frames, frame);
  return result;
}

void build_data_txt(std::istream& list_input, PhonemeAlphabet* alphabet, Corpus<PhonemeInstance, PhonemeInstance>* corpus) {
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

    std::vector<PhonemeInstance> inputs;
    std::vector<PhonemeInstance> labels;

    for(int i = 0; i < size; i++) {
      int phoneme_index = phonemes.size();
      phonemes_from_file[i].id = phoneme_index;

      phonemes.push_back(phonemes_from_file[i]);
      file_indices.push_back(files_map.size() - 1);

      inputs.push_back(phonemes[phoneme_index]);
      labels.push_back(phonemes[phoneme_index]);
    }

    corpus->add(inputs, labels);
  }

  alphabet->labels = to_array(phonemes);
  alphabet->build_classes();

  alphabet->files = to_array(files_map);
  alphabet->file_indices = to_array(file_indices);
  std::cerr << "End building alphabet" << '\n';
}

void build_data_bin(std::istream& input, PhonemeAlphabet& alphabet, Corpus<PhonemeInstance, PhonemeInstance>& corpus) {
  BinaryReader r(&input);
  unsigned alphabet_size;
  r >> alphabet_size;
  alphabet.labels.length = alphabet_size;
  alphabet.labels.data = new PhonemeInstance[alphabet_size];

  alphabet.file_indices.data = new int[alphabet_size];
  alphabet.file_indices.length = alphabet_size;

  for(unsigned i = 0; i < alphabet.size(); i++)
    r >> alphabet.labels[i] >> alphabet.file_indices[i];

  DEBUG(std::cerr << "Read " << alphabet.size() << " phonemes, " << r.bytes << " bytes\n";)

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
  DEBUG(std::cerr << "Read " << alphabet.files.length << " file names, " << r.bytes << " bytes" << std::endl;)

  unsigned corpus_size;
  r >> corpus_size;
  for(unsigned i = 0; i < corpus_size; i++) {
    r >> length;
    unsigned phonId;
    vector<PhonemeInstance> input(length);
    for(unsigned j = 0; j < length; j++) {
      r >> phonId;
      input[j] = alphabet.fromInt(phonId);
    }

    r >> length;
    vector<PhonemeInstance> labels(length);
    for(unsigned j = 0; j < length; j++) {
      r >> phonId;
      labels[j] = alphabet.fromInt(phonId);
    }

    corpus.add(input, labels);
  }

  DEBUG(std::cerr << "Read corpus " << corpus.size() << " instances, " << r.bytes << " bytes" << std::endl;)

  alphabet.build_classes();

  DEBUG(std::cerr << "Read " << r.bytes << " bytes" << std::endl;)
}

void pre_process(PhonemeAlphabet& alphabet) {
  // phonemes without pitch - assign that of the nearest neightbor with pitch
  for(unsigned i = 0; i < alphabet.files.length; i++) {
    std::vector<int> phonemes = alphabet.phonemes_of_file(i);

    double last_pitch = 0;
    for(auto it = phonemes.begin(); it != phonemes.end(); it++) {
      PhonemeInstance& phi = alphabet.fromInt(*it);
      for(auto frame_it = phi.frames.begin(); frame_it != phi.frames.end(); ++frame_it) {
        Frame& frame = *frame_it;
        if(frame.pitch == 0)
          frame.pitch = last_pitch;
        else
          last_pitch = frame.pitch;
      }
    }

    last_pitch = 0;
    for(auto it = phonemes.rbegin(); it != phonemes.rend(); it++) {
      PhonemeInstance& phi = alphabet.fromInt(*it);
      for(auto frame_it = phi.frames.rbegin(); frame_it != phi.frames.rend(); ++frame_it) {
        Frame& frame = *frame_it;
        if(frame.pitch == 0)
          frame.pitch = last_pitch;
        else
          last_pitch = frame.pitch;
      }
    }
  }

  for(unsigned i = 0; i < alphabet.labels.length; i++)
    alphabet.labels[i].pitch_contour = to_pitch_contour<true>(alphabet.labels[i]);
}

void pre_process(PhonemeAlphabet& alphabet, std::vector<PhonemeInstance>& v) {
  for(auto it = v.begin(); it != v.end(); it++)
    *it = alphabet.fromInt((*it).id);
}

void pre_process(PhonemeAlphabet& alphabet, Corpus<PhonemeInstance, PhonemeInstance>& corpus) {
  for(unsigned i = 0; i < corpus.size(); i++) {
    pre_process(alphabet, corpus.input(i));
    pre_process(alphabet, corpus.label(i));
  }
}

#endif
