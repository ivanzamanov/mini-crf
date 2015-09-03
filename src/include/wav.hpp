#ifndef __WAV_HPP__
#define __WAV_HPP__

#include<cstdlib>
#include<cstring>
#include<iostream>
#include<fstream>
#include<vector>
#include<climits>

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

  WaveData range(int offset, int length) { return WaveData(data, this->offset + offset, length); }
  float duration() const { return length / DEFAULT_SAMPLE_RATE; }
  short* begin() const { return data; }
  short* end() const { return data + length; }
  short& operator[](int i) const { return data[offset + i]; }
//  #pragma GCC push_options
//#pragma GCC optimize ("O0")
  void plus(int i, short val) {
    int newVal = (*this)[i];
    newVal += val;
    newVal = std::max(newVal, SHRT_MIN);
    newVal = std::min(newVal, SHRT_MAX);
    (*this)[i] = (short) newVal;
  }
//#pragma GCC pop_options
  unsigned size() const { return length; }

  void print(int start=0, int end=-1) const {
    end = (end == -1) ? this->length : end;
    std::ofstream str("range");
    for(int i = offset + start; i < offset + end; i++)
      str << data[i] << '\n';
  }

  static WaveData copy(WaveData& origin) {
    short* newData = new short[origin.length];
    memcpy(newData, origin.data, origin.length * sizeof(data[0]));
    return WaveData(newData, 0, origin.length);
  }

  static WaveData allocate(float duration) {
    int samples = duration * DEFAULT_SAMPLE_RATE;
    short* data = new short[samples];
    memset(data, 0, samples * sizeof(data[0]));
    return WaveData(data, 0, samples);
  }

  static float toDuration(int sampleCount) {
    return (float) sampleCount / DEFAULT_SAMPLE_RATE;
  }

  static int toSamples(float duration) {
    return duration * DEFAULT_SAMPLE_RATE;
  }
};

struct SpeechWaveData : WaveData {
  SpeechWaveData(): WaveData::WaveData() { }
  SpeechWaveData(WaveData data): WaveData::WaveData(data) { }
  // Sample indexes that are pitch marks
  std::vector<int> marks;

  frequency markFrequency;

  // simply the first mark
  int mark() const { return marks[0]; }
  float localDuration() const { return ((float)length / 2) / DEFAULT_SAMPLE_RATE; }
  float localPitch() const { return 1 / localDuration(); }

  const SpeechWaveData& copy_from(const WaveData& wd) const {
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
    if(data) delete data;
    data = new unsigned char[h.samplesBytes];
    istr.read((char*) data, h.samplesBytes);
  }

  void write(std::string file) {
    std::ofstream str(file);
    write(str);
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

  WaveData extractBySample(int startSample, int endSample) {
    return WaveData((short*) data, startSample, (endSample - startSample) - 1);
  }

  WaveData extractByTime(float start, float end) {
    return extractBySample(at_time(start), at_time(end) - 1);
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
