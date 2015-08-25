#include<cmath>

#include"speech_mod.cpp"

using std::vector;
using namespace util;

template<class T>
void assertEquals(std::string msg, T expected, T actual) {
  if(!(expected == actual)) {
    std::cerr << msg << ": "
              << "Expected: " << expected
              << " Actual: " << actual << std::endl;
    throw "Assert failed";
  }
}

void testOverlapAddAroundMark() {
  float srcDuration = MAX_VOICELESS_PERIOD,
    scale = 1.0,
    destDuration = scale * srcDuration;

  int mark = WaveData::toSamples(srcDuration) / 2;
  SpeechWaveData source(WaveData::allocate(srcDuration));
  for(int i = 0; i < source.length; i++)
    source[i] = i;
  WaveData dest = WaveData::allocate(destDuration);

  float period = WaveData::toDuration(mark);
  overlapAddAroundMark(source, mark, dest, mark, period, period);

  //  for(int i = 1; i < source.length; i++)
    //    assertEquals("Copied value", source[i - 1], dest[i]);
}

const int VOICELESS_MULT = 2;
void testCopyVoicelessPlain() {
  float srcDuration = MAX_VOICELESS_PERIOD * VOICELESS_MULT,
    scale = 1.0,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicelessPart(source, destOffset, dest.length, 0, source.length, scale, dest);
  assertEquals("end offset", dest.length, destOffset);
  delete[] source.data;
  delete[] dest.data;
}

void testCopyVoicelessUpscale() {
  float srcDuration = MAX_VOICELESS_PERIOD * VOICELESS_MULT,
    scale = 2.0,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicelessPart(source, destOffset, dest.length, 0, source.length, scale, dest);
  assertEquals("end offset", dest.length, destOffset);
  delete[] source.data;
  delete[] dest.data;
}

void testCopyVoicelessDownscale() {
  float srcDuration = MAX_VOICELESS_PERIOD * VOICELESS_MULT,
    scale = 1 / 2.0,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicelessPart(source, destOffset, dest.length, 0, source.length, scale, dest);
  assertEquals("end offset", dest.length, destOffset);
  delete[] source.data;
  delete[] dest.data;
}

void testCopyVoicedPlain() {
  float srcDuration = MAX_VOICELESS_PERIOD * VOICELESS_MULT,
    scale = 1,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  float period = srcDuration;
  source.marks.push_back(0);
  source.marks.push_back(source.length);
  PitchRange pitch;
  PitchTier pt {
  ranges: &pitch,
      length: 1
  };
  pitch.set(1/period, 1/period, 0, WaveData::toSamples(destDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicedPart(source, destOffset, dest.length, 0, scale, pt, dest);
  assertEquals("end offset", dest.length, destOffset);
  delete[] source.data;
  delete[] dest.data;
}

void testCopyVoicedEdge() {
  float srcDuration = MAX_VOICELESS_PERIOD * VOICELESS_MULT,
    scale = 1,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  float period = srcDuration / 10;
  source.marks.push_back(0);
  source.marks.push_back(source.length);
  PitchRange pitch;
  PitchTier pt {
      ranges: &pitch,
      length: 1
  };
  pitch.set(1/period, 1/period, 0, WaveData::toSamples(destDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicedPart(source, destOffset, dest.length, 0, scale, pt, dest);
  assertEquals("end offset", dest.length, destOffset);
  delete[] source.data;
  delete[] dest.data;
}

void testCopyVoicedUpscaleDuration() {
  float srcDuration = MAX_VOICELESS_PERIOD * VOICELESS_MULT,
    scale = 2,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  float period = srcDuration / 2;
  source.marks.push_back(0);
  //source.marks.push_back(source.length / 2);
  source.marks.push_back(source.length);
  PitchRange pitch;
  PitchTier pt {
  ranges: &pitch,
      length: 1
  };
  pitch.set(1/period, 1/period, 0, WaveData::toSamples(destDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicedPart(source, destOffset, dest.length, 0, scale, pt, dest);
  assertEquals("end offset", dest.length, destOffset);
  delete[] source.data;
  delete[] dest.data;
}

void testCopyVoicedDownscaleDuration() {
  float srcDuration = MAX_VOICELESS_PERIOD * VOICELESS_MULT,
    scale = 0.5,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  float period = srcDuration / 2.0;
  source.marks.push_back(0);
  //source.marks.push_back(source.length / 2);
  source.marks.push_back(source.length);
  PitchRange pitch;
  PitchTier pt {
  ranges: &pitch,
      length: 1
  };
  pitch.set(1/period, 1/period, 0, WaveData::toSamples(destDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicedPart(source, destOffset, dest.length, 0, scale, pt, dest);
  assertEquals("end offset", dest.length, destOffset);
  delete[] source.data;
  delete[] dest.data;
}

void testVoicedUpscalePitch() {
  float srcDuration = MAX_VOICELESS_PERIOD * 4,
    scale = 1,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  source.marks.push_back(0);
  source.marks.push_back(MAX_VOICELESS_SAMPLES * 1);
  source.marks.push_back(MAX_VOICELESS_SAMPLES * 2);
  source.marks.push_back(source.length);
  PitchRange pitch;
  PitchTier pt {
  ranges: &pitch,
      length: 1
  };
  float newPitch = 1 / (MAX_VOICELESS_PERIOD / 2);
  pitch.set(newPitch,
            newPitch,
            0, WaveData::toSamples(destDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicedPart(source, destOffset, dest.length, 0, scale, pt, dest);
  assertEquals("end offset", WaveData::toSamples(2 * (1 / newPitch)), destOffset);
  delete[] source.data;
  delete[] dest.data;
}

void testVoicedDownscalePitch() {
  float srcDuration = MAX_VOICELESS_PERIOD * 4,
    scale = 1,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  source.marks.push_back(0);
  source.marks.push_back(MAX_VOICELESS_SAMPLES * 1);
  source.marks.push_back(MAX_VOICELESS_SAMPLES * 2);
  source.marks.push_back(source.length);
  PitchRange pitch;
  PitchTier pt {
  ranges: &pitch,
      length: 1
  };
  float newPitch = 1 / (MAX_VOICELESS_PERIOD * 2);
  pitch.set(newPitch,
            newPitch,
            0, WaveData::toSamples(destDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicedPart(source, destOffset, dest.length, 0, scale, pt, dest);
  assertEquals("end offset", WaveData::toSamples(1 / newPitch), destOffset);
  delete[] source.data;
  delete[] dest.data;
}

void testVoicedUpscalePitchDownscaleDuration() {
  float srcDuration = MAX_VOICELESS_PERIOD * 4,
    scale = 1 / 2.0,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  source.marks.push_back(0);
  source.marks.push_back(MAX_VOICELESS_SAMPLES * 1);
  source.marks.push_back(MAX_VOICELESS_SAMPLES * 2);
  source.marks.push_back(source.length);
  PitchRange pitch;
  PitchTier pt {
  ranges: &pitch,
      length: 1
  };
  float newPitch = 1 / (MAX_VOICELESS_PERIOD / 2);
  pitch.set(newPitch,
            newPitch,
            0, WaveData::toSamples(destDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicedPart(source, destOffset, dest.length, 0, scale, pt, dest);
  assertEquals("end offset", WaveData::toSamples(1 / newPitch), destOffset);
  delete[] source.data;
  delete[] dest.data;
}

void testVoicedDownscalePitchUpscaleDuration() {
  float srcDuration = MAX_VOICELESS_PERIOD * 4,
    scale = 2,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  source.marks.push_back(0);
  source.marks.push_back(MAX_VOICELESS_SAMPLES * 1);
  source.marks.push_back(MAX_VOICELESS_SAMPLES * 2);
  source.marks.push_back(source.length);
  PitchRange pitch;
  PitchTier pt {
  ranges: &pitch,
      length: 1
  };
  float newPitch = 1 / (MAX_VOICELESS_PERIOD * 2.0);
  pitch.set(newPitch,
            newPitch,
            0, WaveData::toSamples(destDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicedPart(source, destOffset, dest.length, 0, scale, pt, dest);
  assertEquals("end offset", WaveData::toSamples(1 / newPitch), destOffset);
  delete[] source.data;
  delete[] dest.data;
}

void testVoicedDownscalePitchDownscaleDuration() {
  float srcDuration = MAX_VOICELESS_PERIOD * 4,
    scale = 1/2.0,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  source.marks.push_back(0);
  source.marks.push_back(MAX_VOICELESS_SAMPLES * 1);
  source.marks.push_back(MAX_VOICELESS_SAMPLES * 2);
  source.marks.push_back(source.length);
  PitchRange pitch;
  PitchTier pt {
  ranges: &pitch,
      length: 1
  };
  float newPitch = 1 / (MAX_VOICELESS_PERIOD * 2.0);
  pitch.set(newPitch,
            newPitch,
            0, WaveData::toSamples(destDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicedPart(source, destOffset, dest.length, 0, scale, pt, dest);
  assertEquals("end offset", WaveData::toSamples(1 / newPitch), destOffset);
  delete[] source.data;
  delete[] dest.data;
}

void testVoicedUpscalePitchUpscaleDuration() {
  float srcDuration = MAX_VOICELESS_PERIOD * 4,
    scale = 2.0,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  source.marks.push_back(0);
  source.marks.push_back(MAX_VOICELESS_SAMPLES * 1);
  source.marks.push_back(MAX_VOICELESS_SAMPLES * 2);
  source.marks.push_back(source.length);
  PitchRange pitch;
  PitchTier pt {
  ranges: &pitch,
      length: 1
  };
  float newPitch = 1 / (MAX_VOICELESS_PERIOD / 2);
  pitch.set(newPitch,
            newPitch,
            0, WaveData::toSamples(destDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  copyVoicedPart(source, destOffset, dest.length, 0, scale, pt, dest);
  assertEquals("end offset", WaveData::toSamples(4 * 1 / newPitch), destOffset);
  delete[] source.data;
  delete[] dest.data;
}

void testScaleToPitch() {
  float srcDuration = MAX_VOICELESS_PERIOD * 10,
    scale = 2.0,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  for(int i = 0; i < source.length; i++)
    source[i] = i;
  source.marks.push_back(MAX_VOICELESS_SAMPLES * 4);
  source.marks.push_back(MAX_VOICELESS_SAMPLES * 4.5);
  source.marks.push_back(MAX_VOICELESS_SAMPLES * 5.0);

  PitchRange pitch;
  PitchTier pt {
  ranges: &pitch,
      length: 1
  };
  float newPitch = 1 / (MAX_VOICELESS_PERIOD / 4);
  pitch.set(newPitch,
            newPitch,
            0, WaveData::toSamples(destDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  scaleToPitchAndDuration(dest, destOffset, source, pt, destDuration);
  // Slight rounding error
  assertEquals("end offset", WaveData::toSamples(destDuration), destOffset);
  delete[] source.data;
  delete[] dest.data;
}

void testScaleToPitchEdge() {
  float srcDuration = 0.0615833327,
    scale = 1.0,
    destDuration = scale * srcDuration;

  SpeechWaveData source(WaveData::allocate(srcDuration));
  for(int i = 0; i < source.length; i++)
    source[i] = i;
  source.marks.push_back(779);
  source.marks.push_back(893);
  source.marks.push_back(1009);
  source.marks.push_back(1125);
  source.marks.push_back(1242);
  source.marks.push_back(1359);
  source.marks.push_back(1476);
  PitchRange pitch;
  PitchTier pt {
  ranges: &pitch,
      length: 1
  };
  pitch.set(229,
            204,
            0, WaveData::toSamples(destDuration));
  WaveData dest = WaveData::allocate(destDuration);
  int destOffset = 0;

  scaleToPitchAndDuration(dest, destOffset, source, pt, destDuration);
  // Slight rounding error
  assertEquals("end offset", WaveData::toSamples(destDuration), destOffset);
  delete[] source.data;
  delete[] dest.data;
}

int main() {
  try {
    TEST(testOverlapAddAroundMark);
    TEST(testCopyVoicelessPlain);
    TEST(testCopyVoicelessUpscale);
    TEST(testCopyVoicelessDownscale);
    TEST(testCopyVoicedPlain);
    TEST(testCopyVoicedUpscaleDuration);
    TEST(testCopyVoicedDownscaleDuration);
    TEST(testVoicedUpscalePitch);
    TEST(testVoicedDownscalePitch);

    TEST(testVoicedUpscalePitchDownscaleDuration);
    TEST(testVoicedDownscalePitchUpscaleDuration);
    TEST(testVoicedUpscalePitchUpscaleDuration);
    TEST(testVoicedDownscalePitchDownscaleDuration);

    TEST(testScaleToPitch);
    TEST(testScaleToPitchEdge);

    std::cerr << "All tests passed" << std::endl;
  } catch(char const* err) {
    std::cerr << err << std::endl;
  }
}
