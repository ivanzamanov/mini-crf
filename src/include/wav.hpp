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

  WaveHeader h;
  unsigned char* data;

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
  unsigned at_time(float time) const {
    return time * h.sampleRate;
  }

  // Checked access to sample
  short get(int i) const {
    if(i < 0 || i >= (int) length())
      return 0;
    return ((short*) data)[i];
  }

  // Unchecked access to sample
  short& operator[](int i) const {
    return ((short*) data)[i];
  }
};

struct WaveData {
  WaveData(short* data, int offset, int length)
    : data(data), offset(offset), length(length)
  { }

  short* data;
  int offset, length;

  WaveData range(int offset, int length) {
    return WaveData(data, this->offset + offset, length);
  }

  WaveData copy() {
    short* newData = new short[length];
    memcpy(newData, data, length * sizeof(data[0]));
    return WaveData(newData, 0, length);
  }

  short* begin() const { return data; }
  short* end() const { return data + length; }
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
