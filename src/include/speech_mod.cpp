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
    destPtr->copy_from(ptr -> extract(p.start, p.end));
    // copy pitch marks
    each(fileData.pitch_marks,
         [&](float& mark) {
           if(mark >= p.start && mark <= p.end)
             destPtr -> marks.push_back(ptr -> at_time(mark) - ptr -> at_time(p.start));
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

vector<int> create_plan(vector<STSignal>& stSignals, frequency targetPitch,
                        const PhonemeInstance& tgt) {
  vector<int> plan; plan.resize(stSignals.size()); each(plan, [](int& i) { i = 0; });

  float dur = 0;
  while(dur < tgt.duration) {
    for(unsigned i = 0; i < stSignals.size() && dur < tgt.duration; i++) {
      dur += 1 / targetPitch;
      plan[i]++;
    }
  }
  return plan;
}

struct PitchRange {
  void set(frequency left, frequency right, int length) {
    this->left = left;
    this->right = right;
    this->length = length;
  }

  frequency at(int index) { return left * (1 - length / (index+1)) + right * (length/(index+1)); }
  frequency left;
  frequency right;
  int length;
};

void overlap_add(short* dest, int& lastPulse,
                 int max_offset, STSignal& st, PitchRange pr) {
  const int overlapCount = WaveData::toSamples(1/targetPitch);

  int destIndex = lastPulse - (st.length / 2 - overlapCount);

  int i = 0;
  //std::cerr << "From " << destIndex << " to " << lastPulse + overlapCount << std::endl;
  while(i < st.length && destIndex < max_offset) {
    // Actual overlap-add here
    // How to normalize energy, i.e account for energy distortion
    // caused by the -add
    dest[destIndex] += st[i++];
    destIndex++;
  }
  lastPulse += overlapCount;
}

void initPitchTier(PitchRange* tier, vector<PhonemeInstance> target) {
  unsigned i = 0;
  frequency left = std::exp(target[i].pitch_contour[0]);
  frequency right = std::exp(target[i].pitch_contour[1]);
  tier[i].set(left, right, WaveData::toSamples(target[i].duration));

  for(; i < target.size(); i++) {
    left = (std::exp(target[i].pitch_contour[0]) + tier[i-1].right) / 2;
    // Smooth out pitch at the concatenation points
    tier[i-1].right = left;
    right = std::exp(target[i].pitch_contour[1]);

    tier[i].set(left, right, WaveData::toSamples(target[i].duration));
  }
}

WaveData scaleToPitchAndDuration(SpeechWaveData& source, PitchRange pitch, float duration) {
  WaveData result = WaveData::allocate(duration);
  float scale = result.length / source.length;
  vector<float> scaledMarks;
  each(source.marks, [&](float mark) { scaledMarks.push_back(scale * mark); });
  if(scaledMarks.size() == 0)
    scaledMarks.push_back(duration + 1/pitch.at(WaveData::toSamples(duration)));

  int offset = 0;
  float mark = scaledMarks[0];
  int destNoiseStart = 0;
  int destNoiseEnd = WaveData::toSamples(mark - pitch.at(mark));
  while(offset < result.length) {
    
  }
  
  return result;
}

void SpeechWaveSynthesis::do_resynthesis(WaveData dest, SpeechWaveData* pieces) {
  PitchRange pitchTier[target.size()];
  initPitchTier(pitchTier, target);

  int offset = 0;
  for(unsigned i = 0; i < target.size(); i++) {
    SpeechWaveData& p = pieces[i];
    PhonemeInstance& tgt = target[i];
    WaveData modded = scaleToPitchAndDuration(p, pitchTier[i], tgt.duration);
    for(auto k = 0; k < modded.length; k++)
      dest[offset + k] = modded[k];
    delete modded.data;
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
  do_resynthesis(result, waveData);

  wb.append(result);
  return wb.build();
}
