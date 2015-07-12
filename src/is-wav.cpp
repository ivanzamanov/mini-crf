#include<iostream>
#include<fstream>
#include<cmath>

#include"wav.hpp"

#define PRINT(X) std::cout << #X": " << wav.h.X << std::endl
void printWav(Wave& wav, char* file) {
  std::cout << "File: " << file << std::endl;
  /*  std::cout << "Byte rate: " << wav.h.byteRate << std::endl;
  std::cout << "Sample rate: " << wav.h.sampleRate << std::endl;
  std::cout << "Bits per sample: " << wav.h.bitsPerSample << std::endl;
  std::cout << "Duration (s): " << wav.duration() << std::endl;*/
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
  std::cout << "Samples: " << wav.h.samplesBytes / (wav.h.bitsPerSample / 8) << std::endl;
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

  float cfLeft = 0.001, cfRight = 0.001;

  unsigned left = wav1.at_time( wav1.duration() - cfLeft);
  unsigned right = wav2.at_time( cfRight );
  WaveBuilder wb(wav1.h);

  wb.append(wav1, 0, left);
  int bufLen = std::max(wav1.length() - left, right);
  // Assume 16 bits per sample...
  short buf[bufLen];
  std::cout << bufLen << " smoothing samples" << std::endl;
  for(int i = 1; i <= bufLen; i++) {
    float t = (float) i / bufLen;

    short v1 = (1 - t) * wav1.get(left + i);
    short v2 = t * wav2.get(i);
    buf[i - 1] = v1 + v2;
  }

  wb.append((unsigned char*) buf, bufLen * sizeof(buf[0]));

  wb.append(wav2, right, wav2.length());

  Wave output = wb.build();
  printWav(output, argv[3]);
  std::ofstream result(argv[3]);
  output.write(result);
}

int main(int argc, char** argv) {
  printWavs(argc, argv);
  //concatWavs(argc, argv);
}
