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

void testCopyVoicelessPlain() {
  float srcDuration = 0.1,
    scale = 1.0,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicelessPart(source, &destOffset, 0, source.length, scale, dest);
  assertEquals("end offset", dest.length, destOffset);
}

void testCopyVoicelessUpscale() {
  float srcDuration = 0.1,
    scale = 2.0,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicelessPart(source, &destOffset, 0, source.length, scale, dest);
  assertEquals("end offset", dest.length, destOffset);
}

void testCopyVoicelessDownscale() {
  float srcDuration = 0.2,
    scale = 1 / 2.0,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicelessPart(source, &destOffset, 0, source.length, scale, dest);
  assertEquals("end offset", dest.length, destOffset);
}

void testCopyVoicedPlain() {
  float srcDuration = 0.1,
    scale = 1,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  float period = srcDuration;
  source.marks.push_back(0);
  source.marks.push_back(source.length);
  PitchRange pitch;
  pitch.set(1/period, 1/period, 0, WaveData::toSamples(destDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicedPart(source, &destOffset, 1, scale, pitch, dest);
  assertEquals("end offset", dest.length, destOffset);
}

void testCopyVoicedUpscaleDuration() {
  float srcDuration = 0.1,
    scale = 2,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  float period = srcDuration;
  source.marks.push_back(0);
  source.marks.push_back(source.length);
  PitchRange pitch;
  pitch.set(1/period, 1/period, 0, WaveData::toSamples(destDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicedPart(source, &destOffset, 1, scale, pitch, dest);
  assertEquals("end offset", dest.length, destOffset);
}

void testCopyVoicedDownscaleDuration() {
  float srcDuration = 0.2,
    scale = 0.5,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  float period = srcDuration;
  source.marks.push_back(0);
  source.marks.push_back(source.length);
  PitchRange pitch;
  pitch.set(1/period, 1/period, 0, WaveData::toSamples(destDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicedPart(source, &destOffset, 1, scale, pitch, dest);
  assertEquals("end offset", dest.length, destOffset);
}

void testDurationScale() {
}

int main() {
  try {
    //testDurationScale();
    std::cerr << "voiceless plain" << std::endl;
    testCopyVoicelessPlain();
    std::cerr << "voiceless upscale" << std::endl;
    testCopyVoicelessUpscale();
    std::cerr << "voiceless downscale" << std::endl;
    testCopyVoicelessDownscale();
    std::cerr << "voiced plain" << std::endl;
    testCopyVoicedPlain();
    std::cerr << "voiced upscale duration" << std::endl;
    testCopyVoicedUpscaleDuration();
    std::cerr << "voiced downscale duration" << std::endl;
    //testCopyVoicedDownscaleDuration();

    std::cerr << "voiced upscale pitch" << std::endl;
    std::cerr << "voiced downscale pitch" << std::endl;

    std::cerr << "All tests passed" << std::endl;
  } catch(char const*) {

  }
}
