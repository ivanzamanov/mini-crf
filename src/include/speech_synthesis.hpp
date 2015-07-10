#ifndef __SPEECH_SYNTHESIS_H__
#define __SPEECH_SYNTHESIS_H__

#include<fstream>
#include<iostream>
#include<sstream>

#include"crf.hpp"
#include"textgrid.hpp"
#include"types.hpp"
#include"parser.hpp"

using std::vector;

namespace tool {
  typedef _Corpus<PhonemeInstance, PhonemeInstance> Corpus;

  struct PhonemeAlphabet : LabelAlphabet<PhonemeInstance> {
    vector<FileData> files;
    vector<int> file_indices;
    vector<int> old_file_indices;
    vector<unsigned> old_ids;

    const LabelAlphabet<PhonemeInstance>::LabelClass& get_class(const PhonemeInstance& phon) const {
      return classes[phon.label];
    }

    std::string file_of(PhonemeInstance& phon) { return file_of(phon.id); }
    std::string file_of(int phonId) { return file_data_of(phonId).file; }
    FileData file_data_of(PhonemeInstance& phon) { return file_data_of(phon.id); }
    FileData file_data_of(int phonId) { return files[file_indices[phonId]]; }

    std::string old_file_of(int phonId) {
      auto index = old_file_indices[phonId];
      return files[index].file;
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
      for(unsigned i = 0; i < file_indices.size() && (file_indices[i] <= file_index); i++) {
        if(file_indices[i] == file_index)
          result.push_back(i);
      }
      return result;
    }

    unsigned old_id(unsigned id) {
      return old_ids[id];
    }

    void optimize() {
      build_classes();
      vector<PhonemeInstance> new_labels;
      vector<int> new_file_indices;

      id_t index = 0;
      for(unsigned i = 0; i < classes.size(); i++) {
        for(auto it = classes[i].begin(); it != classes[i].end(); it++) {
          PhonemeInstance obj = fromInt(*it);

          new_file_indices.push_back(file_indices[obj.id]);
          old_ids.push_back(obj.id);

          obj.id = index;
          new_labels.push_back(obj);
          index++;
        }
      }

      labels = new_labels;

      old_file_indices = file_indices;
      file_indices = new_file_indices;
      build_classes();
    }
  };

  struct SynthPrinter {
    SynthPrinter(PhonemeAlphabet& alphabet,
                 StringLabelProvider& provider)
      : alphabet(alphabet), label_provider(provider) { }

    PhonemeAlphabet& alphabet;
    StringLabelProvider& label_provider;

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
        out << "Label=" << label_provider.convert(phon.label) << " ";
        out << "Pitch=" << desired_pitch(desired[i]) << " ";
        out << "Duration=" << desired[i].duration << '\n';

        unsigned old_id = alphabet.old_id(id);
        phonemeIds << id << "=" << phon.label << " ";
        if(i > 0 && old_id != (alphabet.old_id(path[i-1]) + 1))
          run_lengths << "|";
        run_lengths << label_provider.convert(phon.label);
      }

      std::cerr << phonemeIds.str() << std::endl;
      std::cerr << "Output: " << run_lengths.str() << std::endl;
    }

    const std::string desired_pitch(const PhonemeInstance& p) {
      std::stringstream str;
      frequency mid = 0;
      for(auto frame : p.frames)
        mid += frame.pitch;
      mid = mid / p.frames.size();
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
      stime_t time_offset = 0;
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

  void build_data_txt(std::istream& list_input, PhonemeAlphabet* alphabet, Corpus* corpus, StringLabelProvider& label_provider);
  void build_data_bin(std::istream& input, PhonemeAlphabet& alphabet, Corpus& corpus, StringLabelProvider& label_provider);

  void pre_process(PhonemeAlphabet&);
  void pre_process(PhonemeAlphabet&, Corpus&);
}
#endif
