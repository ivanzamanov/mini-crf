#ifndef __WAV_HPP__
#define __WAV_HPP__

#include<cstdlib>
#include<cstring>
#include<iostream>
#include<fstream>
#include<vector>

const unsigned DEFAULT_SAMPLE_RATE = 24000;

// Does not own data
struct WaveData {
  WaveData()
    : data(0), offset(-1), length(-1)
  { }
  WaveData(short* data, int offset, int length)
    : data(data), offset(offset), length(length)
  { }

  short* data;
  int offset, length;

  WaveData range(int offset, int length) {
    return WaveData(data, this->offset + offset, length);
  }

  float duration() const { return length / DEFAULT_SAMPLE_RATE; }
  short* begin() const { return data; }
  short* end() const { return data + length; }
  unsigned size() const { return length; }

  static WaveData copy(WaveData& origin) {
    short* newData = new short[origin.length];
    memcpy(newData, origin.data, origin.length * sizeof(data[0]));
    return WaveData(newData, 0, origin.length);
  }

  static WaveData allocate(float duration) {
    int samples = duration * DEFAULT_SAMPLE_RATE;
    short* data = new short[samples];
    return WaveData(data, 0, samples);
  }
};

struct SpeechWaveData : WaveData {
  SpeechWaveData(): WaveData::WaveData() { }
  SpeechWaveData(WaveData& data): WaveData::WaveData(data) { }
  // Sample indexes that are pitch marks
  std::vector<int> marks;

  // simply the first mark
  int mark() const { return marks[0]; }
  float localDuration(int right) const { return (marks[right] - marks[right - 1]) / DEFAULT_SAMPLE_RATE; }
  float localPitch(int right) const { return 1 / localDuration(right); }

  const SpeechWaveData& operator=(const WaveData& wd) const {
    *( (WaveData*) this) = wd;
    return *this;
  }
};

// Owns data
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
  Wave(std::istream& istr) { read(istr); }
  ~Wave() {
    if(data)
      delete data;
  }

  WaveHeader h;
  unsigned char* data;

  void read(std::istream& istr) {
    istr.read((char*) &h, sizeof(h));
    data = new unsigned char[h.samplesBytes];
    istr.read((char*) data, h.samplesBytes);
  }

  void write(std::ofstream& ostr) {
    ostr.write((char*) &h, sizeof(h));
    ostr.write((char*) data, h.samplesBytes);
  }

  unsigned bytesPerSample() const { return h.bitsPerSample / 8; }
  unsigned length() const { return h.samplesBytes / bytesPerSample(); }
  float duration() const { return (float) length() / h.sampleRate; }
  // The sample index at the given time
  unsigned at_time(float time) const { return time * h.sampleRate; }

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

  WaveData extract(float start, float end) {
    unsigned startSample = at_time(start),
      endSample = at_time(end);
    return WaveData((short*) data, startSample, (endSample - startSample));
  }
};

struct WaveBuilder {
  WaveBuilder(WaveHeader h): h(h) {
    data = 0;
    this->h.samplesBytes = 0;
  }
  WaveHeader h;
  unsigned char* data;

  void append(WaveData& w) {
    append((unsigned char*) (w.data + w.offset), w.length * sizeof(w.data[0]));
  }
  
  void append(Wave& w, int offset, int count) {
    append(w.data + offset, count * w.h.bitsPerSample / 8);
  }

  void append(unsigned char* data, int count) {
    this->data = (unsigned char*) realloc(this->data, h.samplesBytes + count);
    memcpy(this->data + h.samplesBytes, data, count);

    h.samplesBytes += count;
    h.chunkSize += count;
  }

  Wave build() {
    Wave result;
    result.h = h;
    result.data = this->data;
    return result;
  }
};

#endif
