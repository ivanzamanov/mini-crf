#include<exception>
#include<algorithm>
#include<valarray>

#include"speech_mod.hpp"
#include"util.hpp"
#include"fourier.hpp"
#include"comparisons.hpp"

using namespace util;
using std::vector;

bool SMOOTH = false;
bool SCALE_ENERGY = false;

// Mod commons
double hann(double i, int size) {
  return 0.5 - 0.5 * cos(2.0 * M_PI * i / (size - 1));
  //return 0.5 * (1 - cos(2 * M_PI * i / size));
}

void gen_fall(double* data, int size, bool window=true) {
  size++;
  //transform(data, size, [&](int, double&) { return 1; });
  size = size * 2;
  transform(data, size / 2, [=](int i, double) {
      i += size/2;
      return window ? hann(i, size) : 1;
    });
}

void gen_rise(double* data, int size, bool window=true) {
  size++;
  //transform(data, size, [&](int, double&) { return 1; });
  size = size * 2;
  transform(data, size / 2, [=](int i, double) {
      return window ? hann(i, size) : 1;
    });
}

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
int scaleToPitchAndDuration(SpeechWaveData dest,
                            SpeechWaveData source,
                            PitchRange pitch,
                            int lastMark,
                            int debugIndex);

PitchTier initPitchTier(PitchRange* tier, vector<PhonemeInstance> target, const WaveData& dest) {
  unsigned i = 0;
  auto left = std::exp(target[i].pitch_contour[0]);
  auto right = std::exp(target[i].pitch_contour[1]);
  tier[i].set(left, right, 0, dest.toSamples(target[i].duration));

  for(i++; i < target.size(); i++) {
    auto left = tier[i-1].right;
    //left = std::exp(target[i].pitch_contour[0]);
    // Smooth out pitch at the concatenation points
    //tier[i-1].right = left;
    auto right = std::exp(target[i].pitch_contour[1]);

    tier[i].set(left, right, 0, dest.toSamples(target[i].duration));
  }
  return {
    .ranges = tier,
      .length = (int) target.size()
      };
}

static vector<stime_t> findPitchMarks(const vector<stime_t>& marks,
                                      stime_t start, stime_t end) {
  vector<stime_t> result;
  for(auto mark : marks) {
    // Omit boundaries on purpose
    if(mark > start && mark < end)
      result.push_back(mark - start);
  }
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
    auto marks = findPitchMarks(fileData.pitch_marks, p.start, p.end);
    for(auto mark : marks)
      part.marks.push_back(wav.toSamples(mark));

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

Wave SpeechWaveSynthesis::get_concatenation(const Options&) {
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

void do_coupling(vector<SpeechWaveData>& pieces) {
  auto i = 1u;
  while(i < pieces.size()) {
    auto adjustment = couplePieces(pieces[i - 1], pieces[i]);
    INFO("Adjust " << i << " by " << adjustment);
    i++;
  }
}

Wave SpeechWaveSynthesis::get_coupling(const Options&) {
  const auto N = source.size();
  assert(N > 1);
  vector<SpeechWaveData> waveData(N);
  auto sampleRate = readSourceData(*this, waveData);

  do_coupling(waveData);

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
int overlapAddAroundMark(SpeechWaveData& src,
                         int sMark,
                         SpeechWaveData& dst,
                         int dMark,
                         const int samplesLeft,
                         const int samplesRight,
                         bool win=true) {
  sMark += src.offset - src.extra.offset;
  auto source = src.extra;

  dMark += dst.offset - dst.extra.offset;
  auto dest = dst.extra;

  DEBUG(LOG("Mark " << dMark));

  // Will reuse window space...
  double window[std::max(samplesLeft, samplesRight) + 1];

  // It's what they call...
  int destBot, destTop, sourceBot, sourceTop;
  // The Rise...
  gen_rise(window, samplesLeft, win);

  destBot = std::max(0, dMark - samplesLeft);
  destTop = std::min(dest.length, dMark);

  sourceBot = std::max(0, sMark - samplesLeft);
  sourceTop = std::min(sMark, source.length);

  for(auto di = destBot, si = sourceBot, wi = 0;
      di < destTop && si < sourceTop;
      di++, si++, wi++)
    dest.plus<double>(flip ? destTop - di : di, source[si] * window[wi]);

  // And Fall
  destBot = std::max(0, dMark);
  destTop = std::min(dest.length, dMark + samplesRight);

  sourceBot = std::max(sMark, 0);
  sourceTop = std::min(sMark + samplesRight, source.length);

  gen_fall(window, samplesRight, win);

  for(auto di = destBot, si = sourceBot, wi = 0;
      di < destTop && si < sourceTop;
      di++, si++, wi++)
    dest.plus<double>(flip ? destTop - di : di, source[si] * window[wi]);

  return std::min(destTop - destBot, sourceTop - sourceBot);
}

void coupleScaledPieces(vector<SpeechWaveData>& scaledPieces,
                        const vector<SpeechWaveData>& originalPieces) {
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

  do_coupling(scaledPieces);
}

void SpeechWaveSynthesis::do_resynthesis(WaveData dest,
                                         const vector<SpeechWaveData>& pieces,
                                         const Options&) {
  PitchRange pitchTier[target.size()];

  PitchTier pt = initPitchTier(pitchTier, target, dest);

  Progress prog(target.size(), "PSOLA: ");
  vector<SpeechWaveData> scaledPieces(target.size());

  auto lastMark = 0;
  for(auto i = 0u; i < target.size(); i++) {
    auto& p = pieces[i];
    auto& pitch = pt.ranges[i];

    scaledPieces[i] = SpeechWaveData::allocate(target[i].duration, dest.sampleRate);
    PRINT_SCALE(i << ": duration = " << p.duration() / scaledPieces[i].duration());

    lastMark = scaleToPitchAndDuration(scaledPieces[i], p, pitch, lastMark, i);

    //INFO("last mark: " << scaledPieces[i].toDuration(lastMark));
    lastMark -= scaledPieces[i].length;
    assert(lastMark >= 0);

    prog.update();
  }
  prog.finish();

  //coupleScaledPieces(scaledPieces, pieces);

  // Now simply transfer and cleanup...
  auto destOffset = 0;
  int j = 0;
  for(const auto& p : scaledPieces) {
    auto extraOffset = p.offset - p.extra.offset;
    if(destOffset >= extraOffset) {
      destOffset -= extraOffset;
      for(auto i = 0; i < extraOffset; i++, destOffset++)
        dest.plus<double>(destOffset, p.extra[i]);
    }

    for(auto i = 0; i < p.length && destOffset < dest.length; i++, destOffset++)
      dest.plus<double>(destOffset, p[i]);

    //SpeechWaveData::toFile(dest, "/tmp/dest-p-" + std::to_string(j) + ".wav");
    auto extraLength = (p.extra.length - p.length) / 2;
    for(auto i = 0; i < extraLength && destOffset + i < dest.length; i++)
      dest.plus<double>(destOffset + i, p.extra[p.extra.length - extraLength + i]);

    /*SpeechWaveData::toFile(pieces[j], "/tmp/original-" + std::to_string(j) + ".wav");
    SpeechWaveData::toFile(dest, "/tmp/dest-" + std::to_string(j) + ".wav");
    SpeechWaveData::toFile(p, "/tmp/" + std::to_string(j) + ".wav");*/
    j++;
  }
  std::for_each(scaledPieces.begin(), scaledPieces.end(), WaveData::deallocate);
}

vector<bool> fillMissingMarks(vector<int>& marks, PsolaConstants limits) {
  vector<int> filled;
  vector<bool> isVoicelessFlags;
  auto lastMark = 0;
  for(auto mark : marks) {
    if(mark - lastMark > limits.maxVoicelessSamples) {
      auto period = limits.voicelessSamplesCopy;
      while(mark - lastMark > period) {
        lastMark += period;
        filled.push_back(lastMark);
        isVoicelessFlags.push_back(true);
      }
    }

    filled.push_back(lastMark = mark);
    isVoicelessFlags.push_back(false);
  }
  marks = filled;
  return isVoicelessFlags;
}

static int getPeriodOfMark(int markIndex, vector<int>& sourceMarks) {
  auto mark = sourceMarks[markIndex];
  if(mark == 0)
    return sourceMarks[markIndex + 1];
  if(markIndex == (int) sourceMarks.size() - 1)
    return mark - sourceMarks[markIndex - 1];
  return std::min(mark - sourceMarks[markIndex - 1], sourceMarks[markIndex + 1] - mark);
}

int scaleToPitchAndDuration(SpeechWaveData dest,
                            SpeechWaveData source,
                            PitchRange pitch,
                            int firstMark,
                            int debugIndex) {
  PsolaConstants limits(dest.sampleRate);

  // Time scale
  double scale = dest.duration() / source.duration();

  vector<int> sourceMarks(source.marks);
  sourceMarks.insert(sourceMarks.begin(), 0);
  sourceMarks.push_back(source.length);
  auto isVoicelessFlags = fillMissingMarks(sourceMarks, limits);

  auto sMarkIndex = 0u;
  auto dMark = firstMark;
  /*INFO("duration: " << source.duration() << " -> " << dest.duration());
    INFO("first mark: " << dest.toDuration(firstMark));*/

  auto getPitchPeriod = [&](const PitchRange& pitch, int dMark, int sourcePeriod) {
    auto p = pitch.at(dMark);
    if(p == -1)
      return sourcePeriod;
    return dest.toSamples(1 / p);
  };

  //static int X = 0;
  while (sMarkIndex < sourceMarks.size() - 1) {
    auto sMark = sourceMarks[sMarkIndex];
    auto scaledSMark = sourceMarks[sMarkIndex + 1] * scale;

    bool voicelessSource = isVoicelessFlags[sMarkIndex];

    while(dMark < scaledSMark) {
      int sourcePeriod = voicelessSource
        ? limits.voicelessSamplesCopy
        : getPeriodOfMark(sMarkIndex, sourceMarks);

      auto destPeriod = voicelessSource
        ? limits.voicelessSamplesCopy
        : getPitchPeriod(pitch, dMark, sourcePeriod);

      overlapAddAroundMark(source, sMark, dest, dMark,
                           sourcePeriod, sourcePeriod);
      PRINT_SCALE(debugIndex << ": " << (double) sourcePeriod / destPeriod);
      /*SpeechWaveData::toFile(dest, "/tmp/dest-mark-" + std::to_string(X++) + ".wav");
      INFO(source.toDuration(sMark) << " -> " << dest.toDuration(dMark) << " " << voiceless);
      INFO(source.toDuration(sourcePeriod) << " " << dest.toDuration(destPeriod));*/

      dMark += destPeriod;
      assert(destPeriod > 0);
    }
    sMarkIndex++;
  }
  //INFO(debugIndex << ": last mark " << dest.toDuration(dMark - dest.length) << " period: " << dest.toDuration(destPeriod));
  return dMark;
}
