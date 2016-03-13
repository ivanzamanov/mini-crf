#ifndef __WAV_HPP__
#define __WAV_HPP__

#include<cstdlib>
#include<cstring>
#include<iostream>
#include<fstream>
#include<vector>
#include<climits>

#include"types.hpp"

const int DEFAULT_SAMPLE_RATE = 24000;

template<class Arr>
unsigned uint_from_chars(Arr arr) {
  unsigned result = 0;
  result = arr[3];
  result = (result << 8) | arr[2];
  result = (result << 8) | arr[1];
  result = (result << 8) | arr[0];
  return result;
}

// Does not own data
struct WaveData {
  WaveData()
    : data(0), offset(-1), length(-1), sampleRate(DEFAULT_SAMPLE_RATE)
  { }
  WaveData(short* data, int offset, int length, unsigned sampleRate)
    : data(data), offset(offset), length(length), sampleRate(sampleRate)
  { }

  short* data;
  int offset, length;
  unsigned sampleRate;

  WaveData range(int offset, int length) {
    return WaveData(data, this->offset + offset, length, sampleRate);
  }
  float duration() const { return length / sampleRate; }
  short* begin() const { return data; }
  short* end() const { return data + length; }
  short& operator[](int i) const { return data[offset + i]; }
  void plus(int i, short val) {
    int newVal = (*this)[i];
    newVal += val;
    newVal = std::max(newVal, SHRT_MIN);
    newVal = std::min(newVal, SHRT_MAX);
    (*this)[i] = (short) newVal;
  }
  unsigned size() const { return length; }

  void print(int start=0, int end=-1) const {
    end = (end == -1) ? this->length : end;
    std::ofstream str("range");
    for(int i = offset + start; i < offset + end; i++)
      str << data[i] << '\n';
  }

  float toDuration(int sampleCount) const {
    return WaveData::toDuration(sampleCount, sampleRate);
  }

  int toSamples(float duration) const {
    return WaveData::toSamples(duration, sampleRate);
  }

  static float toDuration(int sampleCount, unsigned sampleRate) {
    return (float) sampleCount / sampleRate;
  }

  static int toSamples(float duration, unsigned sampleRate) {
    return duration * sampleRate;
  }

  void extract(short* dest, int count, int sourceOffset=0) const {
    std::memcpy(dest, data + offset + sourceOffset, sizeof(data[0]) * count);
  }

  template<class T>
  void extract(T* dest, int count, int sourceOffset=0) {
    for(int i = 0; i < count; i++)
      dest[i] = data[offset + sourceOffset + i];
  }

  static WaveData copy(const WaveData& origin) {
    short* newData = new short[origin.length];
    memcpy(newData, origin.data, origin.length * sizeof(data[0]));
    return WaveData(newData, 0, origin.length, origin.sampleRate);
  }

  static WaveData allocate(float duration, unsigned sampleRate) {
    int samples = duration * sampleRate;
    short* data = (short*) calloc(samples, sizeof(short));
    return WaveData(data, 0, samples, sampleRate);
  }
};

struct WaveDataTemp : public WaveData {
  WaveDataTemp(const WaveData& data)
    : WaveData(data.data, data.offset, data.length, data.sampleRate)
  { }
  ~WaveDataTemp() {
    free(data);
  }
};

struct SpeechWaveData : public WaveData {
  SpeechWaveData(): WaveData::WaveData() { }
  SpeechWaveData(WaveData data): WaveData::WaveData(data) { }
  // Sample indexes that are pitch marks
  std::vector<int> marks;

  // simply the first mark
  int mark() const { return marks[0]; }
  float localDuration() const { return ((float)length / 2) / sampleRate; }
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

  static WaveHeader default_header() {
    return {
      .chunkId = uint_from_chars("RIFF"),
        .chunkSize = sizeof(WaveHeader) - 2 * sizeof(unsigned),
        .format = uint_from_chars("WAVE"),
        .subchunkId = uint_from_chars("fmt "),
        .subchunk1Size = 16,
        .audioFormat = 1,
        .channels = 1,
        .sampleRate = DEFAULT_SAMPLE_RATE,
        .byteRate = DEFAULT_SAMPLE_RATE * 2,
        .blockAlign = 2,
        .bitsPerSample = 16,
        .subchunk2Id = uint_from_chars("data"),
        .samplesBytes = 0
        };
  }

  void setSampleBytes(unsigned bytes) {
    samplesBytes = bytes;
    chunkSize = sizeof(*this) - sizeof(subchunk1Size) - sizeof(chunkId) + samplesBytes;
  }
};

struct Wave {
  Wave(): data(0) { }
  Wave(std::istream& istr) { data = 0; read(istr); }
  ~Wave() {
    if(data)
      free(data);
  }

  WaveHeader h;
  char* data;

  void read(const FileData& data) {
    read(data.file);
  }

  void read(const std::string file) {
    std::ifstream str(file);
    read(str);
  }

  void read(std::istream& istr) {
    istr.read((char*) &h, sizeof(h));
    if(data) free(data);
    data = (char*) calloc(h.samplesBytes, 1);
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

  unsigned sampleRate() const {
    return h.sampleRate;
  }

  float toDuration(int sampleCount) const {
    return (float) sampleCount / sampleRate();
  }

  int toSamples(float duration) const {
    return duration * sampleRate();
  }

  unsigned bytesPerSample() const { return h.bitsPerSample / 8; }
  unsigned length() const { return h.samplesBytes / bytesPerSample(); }
  float duration() const { return (float) length() / h.sampleRate; }
  // The sample index at the given time
  unsigned at_time(float time) const { return (double) time * h.sampleRate; }

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

  WaveData extractBySample(int startSample, int endSample) const {
    return WaveData((short*) data, startSample, endSample - startSample, sampleRate());
  }

  WaveData extractByTime(float start, float end) const {
    return extractBySample(at_time(start), at_time(end) - 1);
  }
};

struct WaveBuilder {
  WaveBuilder(WaveHeader h): h(h) {
    data = 0;
    this->h.samplesBytes = 0;
  }
  WaveHeader h;
  char* data;

  void append(WaveData& w) {
    append((char*) (w.data + w.offset), w.length * sizeof(w.data[0]));
  }
  
  void append(Wave& w, int offset, int count) {
    append(w.data + offset, count * w.h.bitsPerSample / 8);
  }

  void append(char* data, int count) {
    this->data = (char*) realloc(this->data, h.samplesBytes + count);
    memcpy(this->data + h.samplesBytes, data, count);

    h.setSampleBytes(h.samplesBytes + count);
  }

  Wave build() {
    Wave result;
    result.h = h;
    result.data = this->data;
    return result;
  }
};

template<class F>
void each_frame(Wave& w1, Wave& w2, double width, F func) {
  int length = std::min(w1.length(), w2.length());
  int frameSamples = w1.toSamples(width);
  int frameOffset = 0;
  while(frameOffset < length) {
    int frameLength = std::min(frameSamples, length - frameOffset);
    if(frameLength <= 0)
      break;
    WaveData f1 = w1.extractBySample(frameOffset, frameOffset + frameLength);
    WaveData f2 = w2.extractBySample(frameOffset, frameOffset + frameLength);
    func(f1, f2);
    frameOffset += frameSamples;
  }
}

#endif
