#include<cmath>

#include"speech_mod.cpp"

using std::vector;
using namespace util;

typedef SpeechWaveData STSignal;

template<class T>
void assertEquals(std::string msg, T expected, T actual) {
  if(!(expected == actual)) {
    std::cerr << msg << ": "
              << "Expected: " << expected
              << " Actual: " << actual << std::endl;
    throw "Assert failed";
  }
}

int main() {
  try {
    float srcDuration = 0.4, destDuration = 0.2;
    SpeechWaveData source(WaveData::allocate(srcDuration));
    source.marks.push_back(WaveData::toSamples(0.11));
    source.marks.push_back(WaveData::toSamples(0.12));
    source.marks.push_back(WaveData::toSamples(0.13));
    source.marks.push_back(WaveData::toSamples(0.14));
    
    source.marks.push_back(WaveData::toSamples(0.18));
    source.marks.push_back(WaveData::toSamples(0.19));
    PitchRange pitch;
    pitch.set(500.0, 500.0, WaveData::toSamples(destDuration));
    WaveData dest = WaveData::allocate(destDuration);
    int destOffset = 0;

    scaleToPitchAndDuration(dest, &destOffset, source, pitch, destDuration);
    assertEquals("end offset", dest.length, destOffset);
  } catch(char const*) {

  }
}
