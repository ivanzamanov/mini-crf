#include<iostream>
#include<fstream>
#include<cmath>

#include"util.hpp"
#include"wav.hpp"

void printWav(Wave& wav, char* file) {
  std::cout << "File: " << file << std::endl;
  /*  std::cout << "Byte rate: " << wav.h.byteRate << std::endl;
  std::cout << "Sample rate: " << wav.h.sampleRate << std::endl;
  std::cout << "Bits per sample: " << wav.h.bitsPerSample << std::endl;
  std::cout << "Duration (s): " << wav.duration() << std::endl;*/
#define PRINT(X) std::cout << #X": " << wav.h.X << std::endl
  PRINT(chunkId);
  PRINT(chunkSize);
  PRINT(format);
  PRINT(subchunkId);
  PRINT(subchunk1Size);
  PRINT(audioFormat);
  PRINT(channels);
  PRINT(sampleRate);
  PRINT(byteRate);
  PRINT(blockAlign);
  PRINT(bitsPerSample);
  PRINT(subchunk2Id);
  PRINT(samplesBytes);
  int samples = wav.h.samplesBytes / (wav.h.bitsPerSample / 8);
  std::cout << "Samples: " << samples << std::endl <<
    file << " Duration: " << (float) samples / wav.h.sampleRate << std::endl;
}

void printWavs(int argc, char** argv) {
  std::ifstream str(argv[1]);
  Wave wav(str);
  printWav(wav, argv[1]);

  for(int i = 2; i < argc; i++) {
    std::cout << "----" << std::endl;
    std::ifstream str1(argv[i]);
    Wave wav1(str1);
    printWav(wav1, argv[i]);
  }
}

int main(int argc, char** argv) {
  printWavs(argc, argv);
}
