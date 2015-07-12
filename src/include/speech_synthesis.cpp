#include"speech_synthesis.hpp"

using namespace tool;

void tool::build_data_txt(std::istream& list_input, PhonemeAlphabet* alphabet, Corpus* corpus, StringLabelProvider& label_provider) {
  std::cerr << "Building label alphabet" << '\n';
  std::string buffer;
  std::vector<PhonemeInstance> phonemes;
  std::vector<int> file_indices;
  std::vector<std::string> files_map;

  while(list_input >> buffer) {
    FileData fileData = FileData::of(buffer);
    std::vector<PhonemeInstance> phonemes_from_file = parse_file(fileData, label_provider);
    list_input >> buffer;
    // This is the actual .wav ...
    fileData.file = buffer;
    alphabet->files.push_back(fileData);

    std::vector<PhonemeInstance> inputs;
    std::vector<PhonemeInstance> labels;

    for(auto& phon : phonemes_from_file) {
      int phoneme_index = phonemes.size();
      phon.id = phoneme_index;

      phonemes.push_back(phon);
      file_indices.push_back(alphabet->files.size() - 1);

      inputs.push_back(phonemes[phoneme_index]);
      labels.push_back(phonemes[phoneme_index]);
    }

    corpus->add(inputs, labels);
  }

  alphabet->labels = phonemes;
  alphabet->build_classes();

  alphabet->file_indices = file_indices;
  std::cerr << "End building alphabet" << '\n';
}

void tool::build_data_bin(std::istream& input, PhonemeAlphabet& alphabet, Corpus& corpus, StringLabelProvider& label_provider) {
  BinaryReader r(&input);

  r >> alphabet.labels;
  r >> alphabet.file_indices;

  DEBUG(std::cerr << "Read " << alphabet.size() << " phonemes, " << r.bytes << " bytes\n");
  r >> alphabet.files;
  unsigned length;
  DEBUG(std::cerr << "Read " << alphabet.files.length << " file names, " << r.bytes << " bytes" << std::endl);

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

  r >> label_provider.labels;
  DEBUG(std::cerr << "Read corpus " << corpus.size() << " instances, " << r.bytes << " bytes" << std::endl);

  alphabet.build_classes();

  DEBUG(std::cerr << "Read " << r.bytes << " bytes" << std::endl;);
}

void tool::pre_process(PhonemeAlphabet& alphabet) {
  // phonemes without pitch - assign that of the nearest neightbor with pitch
  for(unsigned i = 0; i < alphabet.files.size(); i++) {
    std::vector<int> phonemes = alphabet.phonemes_of_file(i);

    frequency last_pitch = 0;
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

  for(unsigned i = 0; i < alphabet.labels.size(); i++)
    alphabet.labels[i].pitch_contour = to_pitch_contour<true>(alphabet.labels[i]);
}

static void pre_process(PhonemeAlphabet& alphabet, std::vector<PhonemeInstance>& v) {
  for(auto it = v.begin(); it != v.end(); it++)
    *it = alphabet.fromInt((*it).id);
}

void tool::pre_process(PhonemeAlphabet& alphabet, Corpus& corpus) {
  pre_process(alphabet);
  for(unsigned i = 0; i < corpus.size(); i++) {
    pre_process(alphabet, corpus.input(i));
    pre_process(alphabet, corpus.label(i));
  }
}