#include<iostream>
#include<fstream>

#include"wav.hpp"

void printWav(Wave& wav, char* file) {
  std::cout << "File: " << file << std::endl;
  std::cout << "Byte rate: " << wav.h.byteRate << std::endl;
  std::cout << "Sample rate: " << wav.h.sampleRate << std::endl;
  std::cout << "Bits per sample: " << wav.h.bitsPerSample << std::endl;
  std::cout << "Duration (s): " << wav.duration() << std::endl;
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

void concatWavs(int, char** argv) {
  std::ifstream str1(argv[1]);
  Wave wav1(str1);
  printWav(wav1, argv[1]);
  
  std::ifstream str2(argv[2]);
  Wave wav2(str2);
  printWav(wav2, argv[2]);

  float cfLeft = 0.5, cfRight = 0.5;

  unsigned left = wav1.at_time( wav1.duration() - cfLeft);
  unsigned right = wav2.at_time( cfRight );
  WaveBuilder wb(wav1.h);

  wb.append(wav1, 0, left);

  char bufLen = wav1.length() - left + right;
  char buf[bufLen];
  for(unsigned i = 0; i < bufLen; i++) {
    buf[i] = ((float) bufLen/i) * wav1[left + i] + (1 - ((float) bufLen/i)) * wav2[i];
  }
  
  wb.append(wav2, right, wav2.lenght() - right);
}

int main(int argc, char** argv) {
  //printWavs(argc, argv);
  concatWavs(argc, argv);
}
