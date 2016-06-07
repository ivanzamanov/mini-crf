#include<fstream>
#include<algorithm>
#include<string>
#include<ios>
#include<unistd.h>

#include"gridsearch.hpp"
#include"tool.hpp"
#include"crf.hpp"
#include"features.hpp"
#include"threadpool.h"
#include"speech_mod.hpp"

void resynth(int argc, const char** argv) {
  Options opts;
  if(!init_tool(argc, argv, &opts))
    return;

  int inputIds[1] = { 22635 };

  std::vector<PhonemeInstance> input;
  for(auto phonId : inputIds) {
    auto pi = alphabet_synth.fromInt(phonId);
    input.push_back(pi);
    auto fileData = alphabet_synth.file_data_of(pi);
    std::cerr << "Input: " << fileData.file << std::endl;
    std::cerr << "Label: " << labels_synth.convert(pi.label) << std::endl;
    std::cerr << "Start: " << pi.start << std::endl;
    std::cerr << "End: " << pi.end << std::endl;
  }

  float targetDuration = 0.062211;
  float targetPitch = 225;

  targetPitch = std::log(targetPitch);
  
  std::vector<PhonemeInstance> output;
  for(auto p : input) {
    PhonemeInstance outputPhon;
    outputPhon.pitch_contour = p.pitch_contour;
    outputPhon.pitch_contour[0] = targetPitch;
    outputPhon.pitch_contour[1] = targetPitch;

    outputPhon.start = 0;
    outputPhon.end = targetDuration;
    outputPhon.duration = outputPhon.end;
  
    output.push_back(outputPhon);
  }

  std::string outputFile = opts.get_opt<std::string>("--output", "single.wav");
  std::ofstream wav_output(outputFile);
  Wave fdWave = SpeechWaveSynthesis(input, output, crf.alphabet())
    .get_resynthesis(true);
  Wave tdWave = SpeechWaveSynthesis(input, output, crf.alphabet())
    .get_resynthesis(false);

  fdWave.write(outputFile);
  
  double diff = 0;
  for(unsigned i = 0; i < fdWave.length(); i++)
    diff += std::abs(fdWave[i] - tdWave[i]);

  INFO("Samples: " << fdWave.length());
  INFO("TD/FD Error: " << diff);
  INFO("TD/FD Error mean: " << diff / fdWave.length());
}

bool Progress::enabled = true;
std::string gridsearch::Comparisons::metric = "";
std::string gridsearch::Comparisons::aggregate = "";
int main(int argc, const char** argv) {
  resynth(argc, argv);
}
