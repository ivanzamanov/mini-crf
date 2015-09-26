#ifndef __SPEECH_MOD_HPP__
#define __SPEECH_MOD_HPP__

#include<vector>

#include"speech_synthesis.hpp"
#include"wav.hpp"

using namespace tool;

struct SpeechWaveSynthesis {
  SpeechWaveSynthesis(std::vector<PhonemeInstance>& source,
             std::vector<PhonemeInstance>& target,
             PhonemeAlphabet& origin)
    : source(source), target(target), origin(origin)
  { };

  std::vector<PhonemeInstance>& source;
  std::vector<PhonemeInstance>& target;
  PhonemeAlphabet& origin;

  Wave get_resynthesis();
private:
  void do_resynthesis_td(WaveData, SpeechWaveData*);
  void do_resynthesis_fd(WaveData, SpeechWaveData*);
};

// F0 of less than 50 Hz will be considered voiceless
const int MAX_VOICELESS_SAMPLES = WaveData::toSamples(0.02f) * 0.9;
const double MAX_VOICELESS_PERIOD = WaveData::toDuration(MAX_VOICELESS_SAMPLES);
const int MAX_VOICELESS_SAMPLES_COPY = WaveData::toSamples(0.01f) * 0.9;
const double MAX_VOICELESS_PERIOD_COPY = WaveData::toDuration(MAX_VOICELESS_SAMPLES_COPY);

struct PitchRange {
  void set(frequency left, frequency right, int offset, int length) {
    this->left = left;
    this->right = right;
    this->length = length;
    this->offset = offset;
  }

  frequency at(int index) const {
    double c = (index+1 - offset); // centered
    if(c < 0 || c > length)
      throw std::out_of_range("Pitch from range");
    return left * (1 - c / length) + right * c / length;
  }

  frequency at(double time) const { return at(WaveData::toSamples(time)); }
  frequency left;
  frequency right;
  int length;
  int offset;
};

struct PitchTier {
  PitchRange* ranges;
  int length;
  frequency at(int sample) const {
    for(int i = 0; i < length; i++)
      if( ranges[i].offset + ranges[i].length > sample)
        return ranges[i].at(sample);
    return atEnd();
    // Shouldn't happen so this will break stuff
    throw std::out_of_range("Out of range " + sample);
  }
  frequency atEnd() const {
    int offset = ranges[length - 1].offset;
    int len = ranges[length - 1].length;
    return ranges[length - 1].at(offset + len - 1);
  }
};

PitchTier initPitchTier(PitchRange* tier, vector<PhonemeInstance> target);
double hann(int i, int size);
void gen_rise(double* data, int size);
void gen_fall(double* data, int size);

#endif
