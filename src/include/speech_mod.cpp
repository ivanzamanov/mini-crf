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

int totalMarksBalance = 0;

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
    // copy pitch marks
    each(fileData.pitch_marks,
         [&](float& mark) {
           if(mark >= p.start && mark < p.end) {
             destPtr -> marks.push_back(ptr -> at_time(mark) - ptr -> at_time(p.start));
             totalMarksBalance--;
           }
         });
    // and move on
    ptr++;
    destPtr++;
  }
}

void gen_window(float* data, int size) {
  transform(data, size, [&](int i, float&) { return 0.5 * (1 - cos(2 * M_PI * i / size)); });
}

typedef SpeechWaveData STSignal;
vector<STSignal> extractSTSignals(const SpeechWaveData& swd) {
  vector<STSignal> result;
  unsigned i = 1;
  for(; i < swd.marks.size() - 1; i++) {
    int start = swd.marks[i - 1],
      mark = swd.marks[i],
      end = swd.marks[i + 1];
    int length = std::min(end - mark, mark - start);

    int stLength = 2 * length;
    WaveData d(swd.data, mark - length, stLength);
    d = WaveData::copy(d);
    float window[stLength]; gen_window(window, stLength);
    transform(d.data, stLength, [&](int i, short& v) { return v * window[i]; });
    STSignal sig(d);
    sig.marks.push_back(length);

    result.push_back(sig);
  }

  // Edge case of having too little pitch marks... bad case
  // but we should still be ok with a single piece
  if(swd.marks.size() <= 2) {
    int stLength = swd.length;
    WaveData d(swd.data, swd.marks[0], stLength);
    d = WaveData::copy(d);
    float window[stLength]; gen_window(window, stLength);
    transform(d.data, stLength, [&](int i, short& v) { return v * window[i]; });
    STSignal sig(d);
    sig.marks.push_back(0);
    result.push_back(sig);
  }
  return result;
}

struct PitchRange {
  void set(frequency left, frequency right, int length) {
    this->left = left;
    this->right = right;
    this->length = length;
  }

  frequency at(int index) { return left * (1 - (index+1) / length) + right * ((index+1)/length); }
  frequency at(float time) { return at(WaveData::toSamples(time)); }
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
                    WaveData dest, int destOffset, float localPeriod) {
  int sampleCount = WaveData::toSamples(localPeriod);
  float window[sampleCount];
  gen_window(window, sampleCount);

  int bot = std::max(0, currentMark - (sampleCount / 2));
  int top = std::min(currentMark + (sampleCount / 2), dest.length);

  for(int i = bot, j = 0; i < top; i++, j++)
    dest[destOffset + i] += source[j];

  ++totalMarksBalance;
}

void scaleToPitchAndDuration(WaveData dest, int destOffset,
                             SpeechWaveData& source, PitchRange pitch, float duration) {
  float scale = duration / WaveData::toDuration(source.length);
  // Marks in the new signal
  vector<int> targetMarks;
  // Marks in the source signal scaled to the time of the target signal
  // This will be used to determine how many times to copy a source period
  vector<int> scaledMarks;
  each(source.marks, [&](float mark) { scaledMarks.push_back(mark * scale); });

  if(source.marks.size() > 0) {
    // If any, this means there's a voiced part in the source that we need to scale
    targetMarks.push_back(source.marks[0] * scale);
    int lastMark = source.marks[source.marks.size() - 1] * scale;
    int mark = targetMarks[0];
    while(mark <= lastMark) {
      targetMarks.push_back(mark);
      frequency localPitch = pitch.at(mark);
      mark += WaveData::toSamples(1 / localPitch);
    }
  } else
    targetMarks.push_back(duration + 1/pitch.at(WaveData::toSamples(duration)));

  int copiedMarks = 0;
  for(unsigned index = 0; index < source.marks.size(); index++) {
    int currentMark = source.marks[index];
    int scaledMark = scaledMarks[index];
    while(currentMark <= scaledMark) {
      float period = 1 / (WaveData::toDuration(currentMark) + pitch.at(currentMark));
      overlapAddAroundMark(source, currentMark, dest, destOffset, period);
      
      destOffset += WaveData::toSamples(period);
      currentMark += WaveData::toSamples(period);
      copiedMarks++;
    }
  }

  /*std::cerr << "Copied: " << copiedMarks << std::endl
    << "Source: " << source.marks.size() << std::endl;*/
}

void SpeechWaveSynthesis::do_resynthesis(WaveData dest, SpeechWaveData* pieces) {
  PitchRange pitchTier[target.size()];
  initPitchTier(pitchTier, target);

  int offset = 0;
  for(unsigned i = 0; i < target.size(); i++) {
    SpeechWaveData& p = pieces[i];
    PhonemeInstance& tgt = target[i];
    scaleToPitchAndDuration(dest, offset, p, pitchTier[i], tgt.duration);
    offset += WaveData::toSamples(tgt.duration);
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
  // Ok, so waveData contains all the pieces with pitch marks translated to piece-local time
  float completeDuration = 0;
  each(target, [&](PhonemeInstance& p) { completeDuration += p.duration; });
  // preallocate the complete wave result
  WaveData result = WaveData::allocate(completeDuration);
  std::cerr << "Expected marks: " << totalMarksBalance << std::endl;
  do_resynthesis(result, waveData);
  std::cerr << "Marks balance: " << totalMarksBalance << std::endl;

  wb.append(result);
  return wb.build();
}
