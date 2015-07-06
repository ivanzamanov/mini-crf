#ifndef __WAV_HPP__
#define __WAV_HPP__

#include<cstdlib>
#include<cstring>
#include<iostream>

struct WaveHeader {
  unsigned chunkId,
    chunkSize,
    format,
    subchunkId,
    subchunk1Size;
  unsigned short audioFormat,
    channels;
  unsigned sampleRate,
    byteRate;
  unsigned short blockAlign,
    bitsPerSample;
  unsigned subchunk2Id,
    samplesBytes;
};

struct Wave {
  Wave(): data(0) { }
  Wave(std::istream& istr) {
    istr.read((char*) &h, sizeof(h));
    data = new unsigned char[h.samplesBytes];
    istr.read((char*) data, h.samplesBytes);

    bytesPerSample = (h.bitsPerSample / 8);
  }

  unsigned length() const {
    return h.samplesBytes / bytesPerSample;
  }

  float duration() const {
    return (float) length() / ((float) h.sampleRate);
  }

  // The sample index at the given time
  unsigned at_time(float time) {
    return time * h.sampleRate;
  }

  unsigned char& operator[](int i) {
    return data[i / bytesPerSample];
  }

  WaveHeader h;
  unsigned bytesPerSample;
  unsigned char* data;
};

struct WaveBuilder {
  WaveBuilder(WaveHeader h): h(h) {
    data = 0;
    length = 0;
  }
  WaveHeader h;
  unsigned char* data;
  int length;

  void append(Wave& w, int offset, int count) {
    append(w.data + offset, count);
  }

  void append(unsigned char* data, int count) {
    this->data = (unsigned char*) realloc(data, length + count);
    memcpy(data + length, data, count);
  }
};

#endif
