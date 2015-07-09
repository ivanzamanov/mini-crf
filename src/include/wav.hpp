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
    read(istr);
  }

  void read(std::istream& istr) {
    istr.read((char*) &h, sizeof(h));
    data = new unsigned char[h.samplesBytes];
    istr.read((char*) data, h.samplesBytes);
  }

  void write(std::ostream& ostr) {
    ostr.write((char*) &h, sizeof(h));
    ostr.write((char*) data, h.samplesBytes);
  }

  unsigned bytesPerSample() const {
    return h.bitsPerSample / 8;
  }

  unsigned length() const {
    return h.samplesBytes / bytesPerSample();
  }

  float duration() const {
    return (float) length() / h.sampleRate;
  }

  // The sample index at the given time
  unsigned at_time(float time) {
    return time * h.sampleRate;
  }

  short get(int i) const {
    if(i < 0 || i >= (int) length())
      return 0;
    return ((short*) data)[i];
  }

  short& operator[](int i) {
    return ((short*) data)[i];
  }

  WaveHeader h;
  unsigned char* data;
};

struct WaveBuilder {
  WaveBuilder(WaveHeader h): h(h) {
    data = 0;
    this->h.samplesBytes = 0;
  }
  WaveHeader h;
  unsigned char* data;

  void append(Wave& w, int offset, int count) {
    append(w.data + offset, count * w.h.bitsPerSample / 8);
  }

  void append(unsigned char* data, int count) {
    this->data = (unsigned char*) realloc(this->data, h.samplesBytes + count);
    memcpy(this->data + h.samplesBytes, data, count);

    h.samplesBytes += count;
  }

  Wave build() {
    Wave result;
    result.h = h;
    result.data = this->data;
    return result;
  }
};

#endif
