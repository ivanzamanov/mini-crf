#include<exception>
#include"speech_mod.hpp"
#include"util.hpp"

using namespace util;
using std::vector;

#pragma GCC push_options
#pragma GCC optimize ("O0")

// F0 of less than 50 Hz will be considered voiceless
const int MAX_VOICELESS_SAMPLES = WaveData::toSamples(0.02f);
const double MAX_VOICELESS_PERIOD = WaveData::toDuration(MAX_VOICELESS_SAMPLES);
const int MAX_VOICELESS_SAMPLES_COPY = WaveData::toSamples(0.01f);
const double MAX_VOICELESS_PERIOD_COPY = WaveData::toDuration(MAX_VOICELESS_SAMPLES_COPY);

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
    destPtr -> markFrequency = p.pitch_contour[0];

    // and move on
    ptr++;
    destPtr++;
    i++;
  }
}

double hann(int i, int size) {
  return 0.5 * (1 - cos(2 * M_PI * i / size));
}

void gen_fall(double* data, int size) {
  //transform(data, size, [&](int, double&) { return 1; });
  size = size * 2;
  transform(data, size / 2, [=](int i, double) {
      //return 1;
      i += size/2;
      return hann(i, size);
    });
}

void gen_rise(double* data, int size) {
  //transform(data, size, [&](int, double&) { return 1; });
  size = size * 2;
  transform(data, size / 2, [=](int i, double) {
      //return 1;
      return hann(i, size);
    });
}

struct PitchRange {
  void set(frequency left, frequency right, int offset, int length) {
    this->left = left;
    this->right = right;
    this->length = length;
    this->offset = offset;
  }

  frequency at(int index) const {
    double c = (index+1 - offset); // centered
    if(c < 0 || c > length)
      throw std::out_of_range("Pitch from range");
    return left * (1 - c / length) + right * c / length;
  }

  frequency at(double time) const { return at(WaveData::toSamples(time)); }
  frequency left;
  frequency right;
  int length;
  int offset;
};

struct PitchTier {
  PitchRange* ranges;
  int length;
  frequency at(int sample) const {
    for(int i = 0; i < length; i++)
      if( ranges[i].offset + ranges[i].length > sample)
        return ranges[i].at(sample);
    // Shouldn't happen so this will break stuff
    throw std::out_of_range("Out of range " + sample);
    /*int totalLen = ranges[length - 1].offset + ranges[length - 1].length;
    std::cerr << "Out of range: " << sample << "/" << totalLen << std::endl;
    return ranges[length - 1].at(ranges[length - 1].length - 1);*/
  }
};

PitchTier initPitchTier(PitchRange* tier, vector<PhonemeInstance> target) {
  unsigned i = 0;
  frequency left = std::exp(target[i].pitch_contour[0]);
  frequency right = std::exp(target[i].pitch_contour[1]);
  int offset = 0;
  double duration = target[i].duration;
  double totalDuration = duration;
  tier[i].set(left, right, offset, WaveData::toSamples(duration));

  for(i++; i < target.size(); i++) {
    offset = WaveData::toSamples(totalDuration);
    left = (std::exp(target[i].pitch_contour[0]) + tier[i-1].right) / 2;
    // Smooth out pitch at the concatenation points
    tier[i-1].right = left;
    right = std::exp(target[i].pitch_contour[1]);

    duration = target[i].end - target[i].start;
    totalDuration += duration;
    tier[i].set(left, right, offset, WaveData::toSamples(duration));
  }
  PitchTier result =  {
  ranges: tier,
  length: (int) target.size()
  };
  return result;
}

int overlapAddAroundMark(SpeechWaveData& source, const int currentMark,
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
  /*std::cerr << "(" << std::max(0, currentMark - samplesLeft) + source.offset
            << "," << std::min(currentMark + samplesRight, dest.length) + source.offset
            << ",dest=" << destOffset
            << ")" << std::endl;*/
}

void copyVoicedPart(SpeechWaveData& source, int& destOffset, const int destOffsetBound,
                    int mark, const int nMark, const double scale,
                    const PitchTier pitch, WaveData dest) {
  int sourcePeriodSamplesRight = nMark - mark;
  int sourcePeriodSamplesLeft = nMark - mark;
  int scaledMark = mark + sourcePeriodSamplesRight * scale;
  int sourceMark = mark;

  while(mark < scaledMark && destOffset <= destOffsetBound) {
    int periodSamples = WaveData::toSamples(1 / pitch.at(destOffset));

    double copyPeriodRight = WaveData::toDuration(sourcePeriodSamplesRight);
    double copyPeriodLeft = WaveData::toDuration(sourcePeriodSamplesLeft);
    overlapAddAroundMark(source, sourceMark, dest, destOffset, copyPeriodLeft, copyPeriodRight);

    mark += periodSamples;
    destOffset += periodSamples;
  }
}

float nextRandFloat() {
  static int cycleMax = 10;
  static int cycle = 0;
  cycle = (cycle + 1) % cycleMax + 1;

  return 0.5 + (float) cycle / cycleMax;
}

void copyVoicelessPart(SpeechWaveData& source, int& destOffset, const int destOffsetBound,
                       const int voicelessStart, const int voicelessEnd,
                       const double scale, WaveData dest) {
  int mark = voicelessStart;
  int sourceMark = mark + MAX_VOICELESS_SAMPLES_COPY;
  const int scaledVoicelessEnd = voicelessStart + (voicelessEnd - voicelessStart) * scale;
  while(mark < scaledVoicelessEnd && destOffset <= destOffsetBound) {
    int periodSamples = MAX_VOICELESS_SAMPLES_COPY;
    const double period = WaveData::toDuration(periodSamples);
    overlapAddAroundMark(source, sourceMark, dest, destOffset, period, period);

    periodSamples = MAX_VOICELESS_SAMPLES_COPY * nextRandFloat();
    mark += periodSamples;
    destOffset += periodSamples;
  }
}

void scaleToPitchAndDurationSimple(WaveData dest, int startOffset,
                                   SpeechWaveData& source, PitchTier pitch, double duration) {
  // I'd rather have it rounded up
  int count = duration / WaveData::toDuration(source.length);

  int destOffset = startOffset;
  for(int i = 0; i < count && destOffset < dest.length; i++) {
    destOffset = startOffset + i * WaveData::toSamples(1 / pitch.at(destOffset));
    for(int j = 0; j < source.length && destOffset < dest.length; j++, destOffset++)
      dest[destOffset] = source[j];
  }
}

void scaleToPitchAndDuration(WaveData dest, int destOffset,
                             SpeechWaveData& source, PitchTier pitch, double duration) {
  // Time scale
  double scale = duration / WaveData::toDuration(source.length);
  //std::cerr << "Scale: " << scale << " duration: " << duration << std::endl;
  // Copy of the source marks, but with additional at 0 and length
  vector<int> sourceMarks;
  sourceMarks.push_back(0);
  each(source.marks, [&](int v) { sourceMarks.push_back(v); });
  sourceMarks.push_back(source.length);

  vector<int> scaledMarks;
  each(sourceMarks, [&](int v) { scaledMarks.push_back(v * scale); });

  int startOffset = destOffset;
  int destOffsetBound;

  for(unsigned i = 0; i < sourceMarks.size() - 1; i++) {
    int mark = sourceMarks[i];
    int nMark = sourceMarks[i + 1];
    int scaledEnd = scale * nMark;
    destOffsetBound = startOffset + scaledEnd;
    if(nMark - mark >= MAX_VOICELESS_SAMPLES_COPY) {
      copyVoicelessPart(source, destOffset, destOffsetBound, mark, nMark, scale, dest);
    } else {
      destOffsetBound = startOffset + scaledEnd;
      copyVoicedPart(source, destOffset, destOffsetBound, mark, nMark, scale, pitch, dest);
    }
  }
  //*destOffset = startOffset + WaveData::toSamples(duration);
  /*std::cerr << "end at time: " << WaveData::toDuration(*destOffset)
            << ", copied time " << WaveData::toDuration(*destOffset - startOffset)
            << " expected time " << duration
            << std::endl;*/
}

void smooth(WaveData dest, int offset, double pitch) {
  int samples = WaveData::toSamples(1 / pitch);
  int low = offset - samples;
  int high = offset + samples;

  //std::cerr << WaveData::toDuration(low) << " " << WaveData::toDuration(high) << std::endl;

  int hSize = 2 * (high - low);
  for(int destOffset = low; destOffset <= high; destOffset++) {
    int left = dest[destOffset - samples] * hann(hSize / 2 + destOffset - low, hSize);
    int right = dest[destOffset + samples] * hann(destOffset - low, hSize);
    dest[destOffset] = left + right;
  }
}

void SpeechWaveSynthesis::do_resynthesis(WaveData dest, SpeechWaveData* pieces) {
  PitchRange pitchTier[target.size()];

  PitchTier pt = initPitchTier(pitchTier, target);

  double totalDuration = 0;
  Progress prog(target.size() - 2, "PSOLA: ");
  for(unsigned i = 0; i < target.size(); i++) {
    SpeechWaveData& p = pieces[i];
    PhonemeInstance& tgt = target[i];
    double targetDuration = tgt.end - tgt.start;

    int startOffset = WaveData::toSamples(totalDuration);
    if(p.marks.size() == 0)
      scaleToPitchAndDurationSimple(dest, startOffset, p, pt, targetDuration);
    else
      scaleToPitchAndDuration(dest, startOffset, p, pt, targetDuration);

    totalDuration += targetDuration;

    prog.update();
  }
  prog.finish();
  return;
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
  double completeDuration = 0;
  each(target, [&](PhonemeInstance& p) { completeDuration += p.duration; });
  // preallocate the complete wave result
  WaveData result = WaveData::allocate(completeDuration);
  result.print(0,0);
  do_resynthesis(result, waveData);

  wb.append(result);
  return wb.build();
}

#pragma GCC pop_options
