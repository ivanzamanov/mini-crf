#include<exception>

#include"speech_mod.hpp"
#include"util.hpp"
#include"fourier.hpp"

using namespace util;
using std::vector;

bool SMOOTH = false;
bool SCALE_ENERGY = false;
// Mod commons

template<class T>
short truncate(T v) {
  v = std::max(v, (T) SHRT_MIN);
  v = std::min(v, (T) SHRT_MAX);
  return (short) v;
}

double hann(double i, int size) {
  return 0.5 * (1 - cos(2 * M_PI * i / size));
}

void smooth(WaveData& dest, int offset,
            const PhonemeInstance& left, const PhonemeInstance& right, double crossfadeTime);

void scaleEnergy(WaveData& data, const PhonemeInstance& phon);

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

PitchTier initPitchTier(PitchRange* tier, vector<PhonemeInstance> target, const WaveData& dest) {
  unsigned i = 0;
  frequency left = std::exp(target[i].pitch_contour[0]);
  frequency right = std::exp(target[i].pitch_contour[1]);
  int offset = 0;
  double duration = target[i].duration;
  double totalDuration = duration;
  tier[i].set(left, right, offset, dest.toSamples(duration));

  for(i++; i < target.size(); i++) {
    //offset = WaveData::toSamples(totalDuration);
    offset = 0;
    //left = (std::exp(target[i].pitch_contour[0]) + tier[i-1].right) / 2;
    left = std::exp(target[i].pitch_contour[0]);
    // Smooth out pitch at the concatenation points
    //tier[i-1].right = left;
    right = std::exp(target[i].pitch_contour[1]);

    duration = target[i].end - target[i].start;
    totalDuration += duration;
    tier[i].set(left, right, offset, dest.toSamples(duration));
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
    PsolaConstants limits(dest->sampleRate());

    // extract wave data
    destPtr->copy_from(ptr -> extractByTime(p.start, p.end));
    // copy pitch marks, translating to part-local time
    each(fileData.pitch_marks, [&](double& mark) {
        // Omit boundaries on purpose
        if(mark > p.start && mark < p.end)
          destPtr -> marks.push_back(ptr -> at_time(mark) - ptr -> at_time(p.start));
      });
    if(destPtr -> marks.size() > 1 && destPtr -> marks[0] < limits.maxVoicelessSamples) {
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

Wave SpeechWaveSynthesis::get_resynthesis_td() {
  Options opts;
  opts.add_opt("td", "");
  return get_resynthesis(opts);
}

Wave SpeechWaveSynthesis::get_resynthesis(const Options& opts) {
  // First off, collect the WAV data for each source unit
  const int N = source.size();
  Wave* sourceData = new Wave[N];
  SpeechWaveData* waveData = new SpeechWaveData[N];
  readSourceData(*this, sourceData, waveData);

  // First off, prepare for output, build some default header...
  WaveHeader h = {
    .chunkId = from_chars("RIFF"),
    .chunkSize = sizeof(WaveHeader) - 2 * sizeof(unsigned),
    .format = from_chars("WAVE"),
    .subchunkId = from_chars("fmt "),
    .subchunk1Size = 16,
    .audioFormat = 1,
    .channels = 1,
    .sampleRate = sourceData->sampleRate(),
    .byteRate = sourceData->sampleRate() * 2,
    .blockAlign = 2,
    .bitsPerSample = 16,
    .subchunk2Id = from_chars("data"),
    .samplesBytes = 0
  };
  WaveBuilder wb(h);

  // Ok, so waveData contains all the pieces with pitch marks translated to piece-local time
  double completeDuration = 0;
  each(target, [&](PhonemeInstance& p) { completeDuration += p.duration; });
  // preallocate the complete wave result,
  // but only temporarily
  WaveDataTemp result = WaveData::allocate(completeDuration, sourceData->sampleRate());

  do_resynthesis(result, waveData, opts);

  wb.append(result);

  delete[] sourceData;
  delete[] waveData;

  return wb.build();
}

int overlapAddAroundMark(const SpeechWaveData& source, const int currentMark,
                         WaveData& dest, const int destOffset,
                         const double periodLeft, const double periodRight, bool win=true) {
  DEBUG(LOG("Mark " << destOffset));
  int samplesLeft = dest.toSamples(periodLeft);
  int samplesRight = dest.toSamples(periodRight);
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
  PsolaConstants limits(dest.sampleRate);

  while(destOffset < destOffsetBound) {
    int periodSamples = limits.maxVoicelessSamplesCopy * nextRandFloat();
    const double period = dest.toDuration(periodSamples);
    int copied = overlapAddAroundMark(source, sMark, dest, destOffset, period, period);
    if(copied == 0)
      break;

    sMark += copied / scale;
    destOffset += copied;
  }
}

void SpeechWaveSynthesis::do_resynthesis(WaveData dest, SpeechWaveData* pieces,
                                         const Options& opts) {
  bool FD = !opts.has_opt("td");

  PitchRange pitchTier[target.size()];

  PitchTier pt = initPitchTier(pitchTier, target, dest);

  double totalDuration = 0;
  Progress prog(target.size(), "PSOLA: ");
  int destOffset = 0;
  for(unsigned i = 0; i < target.size(); i++) {
    SpeechWaveData& p = pieces[i];
    PhonemeInstance& tgt = target[i];
    PitchRange pitch = pt.ranges[i];

    int extraTime = 0;
    double targetDuration = tgt.end - tgt.start + extraTime;
    int extra = dest.toSamples(extraTime);

    WaveDataTemp tmp(WaveData::allocate(targetDuration, dest.sampleRate));

    double durationScale = targetDuration / dest.toDuration(p.length);
    PRINT_SCALE(i << ": duration scale " << durationScale << " at " << i);

    //int startOffsetGuide = WaveData::toSamples(totalDuration);
    int startOffset = 0;
    if(dest.toDuration(p.length) <= 1 / pitch.at_mid())
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

  if(SCALE_ENERGY) {
    prog = Progress(target.size(), "Scaling Energy: ");
    for(unsigned i = 0; i < target.size(); i++) {
      PhonemeInstance& tgt = target[i];
      scaleEnergy(dest, tgt);
      prog.update();
    }
    prog.finish();
  }
  
  if(SMOOTH) {
    prog = Progress(target.size(), "Smoothing: ");
    totalDuration = target[0].duration;

    double crossfadeTime = opts.get_opt<double>("crossfade", -1);
    bool isCFTimeSet = crossfadeTime != -1;
    for(unsigned i = 1; i < target.size() - 1; i++) {
      int offset = dest.toSamples(totalDuration);

      smooth(dest, offset, target[i], target[i+1], isCFTimeSet ? crossfadeTime : (1 / pt.at(offset)));

      PhonemeInstance& tgt = target[i];
      double targetDuration = tgt.end - tgt.start;
      totalDuration += targetDuration;
      prog.update();
    }
    prog.finish();
  }
}

int getSmoothingCount(double duration, frequency pitch, const Options& opts) {
  int MAX_SMOOTHING_COUNT = opts.get_opt<int>("smoothing-pitch-count", 1);
  int count = 1;
  while((count <= MAX_SMOOTHING_COUNT) & (duration / 2 > 1 / pitch * count))
    count++;
  count--;
  return std::max(count, 0);
}

void smooth(WaveData& dest, int offset,
            const PhonemeInstance& left,
            const PhonemeInstance& right,
            double crossfadeTime) {
  crossfadeTime = std::min({ (double) left.duration / 2, (double) right.duration / 2, crossfadeTime });

  int crossfadeSamples = dest.toSamples(crossfadeTime);
  short combined[crossfadeSamples * 2];

  short *cfLeft = combined,
   *cfRight = combined + crossfadeSamples;

  // To a temp location
  for(int i = 0; i < crossfadeSamples; i++) {
    cfLeft[i] = dest[offset - crossfadeSamples + i];
    cfRight[i] = dest[offset + i];
  }

  // Apply crossfading window
  for(int i = 0; i < crossfadeSamples; i++) {
    cfLeft[i] *= hann(i + crossfadeSamples, crossfadeSamples * 2);

    cfRight[i] *= hann(i, crossfadeSamples * 2);
  }

  // Combine
  for(int i = 0; i < crossfadeSamples; i++) {
    long l = cfLeft[i];
    long r = cfRight[i];

    cfLeft[i] = (4 * l + 2 * r) / 3;
    cfRight[i] = (2 * l + 4 * r) / 3;
  }

  // Copy back to signal
  for(int i = 0; i < crossfadeSamples * 2; i++)
    dest[offset - crossfadeSamples + i] = combined[i];
}

void scaleToPitchAndDurationSimpleFD(WaveData& dest, int& startOffset,
                                     const SpeechWaveData& source,
                                     const PitchRange& pitch,
                                     double,
                                     int debugIndex) {
  copyVoicedPartFD(source, startOffset, dest.length, 0,
                   source.length, pitch, dest, debugIndex);
}

void scaleToPitchAndDuration(WaveData& dest,
                             int& destOffset,
                             const SpeechWaveData& source,
                             const PitchRange& pitch,
                             double duration,
                             int debugIndex,
                             bool FD) {
  PsolaConstants limits(dest.sampleRate);

  // Time scale
  double scale = duration / dest.toDuration(source.length);

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
  if(nMark - mark >= limits.maxVoicelessSamples || sourceMarks.size() == 1)
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

    if(nMark - mark >= limits.maxVoicelessSamples)
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

  double sourcePitch = 1 / dest.toDuration(sourceLen);

  while (destOffset < dest.length && destOffset < destOffsetBound) {
    double destPitch = pitch.at(destOffset);
    double pitchScale = destPitch / sourcePitch;

    if (std::abs(pitchScale - 1) < 0.1)
      pitchScale = 1;

    PRINT_SCALE(debugIndex << ": pitch scale " << pitchScale << " at " << debugIndex << ", "
           << sourcePitch << " -> " << destPitch);

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
    int periodSamples = dest.toSamples(1 / pitch.at(destOffset));

    double copyPeriodRight = dest.toDuration(sourcePeriodSamplesRight);
    double copyPeriodLeft = dest.toDuration(sourcePeriodSamplesLeft);
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
  int count = duration / dest.toDuration(source.length);

  int destOffset = startOffset;
  for(int i = 0; i < count && destOffset < dest.length; i++) {
    destOffset = startOffset + i * dest.toSamples(1 / pitch.at(destOffset));
    for(int j = 0; j < source.length && destOffset < dest.length; j++, destOffset++)
      dest.plus(destOffset, source[j]);
  }
}

void scaleEnergy(WaveData& data, const PhonemeInstance& phon) {
  int offset = data.toSamples(phon.start);
  int length = data.toSamples(phon.end - phon.start);

  long energy = 0;
  for(int i = offset; i < offset + length; i++)
    energy += data[i] * data[i];

  auto energyScale = (double) phon.energy_val / energy;
  energyScale = sqrt(energyScale);

  for(int i = offset; i < offset + length; i++)
    data[i] = truncate(data[i] * energyScale);
}
