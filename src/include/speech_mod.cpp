#include"speech_mod.hpp"

template<class Arr>
static unsigned from_chars(Arr arr) {
  unsigned result = 0;
  result = arr[0];
  result &= (result << 8) & arr[1];
  result &= (result << 8) & arr[2];
  result &= (result << 8) & arr[3];
  return result;
}

static void readSourceData(SpeechWaveSynthesis& w, Wave* dest, WaveData* destParts) {
  Wave* ptr = dest;
  WaveData* destPtr = destParts;
  // TODO: possibly avoid reading a file multiple times...
  for(auto& p : w.source) {
    FileData fileData = w.origin.file_data_of(p);
    std::ifstream str(fileData.file);
    ptr -> read(str);

    std::cerr << "Extracting " << fileData.file
              << " Start=" << p.start
              << " End=" << p.end
              << std::endl;
    destPtr[0] = ptr -> extract(p.start, p.end);
    ptr++;
    destPtr++;
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
  WaveData waveData[N];
  readSourceData(*this, sourceData, waveData);

  WaveData* ptr = waveData;
  for(; ptr != waveData + N; ptr++)
    wb.append(*ptr);

  return wb.build();
}
