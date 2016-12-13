#ifndef __WAV_HPP__
#define __WAV_HPP__

#include<memory>
#include<cassert>
#include<cstdlib>
#include<cstring>
#include<iostream>
#include<fstream>
#include<vector>
#include<climits>
#include<valarray>

#include"types.hpp"

const unsigned DEFAULT_SAMPLE_RATE = 24000;

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
    : data(0), offset(0), length(0), sampleRate(DEFAULT_SAMPLE_RATE)
  { }
  WaveData(std::shared_ptr<char> data, int offset, int length, unsigned sampleRate)
    : data(data), offset(offset), length(length), sampleRate(sampleRate)
  { }
  WaveData& operator=(const WaveData& o) = default;

  std::shared_ptr<char> data;
  int offset, length;
  unsigned sampleRate;

  WaveData range(int offset, int length) {
    assert(offset >= 0);
    assert(length > 0);
    return WaveData(data, this->offset + offset, length, sampleRate);
  }

  double duration() const { return toDuration(length); }
  short* begin() const { return (short*) data.get(); }
  short* end() const { return (short*) data.get() + length; }

  short& operator[](int i) const { return ((short*) data.get())[offset + i]; }

  template<class T>
  void plus(int i, T val) {
    double newVal = this->data.get()[i];
    newVal += val;
    newVal = std::max(newVal, (double) SHRT_MIN);
    newVal = std::min(newVal, (double) SHRT_MAX);
    this->data.get()[i] = (short) newVal;
  }
  unsigned size() const { return length; }

  void print(int start=0, int end=-1) const {
    end = (end == -1) ? this->length : end;
    std::ofstream str("range");
    for(int i = offset + start; i < offset + end; i++)
      str << data.get()[i] << '\n';
  }

  double toDuration(int sampleCount) const {
    return WaveData::toDuration(sampleCount, sampleRate);
  }

  int toSamples(double duration) const {
    return WaveData::toSamples(duration, sampleRate);
  }

  void extract(short* dest, int count, int sourceOffset=0) const {
    std::memcpy(dest, data.get() + offset + sourceOffset, sizeof(data.get()[0]) * count);
  }

  template<class T>
  void extract(T* dest, int count, int sourceOffset=0) {
    for(int i = 0; i < count; i++)
      dest[i] = data.get()[offset + sourceOffset + i];
  }

  void copy(const WaveData& src, int offset) {
    std::copy(data.get() + offset, data.get() + offset + src.length, src.data.get());
  }

  static double toDuration(int sampleCount, unsigned sampleRate) {
    return (double) sampleCount / sampleRate;
  }

  static int toSamples(double duration, unsigned sampleRate) {
    return std::round(duration * sampleRate);
  }

  static WaveData copy(const WaveData& origin) {
    auto bytes = origin.length * sizeof(data.get()[0]);
    auto newData = std::shared_ptr<char>((char*) malloc(bytes));
    memcpy(newData.get(), origin.data.get() + origin.offset, bytes);
    return WaveData(newData, 0, origin.length, origin.sampleRate);
  }

  static WaveData allocate(double duration, unsigned sampleRate) {
    int samples = duration * sampleRate;
    auto newData = std::shared_ptr<char>((char*) calloc(samples, sizeof(short)));
    return WaveData(newData, 0, samples, sampleRate);
  }

  static std::valarray<double> asValarray(WaveData wd) {
    std::valarray<double> result(wd.size());
    for(auto i = 0u; i < wd.size(); i++)
      result[i] = wd[i];
    return result;
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

// Owner of data, data treated as raw bytes
struct Wave {
  Wave() { }
  Wave(std::istream& istr):Wave() { read(istr); }
  Wave(const std::string& fileName):Wave() { read(fileName); }
  ~Wave() { }

  WaveHeader h;
  std::shared_ptr<char> data;

  void read(const FileData& data) {
    read(data.file);
  }

  void read(const std::string& file) {
    if(file == "")
      return;
    std::ifstream str(file);
    read(str);
  }

  void read(std::istream& istr) {
    istr.read((char*) &h, sizeof(h));
    data = std::shared_ptr<char>((char*) malloc(h.samplesBytes * sizeof(char)));
    istr.read(data.get(), h.samplesBytes);
  }

  void write(const std::string& file) {
    if(file == "")
      return;
    std::ofstream str(file);
    write(str);
  }

  void write(std::ofstream& ostr) {
    ostr.write((char*) &h, sizeof(h));
    ostr.write(data.get(), h.samplesBytes);
  }

  unsigned sampleRate() const {
    return h.sampleRate;
  }

  double toDuration(int sampleCount) const {
    return (double) sampleCount / sampleRate();
  }

  int toSamples(double duration) const {
    return duration * sampleRate();
  }

  unsigned bytesPerSample() const { return h.bitsPerSample / 8; }
  unsigned length() const { return h.samplesBytes / bytesPerSample(); }
  double duration() const { return (double) length() / h.sampleRate; }

  // Unchecked access to sample
  short& operator[](int i) const {
    return ((short*) data.get())[i];
  }

  WaveData extractBySample(unsigned startSample, unsigned endSample) const {
    assert(startSample < endSample);
    assert(endSample < length());

    return WaveData(data, startSample, endSample - startSample, sampleRate());
  }

  WaveData extractByTime(double start, double end) const {
    return extractBySample(toSamples(start), toSamples(end) - 1);
  }
};

struct WaveBuilder {
  WaveBuilder(WaveHeader h): h(h) {
    data = 0;
    this->h.samplesBytes = 0;
  }
  WaveHeader h;
  char* data;

  void append(const WaveData& w) {
    append((char*) (w.data.get() + w.offset), w.length * sizeof(w.data.get()[0]));
  }

  void append(const Wave& w, int offset, int count) {
    append(w.data.get() + offset, count * w.h.bitsPerSample / 8);
  }

  void append(char* data, int count) {
    this->data = (char*) realloc(this->data, h.samplesBytes + count);
    memcpy(this->data + h.samplesBytes, data, count);

    h.setSampleBytes(h.samplesBytes + count);
  }

  Wave build() {
    Wave result;
    result.h = h;
    result.data = std::shared_ptr<char>(this->data);
    return result;
  }
};

struct SpeechWaveData : public WaveData {
  SpeechWaveData(): WaveData(), extra() { }
  SpeechWaveData(const SpeechWaveData& o): WaveData(o), marks(o.marks),  extra(o.extra) { }
  
  static constexpr auto EXTRA_TIME = 0.02;
  // Sample indexes that are pitch marks
  std::vector<int> marks;
  WaveData extra;

  SpeechWaveData& operator=(const WaveData& wd) {
    data = wd.data;
    offset = wd.offset;
    length = wd.length;
    sampleRate = wd.sampleRate;
    return *this;
  }

  void init(const Wave& wav, double start, double end) {
    auto extraStart = std::max(0.0, start - EXTRA_TIME);
    auto extraEnd = std::min(wav.duration(), end + EXTRA_TIME);

    extra = WaveData::copy(wav.extractByTime(extraStart, extraEnd));
    auto thisLen = extra.toSamples(end - start);
    auto thisOffsetInExtra = extra.toSamples(start - extraStart);
    *((WaveData*) this) = extra.range(thisOffsetInExtra, thisLen);
  }

  static SpeechWaveData allocate(double duration, int sampleRate) {
    SpeechWaveData result;
    result.extra = WaveData::allocate(duration + 2 * EXTRA_TIME, sampleRate);

    result.data = result.extra.data;
    result.sampleRate = result.extra.sampleRate;

    result.offset = result.toSamples(EXTRA_TIME);
    result.length = result.toSamples(duration);
    return result;
  }

  static void toFile(const WaveData& swd, const std::string file) {
    WaveHeader h = WaveHeader::default_header();
    h.sampleRate = swd.sampleRate;
    h.byteRate = swd.sampleRate * 2;
    WaveBuilder wb(h);
    wb.append(swd);
    wb.build().write(file);
  }
  static void toFile(SpeechWaveData& swd) {
    SpeechWaveData::toFile(swd, "swd.wav");
  }
};

template<unsigned frameSize>
struct FrameQueue {
  FrameQueue(const Wave& wave, unsigned step=frameSize):
    wave(wave), offset(0), step(step) { }
  const Wave& wave;
  std::array<short, frameSize> buffer;
  unsigned offset;
  unsigned step;

  bool hasNext() const {
    return buffer.size() + offset < wave.length();
  }

  const std::array<short, frameSize>& next() {
    std::fill(buffer.begin(), buffer.end(), 0);
    WaveData wd = wave.extractBySample(offset, offset + buffer.size());
    for(auto i = 0u; i < wd.size(); i++)
      buffer[i] = wd[i];
    offset += step;
    return buffer;
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
