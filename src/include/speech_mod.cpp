#include<exception>
#include"speech_mod.hpp"
#include"util.hpp"

using namespace util;
using std::vector;

#pragma GCC push_options
#pragma GCC optimize ("O0")

template<class Arr>
static unsigned from_chars(Arr arr) {
  unsigned result = 0;
  result = arr[3];
  result = (result << 8) | arr[2];
  result = (result << 8) | arr[1];
  result = (result << 8) | arr[0];
  return result;
}

void gen_fall(double* data, int size, bool window=true) {
  //transform(data, size, [&](int, double&) { return 1; });
  size = size * 2;
  transform(data, size / 2, [=](int i, double) {
      i += size/2;
      return window ? hann(i, size) : 1;
    });
}

void gen_rise(double* data, int size, bool window=true) {
  //transform(data, size, [&](int, double&) { return 1; });
  size = size * 2;
  transform(data, size / 2, [=](int i, double) {
      return window ? hann(i, size) : 1;
    });
}

double hann(int i, int size) {
  return 0.5 * (1 - cos(2 * M_PI * i / size));
}

PitchTier initPitchTier(PitchRange* tier, vector<PhonemeInstance> target) {
  unsigned i = 0;
  frequency left = std::exp(target[i].pitch_contour[0]);
  frequency right = std::exp(target[i].pitch_contour[1]);
  int offset = 0;
  double duration = target[i].duration;
  double totalDuration = duration;
  tier[i].set(left, right, offset, WaveData::toSamples(duration));

  for(i++; i < target.size(); i++) {
    //offset = WaveData::toSamples(totalDuration);
    offset = 0;
    left = (std::exp(target[i].pitch_contour[0]) + tier[i-1].right) / 2;
    // Smooth out pitch at the concatenation points
    tier[i-1].right = left;
    right = std::exp(target[i].pitch_contour[1]);

    duration = target[i].end - target[i].start;
    totalDuration += duration;
    tier[i].set(left, right, offset, WaveData::toSamples(duration));
  }
  PitchTier result =  {
    .ranges = tier,
    .length = (int) target.size()
  };
  return result;
}

static void readSourceData(SpeechWaveSynthesis& w, Wave* dest, SpeechWaveData* destParts) {
  Wave* ptr = dest;
  SpeechWaveData* destPtr = destParts;
  // TODO: possibly avoid reading a file multiple times...
  int i = 0;
  for(auto& p : w.source) {
    FileData fileData = w.origin.file_data_of(p);
    std::ifstream str(fileData.file);
    ptr -> read(str);

    // extract wave data
    destPtr->copy_from(ptr -> extractByTime(p.start, p.end));
    // copy pitch marks, translating to part-local time
    each(fileData.pitch_marks, [&](double& mark) {
        // Omit boundaries on purpose
        if(mark > p.start && mark < p.end)
          destPtr -> marks.push_back(ptr -> at_time(mark) - ptr -> at_time(p.start));
      });
    if(destPtr -> marks.size() > 1 && destPtr -> marks[0] < MAX_VOICELESS_SAMPLES) {
      int diff = destPtr -> marks[1] - destPtr -> marks[0];
      int count = (destPtr -> marks[0] - diff) / diff;
      for(int j = 0; j < count; j++)
        destPtr -> marks.insert(destPtr -> marks.begin(), (count - j) * diff);
    }

    // and move on
    ptr++;
    destPtr++;
    i++;
  }
}
Wave SpeechWaveSynthesis::get_resynthesis() {
  // First off, prepare for output, build some default header...
  WaveHeader h = {
    .chunkId = from_chars("RIFF"),
    .chunkSize = sizeof(WaveHeader) - 2 * sizeof(unsigned),
    .format = from_chars("WAVE"),
    .subchunkId = from_chars("fmt "),
    .subchunk1Size = 16,
    .audioFormat = 1,
    .channels = 1,
    .sampleRate = DEFAULT_SAMPLE_RATE,
    .byteRate = DEFAULT_SAMPLE_RATE * 2,
    .blockAlign = 2,
    .bitsPerSample = 16,
    .subchunk2Id = from_chars("data"),
    .samplesBytes = 0
  };
  WaveBuilder wb(h);

  // First off, collect the WAV data for each source unit
  const int N = source.size();
  Wave* sourceData = new Wave[N];
  SpeechWaveData* waveData = new SpeechWaveData[N];
  readSourceData(*this, sourceData, waveData);

  // Ok, so waveData contains all the pieces with pitch marks translated to piece-local time
  double completeDuration = 0;
  each(target, [&](PhonemeInstance& p) { completeDuration += p.duration; });
  // preallocate the complete wave result
  WaveData result = WaveData::allocate(completeDuration);
  result.print(0,0);

  do_resynthesis_fd(result, waveData);

  wb.append(result);

  delete[] sourceData;
  delete[] waveData;

  return wb.build();
}

#pragma GCC pop_options
