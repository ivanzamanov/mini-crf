#include<exception>
#include<algorithm>
#include<valarray>

#include"speech_mod.hpp"
#include"util.hpp"
#include"fourier.hpp"
#include"libmfcc.hpp"

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

void scaleToPitchAndDurationSimpleTD(SpeechWaveData dest,
                                     SpeechWaveData source,
                                     PitchRange pitch,
                                     int debugIndex);
void copyVoicedPartTD(SpeechWaveData source,
                      int& destOffset,
                      const int destOffsetBound,
                      int mark,
                      int nMark,
                      PitchRange pitch,
                      SpeechWaveData dest,
                      int debugIndex);
void copyVoicelessPart(const SpeechWaveData& source,
                       int& destOffset,
                       const int destOffsetBound,
                       const int,
                       int sMark,
                       int sourceBound,
                       SpeechWaveData dest);
void scaleToPitchAndDuration(SpeechWaveData dest,
                             SpeechWaveData source,
                             PitchRange pitch,
                             int debugIndex);

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

static int readSourceData(SpeechWaveSynthesis& w, std::vector<SpeechWaveData>& destParts) {
  int result = 0, i = 0;
  // TODO: possibly avoid reading a file multiple times...
  for(auto& p : w.source) {
    FileData fileData = w.origin.file_data_of(p);
    Wave wav(fileData.file);

    result = wav.sampleRate();
    PsolaConstants limits(wav.sampleRate());

    SpeechWaveData part;
    // extract wave data
    part.init(wav, p.start, p.end);

    // copy pitch marks, translating to part-local sample
    for(auto mark : fileData.pitch_marks) {
      // Omit boundaries on purpose
      if(mark > p.start && mark < p.end)
        part.marks.push_back(wav.toSamples(mark) - wav.toSamples(p.start));
    }

    destParts[i] = part;
    i++;
  }
  return result;
}

Wave SpeechWaveSynthesis::get_resynthesis_td() {
  Options opts;
  opts.log = false;
  opts.add_opt("td", "");
  auto result = get_resynthesis(opts);
  return result;
}

Wave SpeechWaveSynthesis::get_concatenation(const Options& opts) {
  // First off, collect the WAV data for each source unit
  vector<SpeechWaveData> waveData(source.size());
  auto sampleRate = readSourceData(*this, waveData);

  // First off, prepare for output, build some default header...
  WaveHeader h = WaveHeader::default_header();
  h.sampleRate = sampleRate;
  h.byteRate = sampleRate * 2;

  WaveBuilder wb(h);
  for(auto wd : waveData)
    wb.append(wd);

  std::for_each(waveData.begin(), waveData.end(), WaveData::deallocate);

  return wb.build();
}

Wave SpeechWaveSynthesis::get_resynthesis(const Options& opts) {
  // First off, collect the WAV data for each source unit
  vector<SpeechWaveData> waveData(source.size());
  auto sampleRate = readSourceData(*this, waveData);

  // First off, prepare for output, build some default header...
  WaveHeader h = WaveHeader::default_header();
  h.sampleRate = sampleRate;
  h.byteRate = sampleRate * 2;

  WaveBuilder wb(h);
  // Ok, so waveData contains all the pieces with pitch marks translated to piece-local time
  {
    double completeDuration = 0;
    each(target, [&](const PhonemeInstance& p) { completeDuration += p.duration; });
    // preallocate the complete wave result,
    // but only temporarily
    WaveDataTemp result(WaveData::allocate(completeDuration, sampleRate));
    do_resynthesis(result, waveData, opts);
    wb.append(result);
  }

  std::for_each(waveData.begin(), waveData.end(), WaveData::deallocate);

  return wb.build();
}

template<class Frame>
void extractFrame(Frame& f, WaveData& src, int from) {
  for(auto i = 0u; i < f.size(); i++)
    f[i] = src[from + i];
}

template<class Frame, class Holder>
void computeMFCC(const Frame& f, Holder& h, int sampleRate) {
  double data[f.size()];
  for(auto i = 0u; i < f.size(); i++) data[i] = f[i].real();
  for(auto i = 0u; i < h.size(); i++)
    h[i] = GetCoefficient(data, sampleRate, 42, f.size(), i + 1);
}

template<class Frame>
double diffFrames(const Frame& f1, const Frame& f2, int sampleRate) {
  std::array<double, 5> mfcc1, mfcc2;
  computeMFCC(f1, mfcc1, sampleRate);
  computeMFCC(f2, mfcc2, sampleRate);
  double result = 0;
  for(auto i = 0u; i < mfcc1.size(); i++)
    result += std::abs(mfcc1[i] - mfcc2[i]);
  //INFO("diff " << result);
  return result;
}

template<class Frame>
double diffFramesAtOffset(SpeechWaveData& p, SpeechWaveData& n, int offset,
                          Frame& pFrame, Frame& nFrame) {
  // Assume bound-checked
  extractFrame(pFrame, p, p.length + offset);
  extractFrame(nFrame, n, - offset);
  return diffFrames(pFrame, nFrame, p.sampleRate);
}

int couplePieces(SpeechWaveData& p, SpeechWaveData& n) {
  constexpr auto FRAME = 128;
  constexpr auto STEP = 32;

  std::valarray<cdouble> pFrame(FRAME), nFrame(FRAME);

  auto minDiff = diffFramesAtOffset(p, n, 0, pFrame, nFrame);
  auto minOffset = 0;

  auto maxBoundaryOffset = std::min(p.extra.length - p.length,
                                    n.length / 2);
  auto minBoundaryOffset = std::max(-p.length / 2,
                                    n.extra.offset - n.offset);

  INFO("Trying to adjust between " << minBoundaryOffset << ", " << maxBoundaryOffset);
  for(auto currentBoundaryOffset = minBoundaryOffset;
      currentBoundaryOffset + STEP < maxBoundaryOffset;
      currentBoundaryOffset += STEP) {
    auto diffAtCurrent = diffFramesAtOffset(p, n, currentBoundaryOffset,
                                            pFrame, nFrame);
    if(diffAtCurrent < minDiff)
      minOffset = currentBoundaryOffset;
  }

  auto preDuration = p.duration() + n.duration();

  p.length += minOffset;
  n.offset += minOffset;
  n.length -= minOffset;

  auto postDuration = p.duration() + n.duration();

  assert(p.length > 0);
  assert(p.offset >= 0);
  assert(n.length > 0);
  assert(n.offset >= 0);
  // Due to double-rounding errors, it's possible that
  // they differ as time points, but not as samples...
  assert(p.toSamples(preDuration - postDuration) == 0);

  return minOffset;
}

void do_coupling(vector<SpeechWaveData>& pieces,
                 const Options&) {
  auto i = 1u;
  while(i < pieces.size()) {
    auto adjustment = couplePieces(pieces[i - 1], pieces[i]);
    INFO("Adjust " << i << " by " << adjustment);
    i++;
  }
}

Wave SpeechWaveSynthesis::get_coupling(const Options& opts) {
  const auto N = source.size();
  assert(N > 1);
  vector<SpeechWaveData> waveData(N);
  auto sampleRate = readSourceData(*this, waveData);

  do_coupling(waveData, opts);

  // Now simply transfer and cleanup...
  WaveHeader h = WaveHeader::default_header();
  h.sampleRate = sampleRate;
  h.byteRate = sampleRate * 2;

  WaveBuilder wb(h);
  for(auto& p : waveData)
    wb.append(p);
  std::for_each(waveData.begin(), waveData.end(), WaveData::deallocate);
  return wb.build();
}

template<bool flip=false>
int overlapAddAroundMark(SpeechWaveData source,
                         int sMark,
                         SpeechWaveData dest,
                         int dMark,
                         const int samplesLeft,
                         const int samplesRight,
                         bool win=true) {
  sMark += source.offset - source.extra.offset;
  source = source.extra;

  dMark += dest.offset - dest.extra.offset;
  dest = dest.extra;

  DEBUG(LOG("Mark " << dMark));

  // Will reuse window space...
  double window[2 * std::max(samplesLeft, samplesRight) + 1];

  // It's what they call...
  int destBot, destTop, sourceBot, sourceTop;
  // The Rise...
  gen_rise(window, samplesLeft, win);

  destBot = std::max(0, dMark - samplesLeft);
  destTop = std::min(dest.length, dMark);

  sourceBot = std::max(0, sMark - samplesLeft);
  sourceTop = std::min(sMark, source.length);

  for(int di = destBot,
        si = sourceBot,
        wi = 0; di < destTop && si < sourceTop; di++, si++, wi++)
    dest.plus(flip ? destTop - di : di, source[si] * window[wi]);

  // And Fall
  destBot = std::max(0, dMark);
  destTop = std::min(dest.length, dMark + samplesRight);

  sourceBot = std::max(sMark, 0);
  sourceTop = std::min(sMark + samplesRight, source.length);

  gen_fall(window, samplesRight, win);

  for(int di = destBot,
        si = sourceBot,
        wi = 0; di < destTop && si < sourceTop; di++, si++, wi++)
    dest.plus(flip ? destTop - di : di, source[si] * window[wi]);

  return std::min(destTop - destBot, sourceTop - sourceBot);
}

void copyVoicelessPart(const SpeechWaveData& source,
                       int& dMark,
                       const int destOffsetBound,
                       const int,
                       int sMark,
                       int sourceBound,
                       SpeechWaveData dest) {
  auto dLen = destOffsetBound - dMark;
  auto scale = (double) dLen / (sourceBound - sMark);
  PsolaConstants limits(dest.sampleRate);

  bool flip = false;
  while(dMark < destOffsetBound) {
    auto periodSamples = limits.maxVoicelessSamplesCopy;
    int copied;
    if(flip)
      copied = overlapAddAroundMark<true>(source, sMark, dest, dMark,
                                    periodSamples, periodSamples);
    else
      copied = overlapAddAroundMark<false>(source, sMark, dest, dMark,
                                  periodSamples, periodSamples);

    if(copied == 0)
      break;

    sMark += copied / scale;
    dMark += copied;
    flip = !flip;
  }
}

void coupleScaledPieces(vector<SpeechWaveData>& scaledPieces,
                        const vector<SpeechWaveData>& originalPieces,
                        const Options& opts) {
  for(auto i = 0u; i < scaledPieces.size(); i++) {
    auto& scaled = scaledPieces[i];
    auto& original = originalPieces[i];

    SpeechWaveData newScaled;
    newScaled = WaveData::allocate(scaled.duration() +
                                   (original.extra.duration() - original.duration()),
                                   scaled.sampleRate);
    newScaled.extra = newScaled;

    auto extraFrontBegin = original.extra.data + original.extra.offset;
    auto extraFrontEnd = original.extra.data + original.offset - original.extra.offset;

    newScaled.offset = extraFrontEnd - extraFrontBegin;
    newScaled.length = scaled.length;
    // idea - copy extra before, after, put scaled in the middle
    // Front...
    std::copy(extraFrontBegin,
              extraFrontEnd,
              newScaled.data);
    // middle, i.e. the scaled data
    std::copy(scaled.data + scaled.offset,
              scaled.data + scaled.offset + scaled.length,
              newScaled.data + newScaled.offset);
    // extra back...
    auto extraBackBegin = original.data + original.offset + original.length;
    auto extraBackEnd = original.extra.data + original.extra.offset + original.extra.length;
    std::copy(extraBackBegin,
              extraBackEnd,
              newScaled.data + newScaled.offset + newScaled.length);

    WaveData::deallocate(scaled);
    scaled = newScaled;
  }

  do_coupling(scaledPieces, opts);
}

void SpeechWaveSynthesis::do_resynthesis(WaveData dest,
                                         const vector<SpeechWaveData>& pieces,
                                         const Options& opts) {
  PitchRange pitchTier[target.size()];

  PitchTier pt = initPitchTier(pitchTier, target, dest);

  Progress prog(target.size(), "PSOLA: ");
  vector<SpeechWaveData> scaledPieces(target.size());

  for(auto i = 0u; i < target.size(); i++) {
    auto& p = pieces[i];
    auto& pitch = pt.ranges[i];

    scaledPieces[i] = SpeechWaveData::allocate(target[i].duration, dest.sampleRate);

    scaleToPitchAndDuration(scaledPieces[i], p, pitch, i);

    prog.update();
  }
  prog.finish();

  //coupleScaledPieces(scaledPieces, pieces, opts);

  // Now simply transfer and cleanup...
  auto destOffset = 0;
  for(const auto& p : scaledPieces) {
    for(auto i = 0; i < p.length && destOffset < dest.length; i++, destOffset++)
      dest[destOffset] = p[i];
  }
  std::for_each(scaledPieces.begin(), scaledPieces.end(), WaveData::deallocate);
}

vector<bool> fillMissingMarks(vector<int>& marks, PsolaConstants limits) {
  vector<int> filled;
  vector<bool> isVoicelessFlags;
  auto lastMark = 0;
  for(auto mark : marks) {
    while(mark - lastMark > limits.maxVoicelessSamples) {
      lastMark += limits.maxVoicelessSamples;
      filled.push_back(lastMark);
      isVoicelessFlags.push_back(true);
    }
    filled.push_back(lastMark = mark);
    isVoicelessFlags.push_back(false);
  }
  marks = filled;
  return isVoicelessFlags;
}

void scaleToPitchAndDuration(SpeechWaveData dest,
                             SpeechWaveData source,
                             PitchRange pitch,
                             int) {
  PsolaConstants limits(dest.sampleRate);

  // Time scale
  double scale = dest.duration() / source.duration();

  vector<int> sourceMarks(source.marks);
  sourceMarks.push_back(source.length);
  auto isVoicelessFlags = fillMissingMarks(sourceMarks, limits);

  auto sMarkIndex = 0u;
  auto dMark = sourceMarks[sMarkIndex];
  while(sMarkIndex < sourceMarks.size()) {
    auto sMark = sourceMarks[sMarkIndex];
    auto scaledSMark = (int) sMark * scale;
    bool voiceless = isVoicelessFlags[sMarkIndex];

    while(dMark < scaledSMark) {
      overlapAddAroundMark(source, sMark, dest, dMark,
                           limits.maxVoicelessSamples, limits.maxVoicelessSamples);
      dMark += voiceless
        ? limits.maxVoicelessSamples * scale
        : dest.toSamples(1 / pitch.at(dMark));
    }
    sMarkIndex++;
  }
}
