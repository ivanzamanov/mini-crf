#include<random>

#include"speech_mod.hpp"
#include"util.hpp"

using namespace util;
using std::vector;

template<class Arr>
static unsigned from_chars(Arr arr) {
  unsigned result = 0;
  result = arr[3];
  result = (result << 8) | arr[2];
  result = (result << 8) | arr[1];
  result = (result << 8) | arr[0];
  return result;
}

static void readSourceData(SpeechWaveSynthesis& w, Wave* dest, SpeechWaveData* destParts) {
  Wave* ptr = dest;
  SpeechWaveData* destPtr = destParts;
  // TODO: possibly avoid reading a file multiple times...
  for(auto& p : w.source) {
    FileData fileData = w.origin.file_data_of(p);
    std::ifstream str(fileData.file);
    ptr -> read(str);

    // extract wave data
    destPtr->copy_from(ptr -> extractByTime(p.start, p.end));
    // copy pitch marks, translating to part-local time
    each(fileData.pitch_marks, [&](float& mark) {
        if(mark >= p.start && mark <= p.end)
          destPtr -> marks.push_back(ptr -> at_time(mark) - ptr -> at_time(p.start));
      });
    // and move on
    ptr++;
    destPtr++;
  }
}

void gen_window(float* data, int size) {
  //transform(data, size, [&](int i, float&) { return 0.5 * (1 - cos(2 * M_PI * i / size)); });
  transform(data, size, [&](int, float&) { return 1; });
}

void gen_fall(float* data, int size) {
  transform(data, size, [&](int, float&) { return 1; });
  //transform(data, size / 2, [&](int i, float&) { return 0.5 * (1 - cos(2 * M_PI * (i - size/2 ) / size)); });
}

void gen_rise(float* data, int size) {
  transform(data, size, [&](int, float&) { return 1; });
  //transform(data, size/2, [&](int i, float&) { return 0.5 * (1 - cos(2 * M_PI * i / size)); });
}

struct PitchRange {
  void set(frequency left, frequency right, int length) {
    this->left = left;
    this->right = right;
    this->length = length;
  }

  frequency at(int index) const { return left * (1 - (index+1) / length) + right * ((index+1)/length); }
  frequency at(float time) const { return at(WaveData::toSamples(time)); }
  frequency left;
  frequency right;
  int length;
};

void initPitchTier(PitchRange* tier, vector<PhonemeInstance> target) {
  unsigned i = 0;
  frequency left = std::exp(target[i].pitch_contour[0]);
  frequency right = std::exp(target[i].pitch_contour[1]);
  tier[i].set(left, right, WaveData::toSamples(target[i].duration));

  for(i++; i < target.size(); i++) {
    left = (std::exp(target[i].pitch_contour[0]) + tier[i-1].right) / 2;
    // Smooth out pitch at the concatenation points
    tier[i-1].right = left;
    right = std::exp(target[i].pitch_contour[1]);

    tier[i].set(left, right, WaveData::toSamples(target[i].duration));
  }
}

void overlapAddAroundMark(SpeechWaveData& source, int currentMark,
                          WaveData dest, int destOffset, float periodLeft, float periodRight) {
  int samplesLeft = WaveData::toSamples(periodLeft);
  int samplesRight = WaveData::toSamples(periodRight);
  // Will reuse window space...
  float window[2 * std::max(samplesLeft, samplesRight) + 1];

  // It's what they call...
  int destBot, destTop, sourceBot, sourceTop;
  // The Rise...
  gen_rise(window, samplesLeft);
  destBot = std::max(0, destOffset - samplesLeft);
  destTop = std::min(dest.length, destOffset);
  sourceBot = std::max(0, currentMark - samplesLeft);
  sourceTop = std::min(currentMark, source.length);

  for(int di = destBot,
        si = sourceBot,
        wi = 0; di < destTop && si < sourceTop; di++, si++, wi++)
    dest[di] += source[si] * window[wi];

  // And Fall
  destBot = std::max(0, destOffset);
  destTop = std::min(dest.length, destOffset + samplesRight);
  sourceBot = std::max(currentMark, 0);
  sourceTop = std::min(currentMark + samplesRight, source.length);
  gen_fall(window, samplesRight);

  for(int di = destBot,
        si = sourceBot,
        wi = 0; di < destTop && si < sourceTop; di++, si++, wi++)
    dest[di] += source[si] * window[wi];

  /*std::cerr << "(" << std::max(0, currentMark - samplesLeft) + source.offset
            << "," << std::min(currentMark + samplesRight, dest.length) + source.offset
            << ",dest=" << destOffset
            << ")" << std::endl;*/
}

void copyVoicedPart(SpeechWaveData& source, int sourceMarkIndex, float scale,
                    const PitchRange pitch, WaveData dest, int& destOffset) {
  int currentMark = source.marks[sourceMarkIndex];
  int lastMark = (sourceMarkIndex > 0) ? source.marks[sourceMarkIndex - 1] : 0;
  int scaledMark = currentMark * scale;
  while(currentMark <= scaledMark) {
    float periodLeft = 1 / pitch.at(lastMark);
    float periodRight = 1 / pitch.at(currentMark);
    std::cerr << "(" << WaveData::toDuration(destOffset - WaveData::toSamples(periodLeft)) << " - "
              << WaveData::toDuration(destOffset + WaveData::toSamples(periodRight)) << ")" << std::endl;
    overlapAddAroundMark(source, currentMark, dest, destOffset, periodLeft, periodRight);

    lastMark = currentMark;
    destOffset += WaveData::toSamples(periodLeft);
    currentMark += WaveData::toSamples(periodLeft);
  }
}

void copyVoicelessPart(SpeechWaveData& source, int& destOffset, int voicelessStart, int voicelessEnd,
                       float scale, WaveData dest) {
  std::default_random_engine generator(1);
  std::uniform_real_distribution<float> distribution (0.004, 0.006);

  int mark = voicelessStart;
  while(mark <= voicelessEnd * scale) {
    const float period = distribution(generator);
    const int periodSamples = WaveData::toSamples(period);
    std::cerr << "(" << WaveData::toDuration(destOffset - WaveData::toSamples(period)) << " - "
              << WaveData::toDuration(destOffset + WaveData::toSamples(period)) << ")" << std::endl;
    overlapAddAroundMark(source, mark, dest, destOffset, period, period);

    destOffset += periodSamples;
    mark += periodSamples;
  }
}

void scaleToPitchAndDuration(WaveData dest, int& destOffset,
                             SpeechWaveData& source, PitchRange pitch, float duration) {
  // F0 of less than 50 Hz will be considered voiceless
  const int MAX_VOICELESS_SAMPLES = WaveData::toSamples(0.02f);
  // Time scale
  float scale = duration / WaveData::toDuration(source.length);
  // Copy of the source marks, but with additional at 0 and length
  vector<int> sourceMarks;
  sourceMarks.push_back(0);
  each(source.marks, [&](int v) { sourceMarks.push_back(v); });
  sourceMarks.push_back(source.length);

  int voicelessStart = 0, voicelessEnd = 0;
  // Ok, this takes care of the voiced parts
  for(unsigned i = 0; i < sourceMarks.size() - 2; i++) {
    voicelessStart = sourceMarks[i], voicelessEnd = sourceMarks[i + 1];
    while (voicelessEnd - voicelessStart > MAX_VOICELESS_SAMPLES) {
      copyVoicelessPart(source, voicelessStart, destOffset, voicelessEnd, scale, dest);
      voicelessStart = voicelessEnd;
      voicelessEnd += MAX_VOICELESS_SAMPLES;
    }

    if(voicelessStart + MAX_VOICELESS_SAMPLES > sourceMarks[i + 1])
      copyVoicedPart(source, i, scale, pitch, dest, destOffset);
  }
}

void SpeechWaveSynthesis::do_resynthesis(WaveData dest, SpeechWaveData* pieces) {
  PitchRange pitchTier[target.size()];
  initPitchTier(pitchTier, target);

  int offset = 0;
  for(unsigned i = 0; i < target.size(); i++) {
    SpeechWaveData& p = pieces[i];
    PhonemeInstance& tgt = target[i];
    float duration = tgt.end - tgt.start;
    scaleToPitchAndDuration(dest, offset, p, pitchTier[i], duration);
  }
}

Wave SpeechWaveSynthesis::get_resynthesis() {
  // First off, prepare for output, build some default header...
  WaveHeader h = {
  chunkId: from_chars("RIFF"),
  chunkSize: sizeof(WaveHeader) - 2 * sizeof(unsigned),
  format: from_chars("WAVE"),
  subchunkId: from_chars("fmt "),
  subchunk1Size: 16,
  audioFormat: 1,
  channels: 1,
  sampleRate: DEFAULT_SAMPLE_RATE,
  byteRate: DEFAULT_SAMPLE_RATE * 2,
  blockAlign: 2,
  bitsPerSample: 16,
  subchunk2Id: from_chars("data"),
  samplesBytes: 0
  };
  WaveBuilder wb(h);

  // First off, collect the WAV data for each source unit
  const int N = source.size();
  Wave sourceData[N];
  SpeechWaveData waveData[N];
  readSourceData(*this, sourceData, waveData);
  /*WaveBuilder wbt(h);
  each(waveData, N, [&](SpeechWaveData& swd) {
      wbt.append(swd);
    });
    wbt.build().write("test.wav");*/

  // Ok, so waveData contains all the pieces with pitch marks translated to piece-local time
  float completeDuration = 0;
  each(target, [&](PhonemeInstance& p) { completeDuration += p.duration; });
  // preallocate the complete wave result
  WaveData result = WaveData::allocate(completeDuration);
  do_resynthesis(result, waveData);

  wb.append(result);
  return wb.build();
}
