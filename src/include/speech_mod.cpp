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

    /*
      std::cerr << "Extracting " << fileData.file
      << " Start=" << p.start
      << " End=" << p.end
      << std::endl;
    */
    // extract wave data
    *destPtr = ptr -> extract(p.start, p.end);
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

static void do_copy(short* dest, WaveData d) {
  for(int i = 0; i < d.length; i++)
    dest[i] = d.data[d.offset + i];
}

static vector<SpeechWaveData> extractSTSignals(const SpeechWaveData& swd) {
  
}

static float estimateDuration(vector<SpeechWaveData>& swds, const PhonemeInstance& p) {

}

typedef SpeechWaveData STSignal;
vector<int> create_plan(vector<STSignal>& stSignals, const PhonemeInstance& tgt) {
  // Between the beginning and first pitch mark will always be included
  vector<int> plan; plan.resize(stSignals.size()); each(plan, [](int& i) { i = 0; });
  float targetPitch = (tgt.pitch_contour[0] + tgt.pitch_contour[1]) / 2;

  float dur = (stSignals[0].marks[1] - stSignals[1].marks[0]) / DEFAULT_SAMPLE_RATE;
  while(dur < tgt.duration) {
    for(unsigned i = 1; i < stSignals.size() && dur < tgt.duration; i++) {
      STSignal& stC = stSignals[i];

      float localDur = stC.localDuration(1);
      float localPitch = stC.localPitch(1);

      float scale = targetPitch / localPitch;
      dur += localDur * scale;

      plan[i]++;
    }
    if(dur < tgt.duration)
      dur += stSignals[stSignals.size() - 1].localDuration(2);
  }
  return plan;
}

void SpeechWaveSynthesis::do_resynthesis(short* dest, SpeechWaveData* pieces) {
  double durationError = 0;
  int destOffset = 0;
  for(int i = 0; i < target.size(); i++) {
    SpeechWaveData& p = pieces[i];
    PhonemeInstance& tgt = target[i];
    PhonemeInstance& src = target[i];
    double actualDuration = 0;
    // Hann-windowed short-term signals to be recombined
    vector<STSignal> stSignals = extractSTSignals(p);
    // Create a resynthesis plan
    vector<int> plan = create_plan(stSignals, tgt);

    // Now recombine
    int last_mark = stSignals[0].mark();
    do_copy(dest, stSignals[0]);
    for(int j = 1; j < stSignals.size(); j++) {
      WaveData& st = stSignals[j];
      
    }

    durationError = actualDuration - tgt.duration;
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
  short* result = WaveData::allocate(completeDuration);
  do_resynthesis(waveData, result);

  each(waveData, N,
       [&](SpeechWaveData& swd) {
         wb.append(swd);
       });

  return wb.build();
}
