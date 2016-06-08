#ifndef __SPEECH_MOD_HPP__
#define __SPEECH_MOD_HPP__

#include<cassert>
#include<vector>

#include"speech_synthesis.hpp"
#include"wav.hpp"
#include"options.hpp"

using namespace tool;

extern bool SMOOTH;
extern bool SCALE_ENERGY;
extern int EXTRA_TIME;

struct SpeechWaveSynthesis {
  SpeechWaveSynthesis(std::vector<PhonemeInstance>& source,
                      const std::vector<PhonemeInstance>& target,
                      PhonemeAlphabet& origin)
    : source(source), target(target), origin(origin)
  { };

  std::vector<PhonemeInstance>& source;
  const std::vector<PhonemeInstance>& target;
  PhonemeAlphabet& origin;

  Wave get_resynthesis(const Options&);
  Wave get_concatenation(const Options&);
  Wave get_resynthesis_td();

  Wave get_coupling(const Options&);
private:
  void do_resynthesis(WaveData, const std::vector<SpeechWaveData>&, const Options&);
};

struct PsolaConstants {
  PsolaConstants(int sampleRate) {
    // F0 of less than 50 Hz will be considered voiceless
    maxVoicelessSamples = WaveData::toSamples(0.02, sampleRate);
    voicelessSamplesCopy = WaveData::toSamples(0.004, sampleRate);
    maxVoicelessPeriod = WaveData::toDuration(maxVoicelessSamples, sampleRate);
  }
  PsolaConstants(const PsolaConstants& o) = default;

  int maxVoicelessSamples;
  int voicelessSamplesCopy;

  double maxVoicelessPeriod;
};

struct PitchRange {
  void set(frequency left, frequency right, int offset, int length) {
    this->left = left;
    this->right = right;
    this->length = length;
    this->offset = offset;
  }

  frequency at(int index) const {
    if(left == 0 && right == 0)
      return -1;
    if(left == 0)
      return right;
    if(right == 0)
      return left;

    double c = (index - offset); // centered
    if(c < 0)
      return at(0);
    else if (c >= length)
      return at(length - 1);
    assert(!(c < 0 || c > length)); // Bad sample index
    return left * (1 - c / length) + right * c / length;
  }
  
  frequency at_mid() const { return at(length / 2); }

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
    assert(false); // Bad sample index
  }
  frequency atEnd() const {
    int offset = ranges[length - 1].offset;
    int len = ranges[length - 1].length;
    return ranges[length - 1].at(offset + len - 1);
  }
};

#endif
