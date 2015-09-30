#include<exception>
#include<climits>
#include"speech_mod.hpp"
#include"util.hpp"
#include"fourier.hpp"

#pragma GCC push_options
#pragma GCC optimize ("O0")

using namespace util;
using std::vector;

static int overlapAddAroundMark(SpeechWaveData& source, const int currentMark,
                                WaveData dest, const int destOffset,
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

short truncate(double v) {
  v = std::max(v, (double) SHRT_MIN);
  v = std::min(v, (double) SHRT_MAX);
  return (short) v;
}

const int I_NF = 500;
static void copyVoicedPart(SpeechWaveData& oSource,
                           int& destOffset,
                           const int destOffsetBound,
                           int mark,
                           const int nMark,
                           frequency pitch,
                           WaveData dest) {
  double destPitch = pitch;
  double sourcePitch = 1 / WaveData::toDuration(nMark - mark);
  double pitchScale = destPitch / sourcePitch;

  int NF = nMark - mark;
  NF /= 2;
  cdouble frequencies[NF + 1];

  if(std::abs(pitchScale - 1) < 0.1)
    pitchScale = 1;
  int sourceLen = nMark - mark;
  int newPeriod = sourceLen / pitchScale;
  double values[sourceLen];
  
  for(int i = 0; i < sourceLen; i++) values[i] = (double) oSource[mark + i];

  if(pitchScale != 1) {
    ft::FT(values, sourceLen, frequencies, NF);
    ft::rFT(frequencies, NF, values, sourceLen, newPeriod);
  }

  while(destOffset < dest.length && destOffset < destOffsetBound) {
    for(int i = 0; i < sourceLen && destOffset < dest.length; i++, destOffset++)
      dest.plus(destOffset, truncate(values[i]));
  }
  //destOffset -= newLen / OVERLAP;
}

static float nextRandFloat() {
  static int cycleMax = 4;
  static int cycle = 0;
  cycle = cycle % cycleMax + 1;

  return 0.8 + (float) cycle * 0.1;
}

static void copyVoicelessPart(SpeechWaveData& source,
                              int& destOffset,
                              const int destOffsetBound,
                              const int,
                              int sMark,
                              int sourceBound,
                              WaveData dest) {
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
  return;
}

static void scaleToPitchAndDurationSimple(WaveData dest, int& startOffset,
                                          SpeechWaveData& source, frequency pitch, double) {
  copyVoicedPart(source, startOffset, dest.length, 0, dest.length, pitch, dest);
}

static void scaleToPitchAndDuration(WaveData dest,
                                    int& destOffset,
                                    SpeechWaveData& source, frequency pitch, double duration) {
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
  if(nMark - mark >= MAX_VOICELESS_SAMPLES)
    copyVoicelessPart(source, destOffset, destOffsetBound, 0, mark, nMark, dest);
  else
    copyVoicedPart(source, destOffset, destOffsetBound, mark, nMark, pitch, dest);

  for(unsigned i = 0; i < sourceMarks.size() - 1; i++) {
    mark = sourceMarks[i];
    nMark = sourceMarks[i + 1];

    int scaledEnd = scale * nMark;
    destOffsetBound = startOffset + scaledEnd;

    if(nMark - mark >= MAX_VOICELESS_SAMPLES)
      copyVoicelessPart(source, destOffset, destOffsetBound, mark, mark, nMark, dest);
    else
      copyVoicedPart(source, destOffset, destOffsetBound, mark, nMark, pitch, dest);
  }

  if(sourceMarks.size() == 1)
    return;
  i = sourceMarks.size() - 2;
  mark = sourceMarks[i];
  nMark = source.length;
  destOffsetBound = startOffset + source.length * scale;
  copyVoicelessPart(source, destOffset, destOffsetBound, mark, nMark, scale, dest);
}

void smooth(WaveData dest, int offset, double pitch,
            PhonemeInstance& left, PhonemeInstance& right) {
  //return;
  int SMOOTHING_COUNT = 2;
  SMOOTHING_COUNT = std::min(left.duration / 2 - SMOOTHING_COUNT / pitch,
                             right.duration / 2 - SMOOTHING_COUNT / pitch);

  if(SMOOTHING_COUNT == 0)
    return;
  int samples = WaveData::toSamples(1 / pitch) * SMOOTHING_COUNT;
  int low = offset - samples;
  int high = offset + samples;

  for(int destOffset = low; destOffset <= high; destOffset++) {
    double t = (destOffset - low) / (high - low);
    int left = dest[destOffset - samples];
    int right = dest[destOffset + samples];
    dest[destOffset] = truncate(left * (1 - t) + right * t);
  }
}

void SpeechWaveSynthesis::do_resynthesis_fd(WaveData dest, SpeechWaveData* pieces) {
  PitchRange pitchTier[target.size()];

  PitchTier pt = initPitchTier(pitchTier, target);

  double totalDuration = 0;
  Progress prog(target.size() - 2, "PSOLA: ");
  int destOffset = 0;
  for(unsigned i = 0; i < target.size(); i++) {
    SpeechWaveData& p = pieces[i];
    PhonemeInstance& tgt = target[i];
    frequency pitch = pt.at(destOffset);

    int extraTime = 2 / pitch;
    double targetDuration = tgt.end - tgt.start + extraTime;
    int extra = WaveData::toSamples(extraTime);

    WaveDataTemp tmp(WaveData::allocate(targetDuration));

    //int startOffsetGuide = WaveData::toSamples(totalDuration);
    int startOffset = 0;
    if(WaveData::toDuration(p.length) <= 1 / pitch) {
      scaleToPitchAndDurationSimple(tmp, startOffset, p, pitch, targetDuration);
      LOG("Simple at " << i);
    }
    else
      scaleToPitchAndDuration(tmp, startOffset, p, pitch, targetDuration);

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

#pragma GCC pop_options
