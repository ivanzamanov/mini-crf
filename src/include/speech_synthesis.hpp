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
extern bool FORCE_SCALE;

namespace tool {
  typedef _Corpus<PhonemeInstance, PhonemeInstance> Corpus;

  struct PhonemeAlphabet : LabelAlphabet<PhonemeInstance> {
    vector<FileData> files;
    vector<int> file_indices;
    vector<int> old_file_indices;
    vector<unsigned> old_ids;
    vector<unsigned> new_ids;

    const LabelAlphabet<PhonemeInstance>::LabelClass get_class(const PhonemeInstance& phon) const {
      if(!FORCE_SCALE)
        return classes[phon.label];
      else {
        LabelAlphabet<PhonemeInstance>::LabelClass result;
        filter(classes[phon.label], result, phon);
        return result;
      }
    }

    bool between(double v, double min, double max) const {
      return v >= min && v <= max;
    }

    void filter(const LabelAlphabet<PhonemeInstance>::LabelClass& source,
                LabelAlphabet<PhonemeInstance>::LabelClass& target,
                const PhonemeInstance& phon) const {
      for(auto& p : source)
        if( (between(fromInt(p).duration / phon.duration, 0.5, 2)
             && between(fromInt(p).pitch_contour[0] - phon.pitch_contour[0], -0.69, 0.69)
             && between(fromInt(p).pitch_contour[1] - phon.pitch_contour[1], -0.69, 0.69)
             ) || target.empty()) {
          target.push_back(p);
        }
    }

    FileData file_data_of(const PhonemeInstance& phon) {
      return files[ file_indices[ phon.id ] ];
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

    unsigned new_id(unsigned id) {
      return new_ids[id];
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
          
          new_ids.resize(std::max((int) obj.id + 1, (int) new_ids.size()));
          new_ids[obj.id] = old_ids.size() - 1;

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
        std::string file = alphabet.file_data_of(phon).file;
        out << i << ": ";
        out << "File= " << file << " ";
        out << "Start= " << phon.start << " ";
        out << "End= " << phon.end << " ";
        out << "Label= " << label_provider.convert(phon.label) << " ";
        out << "Pitch= " << desired_pitch(desired[i]) << " ";
        out << "Duration= " << desired[i].duration << '\n';

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

    void print_textgrid(std::vector<int> &path, std::vector<PhonemeInstance> &input, StringLabelProvider& lp, const std::string file) {
      std::ofstream out(file);
      print_textgrid(path, input, lp, out);
    }
    
    void print_textgrid(std::vector<int> &path, std::vector<PhonemeInstance> &input, StringLabelProvider& lp, std::ostream& out) {
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
        str << lp.convert(phon.label) << "=" << i << "=" << phon.id;
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
