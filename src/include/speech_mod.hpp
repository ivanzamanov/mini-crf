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
  void do_resynthesis(WaveData, SpeechWaveData*);
};

#endif
