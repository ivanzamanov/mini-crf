#include<fstream>
#include<algorithm>
#include<string>
#include<ios>
#include<unistd.h>

#include"tool.hpp"
#include"crf.hpp"
#include"features.hpp"
#include"threadpool.h"
#include"speech_mod.hpp"

void resynth(int argc, const char** argv) {
  Options opts = parse_options(argc, argv);

  int phonId = util::parse_int(opts.input);

  std::vector<PhonemeInstance> input;
  auto pi = alphabet_synth.fromInt(phonId);
  input.push_back(pi);

  std::vector<PhonemeInstance> output;
  PhonemeInstance outputPhon;
  outputPhon.pitch_contour = pi.pitch_contour;
  outputPhon.start = 0;

  outputPhon.end = pi.duration;
  outputPhon.duration = outputPhon.end;
  
  output.push_back(outputPhon);

  std::string outputFile = opts.get_opt("--output");
  std::ofstream wav_output(outputFile);
  SpeechWaveSynthesis(input, output, crf.alphabet())
    .get_resynthesis()
    .write(wav_output);
}

void clone(int argc, const char** argv) {
  Options opts = parse_options(argc, argv);

  int phonId = util::parse_int(opts.input);

  std::vector<PhonemeInstance> input;
  auto pi = alphabet_synth.fromInt(phonId);
  input.push_back(pi);

  std::vector<PhonemeInstance> output;
  PhonemeInstance outputPhon;
  outputPhon.pitch_contour = pi.pitch_contour;
  outputPhon.start = 0;

  outputPhon.end = 2;
  outputPhon.duration = outputPhon.end;
  
  output.push_back(outputPhon);

  std::string outputFile = opts.get_opt("--output");
  std::ofstream wav_output(outputFile);
  SpeechWaveSynthesis(input, output, crf.alphabet())
    .get_resynthesis()
    .write(wav_output);
}

bool Progress::enabled = true;
int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);
  if(!init_tool(argc, argv)) {
    return 1;
  }

  resynth(argc, argv);
}
