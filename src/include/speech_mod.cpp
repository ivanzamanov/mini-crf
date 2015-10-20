#include<exception>

#include"speech_mod.hpp"
#include"util.hpp"
#include"fourier.hpp"

using namespace util;
using std::vector;

// Mod commons

template<class T>
short truncate(T v) {
  v = std::max(v, (T) SHRT_MIN);
  v = std::min(v, (T) SHRT_MAX);
  return (short) v;
}

double hann(int i, int size) {
  return 0.5 * (1 - cos(2 * M_PI * i / size));
}

void smooth(WaveData& dest, int offset, frequency pitch,
            const PhonemeInstance& left, const PhonemeInstance& right);

void scaleToPitchAndDurationSimpleFD(WaveData& dest,
                                     int& startOffset,
                                     const SpeechWaveData& source,
                                     const PitchRange& pitch,
                                     double,
                                     int debugIndex);

void scaleToPitchAndDurationFD(WaveData& dest,
                               int& destOffset,
                               SpeechWaveData& source,
                               const PitchRange& pitch,
                               double duration,
                               int debugIndex);

void scaleToPitchAndDurationSimpleTD(WaveData& dest,
                                     int startOffset,
                                     const SpeechWaveData& source,
                                     const PitchRange& pitch,
                                     double duration,
                                     int debugIndex);

void copyVoicedPartFD(const SpeechWaveData& oSource,
                      int& destOffset,
                      const int destOffsetBound,
                      int mark,
                      const int nMark,
                      const PitchRange& pitch,
                      WaveData& dest,
                      int debugIndex);

void copyVoicedPartTD(const SpeechWaveData& source,
                      int& destOffset,
                      const int destOffsetBound,
                      int mark,
                      const int nMark,
                      const PitchRange& pitch,
                      WaveData& dest,
                      int debugIndex);

void scaleToPitchAndDuration(WaveData& dest,
                             int& destOffset,
                             const SpeechWaveData& source,
                             const PitchRange& pitch,
                             double duration,
                             int debugIndex,
                             bool FD);

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

template<class Arr>
static unsigned from_chars(Arr arr) {
  unsigned result = 0;
  result = arr[3];
  result = (result << 8) | arr[2];
  result = (result << 8) | arr[1];
  result = (result << 8) | arr[0];
  return result;
}

Wave SpeechWaveSynthesis::get_resynthesis(bool FD) {
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

  INFO("Using " << (FD ? "FD" : "TD") << "-PSOLA");
  do_resynthesis(result, waveData, FD);

  wb.append(result);

  delete[] sourceData;
  delete[] waveData;

  return wb.build();
}

int overlapAddAroundMark(const SpeechWaveData& source, const int currentMark,
                         WaveData& dest, const int destOffset,
                         const double periodLeft, const double periodRight, bool win=true) {
  DEBUG(LOG("Mark " << destOffset));
  int samplesLeft = WaveData::toSamples(periodLeft);
  int samplesRight = WaveData::toSamples(periodRight);
  // Will reuse window space...
  double window[2 * std::max(samplesLeft, samplesRight) + 1];

  // It's what they call...
  int destBot, destTop, sourceBot, sourceTop;
  // The Rise...
  gen_rise(window, samplesLeft, win);
  destBot = std::max(0, destOffset - samplesLeft);
  destTop = std::min(dest.length, destOffset);
  sourceBot = std::max(0, currentMark - samplesLeft);
  sourceTop = std::min(currentMark, source.length);

  for(int di = destBot,
        si = sourceBot,
        wi = 0; di < destTop && si < sourceTop; di++, si++, wi++)
    dest.plus(di, source[si] * window[wi]);

  // And Fall
  destBot = std::max(0, destOffset);
  destTop = std::min(dest.length, destOffset + samplesRight);
  sourceBot = std::max(currentMark, 0);
  sourceTop = std::min(currentMark + samplesRight, source.length);
  gen_fall(window, samplesRight, win);

  int copied = 0;
  for(int di = destBot,
        si = sourceBot,
        wi = 0; di < destTop && si < sourceTop; di++, si++, wi++, copied++)
    dest.plus(di, source[si] * window[wi]);

  return copied;
}

float nextRandFloat() {
  static int cycleMax = 4;
  static int cycle = 0;
  cycle = cycle % cycleMax + 1;

  return 0.8 + (float) cycle * 0.1;
}

void copyVoicelessPart(const SpeechWaveData& source,
                       int& destOffset,
                       const int destOffsetBound,
                       const int,
                       int sMark,
                       int sourceBound,
                       WaveData& dest) {
  int dLen = destOffsetBound - destOffset;
  float scale = (float) dLen / (sourceBound - sMark);

  while(destOffset < destOffsetBound) {
    int periodSamples = MAX_VOICELESS_SAMPLES_COPY * nextRandFloat();
    const double period = WaveData::toDuration(periodSamples);
    int copied = overlapAddAroundMark(source, sMark, dest, destOffset, period, period);
    if(copied == 0)
      break;

    sMark += copied / scale;
    destOffset += copied;
  }
}

void SpeechWaveSynthesis::do_resynthesis(WaveData dest, SpeechWaveData* pieces, bool FD) {
  PitchRange pitchTier[target.size()];

  PitchTier pt = initPitchTier(pitchTier, target);

  double totalDuration = 0;
  Progress prog(target.size(), "PSOLA: ");
  int destOffset = 0;
  for(unsigned i = 0; i < target.size(); i++) {
    SpeechWaveData& p = pieces[i];
    PhonemeInstance& tgt = target[i];
    PitchRange pitch = pt.ranges[i];

    int extraTime = 0;
    double targetDuration = tgt.end - tgt.start + extraTime;
    int extra = WaveData::toSamples(extraTime);

    WaveDataTemp tmp(WaveData::allocate(targetDuration));

    double durationScale = targetDuration / WaveData::toDuration(p.length);
    if(durationScale < 0.5 || durationScale > 2) {
      WARN("Duration scale " << durationScale << " at " << i);
    }

    //int startOffsetGuide = WaveData::toSamples(totalDuration);
    int startOffset = 0;
    if(WaveData::toDuration(p.length) <= 1 / pitch.at_mid())
      if(FD)
        scaleToPitchAndDurationSimpleFD(tmp, startOffset, p, pitch, targetDuration, i);
      else
        scaleToPitchAndDurationSimpleTD(tmp, startOffset, p, pitch, targetDuration, i);
    else
      scaleToPitchAndDuration(tmp, startOffset, p, pitch, targetDuration, i, FD);

    for(int i = 0; i < tmp.length - extra && destOffset < dest.length; i++, destOffset++)
      dest[destOffset] = tmp[i];

    totalDuration += targetDuration - extraTime;

    prog.update();
  }
  prog.finish();
  return;
  prog = Progress(target.size(), "Smoothing: ");
  totalDuration = target[0].duration;
  for(unsigned i = 1; i < target.size() - 1; i++) {
    int offset = WaveData::toSamples(totalDuration);

    smooth(dest, offset, pt.at(offset), target[i], target[i+1]);

    PhonemeInstance& tgt = target[i];
    double targetDuration = tgt.end - tgt.start;
    totalDuration += targetDuration;
    prog.update();
  }
  prog.finish();
}

int getSmoothingCount(double duration, frequency pitch) {
  int MAX_SMOOTHING_COUNT = 1;
  int count = 1;
  while((count <= MAX_SMOOTHING_COUNT) & (duration / 2 > 1 / pitch * count))
    count++;
  count--;
  return std::max(count, 0);
}

void smooth(WaveData dest, int offset, frequency pitch,
            PhonemeInstance& left, PhonemeInstance& right) {
  int countLeft = getSmoothingCount(left.duration, pitch);
  int countRight = getSmoothingCount(right.duration, pitch);

  int valuesCount = WaveData::toSamples(1 / pitch);
  short valuesLeft[valuesCount];
  dest.extract(valuesLeft, valuesCount, offset - countLeft * valuesCount);
  short valuesRight[valuesCount];
  dest.extract(valuesRight, valuesCount, offset + (countRight - 1) * valuesCount);

  int startDestOffset = offset + (countRight - 1) * valuesCount;
  int totalCount = countLeft + countRight;
  short combined[valuesCount];
  for(int i = 0; i < totalCount; i++) {
    // combine left and right
    float t = (float) i / totalCount;
    for(int j = 0; j < valuesCount; j++)
      combined[j] = valuesLeft[j] * (1 - t) + valuesRight[j] * t;

    // and copy
    for(int j = 0; j < valuesCount; j++)
      dest[startDestOffset + j] = combined[j];
    startDestOffset += valuesCount;
  }
}

void scaleToPitchAndDurationSimpleFD(WaveData& dest, int& startOffset,
                                     const SpeechWaveData& source,
                                     const PitchRange& pitch,
                                     double,
                                     int debugIndex) {
  copyVoicedPartFD(source, startOffset, dest.length, 0,
                   dest.length, pitch, dest, debugIndex);
}

void scaleToPitchAndDuration(WaveData& dest,
                             int& destOffset,
                             const SpeechWaveData& source,
                             const PitchRange& pitch,
                             double duration,
                             int debugIndex,
                             bool FD) {
  // Time scale
  double scale = duration / WaveData::toDuration(source.length);

  vector<int> sourceMarks;
  each(source.marks, [&](int v) { sourceMarks.push_back(v); });
  sourceMarks.push_back(source.length);

  vector<int> scaledMarks;
  each(sourceMarks, [&](int v) { scaledMarks.push_back(v * scale); });

  int startOffset = 0;
  int destOffsetBound;
  
  int i = 0;
  int mark = 0;
  int nMark = sourceMarks[i];
  destOffsetBound = startOffset + nMark * scale;
  if(nMark - mark >= MAX_VOICELESS_SAMPLES || sourceMarks.size() == 1)
    copyVoicelessPart(source, destOffset, destOffsetBound, 0, mark, nMark, dest);
  else
    if(FD)
      copyVoicedPartFD(source, destOffset, destOffsetBound,
                       mark, nMark, pitch, dest, debugIndex);
    else
      copyVoicedPartTD(source, destOffset, destOffsetBound,
                       nMark, nMark + (nMark - mark), pitch, dest, debugIndex);

  for(unsigned i = 0; i < sourceMarks.size() - 1; i++) {
    mark = sourceMarks[i];
    nMark = sourceMarks[i + 1];

    int scaledEnd = scale * nMark;
    destOffsetBound = startOffset + scaledEnd;

    if(nMark - mark >= MAX_VOICELESS_SAMPLES)
      copyVoicelessPart(source, destOffset, destOffsetBound, mark, mark, nMark, dest);
    else
      if(FD)
        copyVoicedPartFD(source, destOffset, destOffsetBound,
                         mark, nMark, pitch, dest, debugIndex);
      else
        copyVoicedPartTD(source, destOffset, destOffsetBound,
                         mark, nMark, pitch, dest, debugIndex);
  }

  if(sourceMarks.size() == 1)
    return;
  i = sourceMarks.size() - 2;
  mark = sourceMarks[i];
  nMark = source.length;
  destOffsetBound = startOffset + source.length * scale;
  copyVoicelessPart(source, destOffset, destOffsetBound, mark, nMark, scale, dest);
}

void copyVoicedPartFD(const SpeechWaveData& oSource,
                      int& destOffset,
                      const int destOffsetBound,
                      int mark,
                      const int nMark,
                      const PitchRange& pitch,
                      WaveData& dest,
                      int debugIndex) {
  int sourceLen = nMark - mark;
  double* values = new double[1024 * 4];
  for(int i = 0; i < sourceLen; i++) values[i] = (double) oSource[mark + i];

  // Harmonics below Nyquist frequency
  int NF = nMark - mark;
  NF /= 2;
  cdouble* frequencies = new cdouble[NF + 1];
  ft::FT(values, sourceLen, frequencies, NF);

  double sourcePitch = 1 / WaveData::toDuration(nMark - mark);

  while (destOffset < dest.length && destOffset < destOffsetBound) {
    double destPitch = pitch.at(destOffset);
    double pitchScale = destPitch / sourcePitch;

    if (std::abs(pitchScale - 1) < 0.1)
      pitchScale = 1;
    if (pitchScale >= 2 || pitchScale <= 0.5) {
      WARN("Pitch scale " << pitchScale << " at " << debugIndex);
    }

    int newPeriod = sourceLen / pitchScale;
    if (newPeriod != sourceLen)
      ft::rFT(frequencies, NF, values, newPeriod, newPeriod);
    else
      for(int i = 0; i < sourceLen; i++) values[i] = (double) oSource[mark + i];

    for(int i = 0; i < newPeriod && destOffset < dest.length; i++, destOffset++)
      dest.plus(destOffset, truncate(values[i]));
  }

  delete[] values;
  delete[] frequencies;
}

void copyVoicedPartTD(const SpeechWaveData& source,
                      int& destOffset,
                      const int destOffsetBound,
                      int mark,
                      const int nMark,
                      const PitchRange& pitch,
                      WaveData& dest,
                      int) {
  int sourcePeriodSamplesRight = nMark - mark;
  int sourcePeriodSamplesLeft = nMark - mark;

  while(destOffset <= destOffsetBound) {
    int periodSamples = WaveData::toSamples(1 / pitch.at(destOffset));

    double copyPeriodRight = WaveData::toDuration(sourcePeriodSamplesRight);
    double copyPeriodLeft = WaveData::toDuration(sourcePeriodSamplesLeft);
    overlapAddAroundMark(source, mark, dest, destOffset, copyPeriodLeft, copyPeriodRight);

    destOffset += periodSamples;
  }
}

void scaleToPitchAndDurationSimpleTD(WaveData& dest,
                                     int startOffset,
                                     const SpeechWaveData& source,
                                     const PitchRange& pitch,
                                     double duration,
                                     int) {
  // I'd rather have it rounded up
  int count = duration / WaveData::toDuration(source.length);

  int destOffset = startOffset;
  for(int i = 0; i < count && destOffset < dest.length; i++) {
    destOffset = startOffset + i * WaveData::toSamples(1 / pitch.at(destOffset));
    for(int j = 0; j < source.length && destOffset < dest.length; j++, destOffset++)
      dest.plus(destOffset, source[j]);
  }
}
