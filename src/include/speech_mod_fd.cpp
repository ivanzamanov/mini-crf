#include<exception>
#include"speech_mod.hpp"
#include"util.hpp"
#include"fourier.hpp"

using namespace util;
using std::vector;

static int overlapAddAroundMark(SpeechWaveData& source, const int currentMark,
                         WaveData dest, const int destOffset,
                         const double periodLeft, const double periodRight) {
  DEBUG(std::cerr << "Mark " << destOffset << std::endl);
  int samplesLeft = WaveData::toSamples(periodLeft);
  int samplesRight = WaveData::toSamples(periodRight);
  // Will reuse window space...
  double window[2 * std::max(samplesLeft, samplesRight) + 1];

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
    dest.plus(di, source[si] * window[wi]);

  // And Fall
  destBot = std::max(0, destOffset);
  destTop = std::min(dest.length, destOffset + samplesRight);
  sourceBot = std::max(currentMark, 0);
  sourceTop = std::min(currentMark + samplesRight, source.length);
  gen_fall(window, samplesRight);

  for(int di = destBot,
        si = sourceBot,
        wi = 0; di < destTop && si < sourceTop; di++, si++, wi++)
    dest.plus(di, source[si] * window[wi]);

  return 0;
}

const int I_NF = 500;
static void copyVoicedPart(SpeechWaveData& oSource,
                    int& destOffset,
                    const int destOffsetBound,
                    int mark,
                    const int nMark,
                    const PitchTier pitch,
                    WaveData dest) {
  int NF = std::min(I_NF, nMark - mark);
  cdouble frequencies[NF];
  cdouble nFrequencies[NF];

  double destPitch = pitch.at(destOffset);
  double sourcePitch = 1 / WaveData::toDuration(nMark - mark);
  double pitchScale = destPitch / sourcePitch;
  int sourceLen = nMark - mark;
  int newLen = sourceLen / pitchScale;
  double values[std::max(sourceLen, newLen)];
  double nValues[newLen];
  for(int i = 0; i < sourceLen; i++) values[i] = (double) oSource[mark + i];

  ft::FT(values, sourceLen, frequencies, NF);
  for(int i = 0; i < NF; i++) {
    int sIndex = (int) (std::round(i / pitchScale)) % NF;
    nFrequencies[i] = frequencies[sIndex];
  }
  for(int i = 0; i < NF; i++) if(!(frequencies[i] == nFrequencies[i])) std::cerr << "Diff at " << i << std::endl;
  ft::rFT(nFrequencies, NF, nValues, newLen);
  double err = 0;
  for(int i = 0; i < std::min(newLen, sourceLen); i++)
    err += std::abs(nValues[i] - values[i]);
  std::cerr << "Error: " << err << std::endl;

  while(destOffset < dest.length && destOffset < destOffsetBound) {
    for(int i = 0; i < newLen && destOffset < dest.length; i++, destOffset++)
      dest[destOffset] = values[i];
  }
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
                       const int sourceOffset,
                       int sMark,
                       int sourceBound,
                       WaveData dest) {
  int dLen = destOffsetBound - destOffset;
  int sLen = sourceBound - sourceOffset;
  float scale = (float) dLen / (sLen - sMark);

  while(destOffset <= destOffsetBound) {
    int periodSamples = MAX_VOICELESS_SAMPLES_COPY * nextRandFloat();
    const double period = WaveData::toDuration(periodSamples);
    overlapAddAroundMark(source, sMark, dest, destOffset, period, period);

    sMark += periodSamples / scale;
    destOffset += periodSamples;
  }
}

static void scaleToPitchAndDurationSimple(WaveData dest, int startOffset,
                                   SpeechWaveData& source, PitchTier pitch, double duration) {
  // I'd rather have it rounded up
  int count = duration / WaveData::toDuration(source.length);

  int destOffset = startOffset;
  for(int i = 0; i < count && destOffset < dest.length; i++) {
    destOffset = startOffset + i * WaveData::toSamples(1 / pitch.at(destOffset));
    for(int j = 0; j < source.length && destOffset < dest.length; j++, destOffset++)
      dest.plus(destOffset, source[j]);
  }
}

static void scaleToPitchAndDuration(WaveData dest, int destOffset,
                             SpeechWaveData& source, PitchTier pitch, double duration) {
  // Time scale
  double scale = duration / WaveData::toDuration(source.length);

  vector<int> sourceMarks;
  each(source.marks, [&](int v) { sourceMarks.push_back(v); });
  sourceMarks.push_back(source.length);

  vector<int> scaledMarks;
  each(sourceMarks, [&](int v) { scaledMarks.push_back(v * scale); });

  int startOffset = destOffset;
  int destOffsetBound;
  
  int i = 0;
  int mark = 0;
  int nMark = sourceMarks[i];
  destOffsetBound = startOffset + nMark * scale;
  if(nMark - mark >= MAX_VOICELESS_SAMPLES)
    copyVoicelessPart(source, destOffset, destOffsetBound, 0, mark, nMark, dest);
  else
    copyVoicedPart(source, destOffset, destOffsetBound, nMark, nMark + (nMark - mark), pitch, dest);

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

void smooth(WaveData dest, int offset, double pitch) {
  int samples = WaveData::toSamples(1 / pitch);
  int low = offset - samples;
  int high = offset + samples;

  int hSize = 2 * (high - low);
  for(int destOffset = low; destOffset <= high; destOffset++) {
    int left = dest[destOffset - samples] * hann(hSize / 2 + destOffset - low, hSize);
    int right = dest[destOffset + samples] * hann(destOffset - low, hSize);
    dest[destOffset] = left + right;
  }
}

void SpeechWaveSynthesis::do_resynthesis_fd(WaveData dest, SpeechWaveData* pieces) {
  PitchRange pitchTier[target.size()];

  PitchTier pt = initPitchTier(pitchTier, target);

  double totalDuration = 0;
  Progress prog(target.size() - 2, "PSOLA: ");
  for(unsigned i = 0; i < target.size(); i++) {
    SpeechWaveData& p = pieces[i];
    PhonemeInstance& tgt = target[i];
    double targetDuration = tgt.end - tgt.start;

    int startOffset = WaveData::toSamples(totalDuration);
    if(WaveData::toDuration(p.length) <= 1 / pt.at(WaveData::toSamples(totalDuration)))
      scaleToPitchAndDurationSimple(dest, startOffset, p, pt, targetDuration);
    else
      scaleToPitchAndDuration(dest, startOffset, p, pt, targetDuration);

    totalDuration += targetDuration;

    prog.update();
  }
  prog.finish();
  //return;
  prog = Progress(target.size(), "Smoothing: ");
  totalDuration = target[0].duration;
  for(unsigned i = 1; i < target.size() - 1; i++) {
    int offset = WaveData::toSamples(totalDuration);
    smooth(dest, offset, pt.at(offset));

    PhonemeInstance& tgt = target[i];
    double targetDuration = tgt.end - tgt.start;
    totalDuration += targetDuration;
    prog.update();
  }
  prog.finish();
}
