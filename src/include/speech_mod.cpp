#include<exception>
#include<algorithm>

#include"speech_mod.hpp"
#include"util.hpp"
#include"fourier.hpp"

using namespace util;
using std::vector;

bool SMOOTH = false;
bool SCALE_ENERGY = false;
int EXTRA_TIME = 0;
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

static int readSourceData(SpeechWaveSynthesis& w, SpeechWaveData* destParts) {
  SpeechWaveData* destPtr = destParts;
  // TODO: possibly avoid reading a file multiple times...
  int i = 0;
  int result = 0;
  for(auto& p : w.source) {
    FileData fileData = w.origin.file_data_of(p);
    Wave wav(fileData.file);
    result = wav.sampleRate();
    PsolaConstants limits(wav.sampleRate());

    // extract wave data
    destPtr->copy_from(wav.extractByTime(p.start, p.end));
    // copy pitch marks, translating to part-local sample
    for(auto mark : fileData.pitch_marks) {
      // Omit boundaries on purpose
      if(mark > p.start && mark < p.end)
        destPtr -> marks.push_back(wav.at_time(mark) - wav.at_time(p.start));
    }

    if(destPtr -> marks.size() > 1 && destPtr -> marks[0] < limits.maxVoicelessSamples) {
      // ...now why do I do this?! ...
      int diff = destPtr -> marks[1] - destPtr -> marks[0];
      int count = (destPtr -> marks[0] - diff) / diff;
      for(int j = 0; j < count; j++)
        destPtr -> marks.insert(destPtr->marks.begin(), (count - j) * diff);
    }

    // and move on
    destPtr++;
    i++;
  }
  return result;
}

Wave SpeechWaveSynthesis::get_resynthesis_td() {
  Options opts;
  opts.log = false;
  opts.add_opt("td", "");
  auto result = get_resynthesis(opts);
  /*auto result2 = get_resynthesis(opts);

  assert(result.length() == result2.length());
  for(auto i = 0u; i < result2.length(); i++)
  assert(result[i] == result2[i]);*/

  return result;
}

Wave SpeechWaveSynthesis::get_coupling(const Options& opts) {
  const auto N = source.size();
  SpeechWaveData* waveData = new SpeechWaveData[N];
  auto sampleRate = readSourceData(*this, waveData);

  WaveHeader h = WaveHeader::default_header();
  h.sampleRate = sampleRate;
  h.byteRate = sampleRate * 2;

  WaveBuilder wb(h);

  double completeDuration = 0;
  each(target, [&](const PhonemeInstance& p) { completeDuration += p.duration; });
  WaveData result = WaveData::allocate(completeDuration, sampleRate);

  do_coupling(result, waveData, opts);
  
  wb.append(result);

  std::for_each(waveData, waveData + N, WaveData::deallocate);
  delete[] waveData;

  return wb.build();
}

Wave SpeechWaveSynthesis::get_resynthesis(const Options& opts) {
  // First off, collect the WAV data for each source unit
  const auto N = source.size();
  SpeechWaveData* waveData = new SpeechWaveData[N];
  auto sampleRate = readSourceData(*this, waveData);

  /*Wave* someSourceData2 = new Wave[N];
  SpeechWaveData* waveData2 = new SpeechWaveData[N];
  readSourceData(*this, someSourceData2, waveData2);
  for(auto i = 0u; i < N; i++) {
    for(auto j = 0u; j < someSourceData[i].length(); j++)
      assert(someSourceData[i][j] == someSourceData2[i][j]);
    for(auto j = 0u; j < waveData[i].size(); j++)
      assert(waveData[i][j] == waveData2[i][j]);
    for(auto j = 0u; j < waveData2[i].marks.size(); j++)
      assert(waveData2[i].marks[j] == waveData2[i].marks[j]);
  }
  */
  
  // First off, prepare for output, build some default header...
  WaveHeader h = WaveHeader::default_header();
  h.sampleRate = sampleRate;
  h.byteRate = sampleRate * 2;

  WaveBuilder wb(h);

  // Ok, so waveData contains all the pieces with pitch marks translated to piece-local time
  double completeDuration = 0;
  each(target, [&](const PhonemeInstance& p) { completeDuration += p.duration; });
  // preallocate the complete wave result,
  // but only temporarily
  WaveDataTemp result(WaveData::allocate(completeDuration, sampleRate));
  //INFO(completeDuration << " in " << result.size() << " samples");
  do_resynthesis(result, waveData, opts);

  /*WaveDataTemp result2 = WaveData::allocate(completeDuration, someSourceData->sampleRate());
  do_resynthesis(result2, waveData, opts);
  for(auto i = 0u; i<result.size(); i++)
    assert(result[i] == result2[i]);
  */

  wb.append(result);

  std::for_each(waveData, waveData + N, WaveData::deallocate);
  delete[] waveData;

  return wb.build();
}

void SpeechWaveSynthesis::do_coupling(WaveData dest,
                                      SpeechWaveData* pieces,
                                      const Options& opts) {
  
}

int overlapAddAroundMark(const WaveData& source, const int currentMark,
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

double nextRandFloat() {
  return 1;
  /*static int cycleMax = 4;
  static int cycle = 0;
  cycle = cycle % cycleMax + 1;

  return 0.8 + (double) cycle * 0.1;*/
}

void copyVoicelessPart(const SpeechWaveData& source,
                       int& destOffset,
                       const int destOffsetBound,
                       const int,
                       int sMark,
                       int sourceBound,
                       WaveData& dest) {
  auto dLen = destOffsetBound - destOffset;
  auto scale = (double) dLen / (sourceBound - sMark);
  PsolaConstants limits(dest.sampleRate);

  //WaveDataTemp tmp(WaveData::copy(dest));

  while(destOffset < destOffsetBound) {
    auto periodSamples = limits.maxVoicelessSamplesCopy * nextRandFloat();
    const auto period = dest.toDuration(periodSamples);
    auto copied = overlapAddAroundMark(source, sMark, dest, destOffset, period, period);
    //overlapAddAroundMark(source, sMark, tmp, destOffset, period, period);
    if(copied == 0)
      break;

    sMark += copied / scale;
    destOffset += copied;
  }

  //for(auto i = 0u; i < tmp.size(); i++)
  //  assert(tmp[i] == dest[i]);
}

void scalePiece(WaveData dest, WaveData tmp, int startOffset, SpeechWaveData& p, PitchRange pitch, double targetDuration, int i, bool FD) {
  if(dest.toDuration(p.length) <= 1 / pitch.at_mid())
    if(FD)
      scaleToPitchAndDurationSimpleFD(tmp, startOffset, p, pitch, targetDuration, i);
    else
      scaleToPitchAndDurationSimpleTD(tmp, startOffset, p, pitch, targetDuration, i);
  else
    scaleToPitchAndDuration(tmp, startOffset, p, pitch, targetDuration, i, FD);
}

void SpeechWaveSynthesis::do_resynthesis(WaveData dest, SpeechWaveData* pieces,
                                         const Options& opts) {
  bool FD = !opts.has_opt("td");

  PitchRange pitchTier[target.size()];

  PitchTier pt = initPitchTier(pitchTier, target, dest);

  double totalDuration = 0;
  Progress prog(target.size(), "PSOLA: ");
  int destOffset = 0;

  for(auto i = 0u; i < target.size(); i++) {
    SpeechWaveData& p = pieces[i];
    auto& tgt = target[i];
    PitchRange pitch = pt.ranges[i];

    auto targetDuration = tgt.end - tgt.start;

    WaveDataTemp tmp(WaveData::allocate(targetDuration, dest.sampleRate));

    double durationScale = targetDuration / dest.toDuration(p.length);
    PRINT_SCALE(i << ": duration scale " << durationScale << " at " << i);

    scalePiece(dest, tmp, 0, p, pitch, targetDuration, i, FD);

    /*WaveDataTemp tmp2(WaveData::allocate(targetDuration, dest.sampleRate));
    scalePiece(dest, tmp2, 0, p, pitch, targetDuration, i, FD);
    for(auto i = 0u; i < tmp.size(); i++)
    assert(tmp[i] == tmp2[i]);*/

    for(auto i = 0; i < tmp.length && destOffset < dest.length; i++, destOffset++)
      dest[destOffset] = tmp[i];

    totalDuration += targetDuration;

    prog.update();
  }
  prog.finish();

  if(SCALE_ENERGY) {
    prog = Progress(target.size(), "Scaling Energy: ");
    for(auto i = 0u; i < target.size(); i++) {
      scaleEnergy(dest, target[i]);
      prog.update();
    }
    prog.finish();
  }

  if(SMOOTH) {
    prog = Progress(target.size(), "Smoothing: ");
    totalDuration = target[0].duration;

    auto crossfadeTime = opts.get_opt<double>("crossfade", -1);
    auto isCFTimeSet = crossfadeTime != -1;
    for(auto i = 1u; i < target.size() - 1; i++) {
      int offset = dest.toSamples(totalDuration);

      smooth(dest, offset, target[i], target[i+1], isCFTimeSet ? crossfadeTime : (1 / pt.at(offset)));

      auto& tgt = target[i];
      totalDuration += tgt.end - tgt.start;
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

    cfLeft[i] = (4 * l + 2 * r) / 6;
    cfRight[i] = (2 * l + 4 * r) / 6;
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

  auto startOffset = 0,
    i = 0,
    mark = 0,
    nMark = sourceMarks[i],
    destOffsetBound = startOffset + (int) (nMark * scale);

  if(nMark - mark >= limits.maxVoicelessSamples || sourceMarks.size() == 1)
    copyVoicelessPart(source, destOffset, destOffsetBound, 0, mark, nMark, dest);
  else
    if(FD)
      copyVoicedPartFD(source, destOffset, destOffsetBound,
                       mark, nMark, pitch, dest, debugIndex);
    else
      copyVoicedPartTD(source, destOffset, destOffsetBound,
                       nMark, nMark + (nMark - mark), pitch, dest, debugIndex);

  for(auto i = 0u; i < sourceMarks.size() - 1; i++) {
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

  //WaveDataTemp tmp(WaveData::copy(dest));

  while(destOffset <= destOffsetBound) {
    auto periodSamples = dest.toSamples(1 / pitch.at(destOffset));

    auto copyPeriodRight = dest.toDuration(sourcePeriodSamplesRight);
    auto copyPeriodLeft = dest.toDuration(sourcePeriodSamplesLeft);
    overlapAddAroundMark(source, mark, dest, destOffset, copyPeriodLeft, copyPeriodRight);
    /*overlapAddAroundMark(source, mark, tmp, destOffset, copyPeriodLeft, copyPeriodRight);
    for(auto i = 0u; i < tmp.size(); i++)
    assert(tmp[i] == dest[i]);*/

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
