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
    float srcDuration = 0.2, destDuration = 1.0;
    SpeechWaveData source(WaveData::allocate(srcDuration));
    PitchRange pitch;
    pitch.set(5.0, 5.0, WaveData::toSamples(destDuration));
    WaveData dest = WaveData::allocate(destDuration);
    int destOffset = 0;

    scaleToPitchAndDuration(dest, &destOffset, source, pitch, destDuration);
    assertEquals("end offset", dest.length, destOffset);
  } catch(char const*) {

  }
}
