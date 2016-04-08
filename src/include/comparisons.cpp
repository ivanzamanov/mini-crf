#include"comparisons.hpp"

extern double hann(double i, int size);

double compare_LogSpectrum(const Wave& result, const std::vector<FrameFrequencies>& frames2) {
  auto cmp = [=](const FrameFrequencies& f1, const FrameFrequencies& f2) {
    auto diff = 0.0;
    for(auto i = 0u; i < f1.size(); i++) {
      auto m1 = std::norm(f1[i]);
      m1 = m1 ? m1 : 1;
      auto m2 = std::norm(f2[i]);
      m2 = m2 ? m2 : 1;
      diff += std::pow( std::log10( m2 / m1), 2 );
    }
    return diff;
  };

  auto frames1 = toFFTdFrames(result);
  return compare(frames1, frames2, cmp) / frames1.size();
}

double compare_MFCC(const Wave& result, const std::vector<FrameFrequencies>& frames2) {
  auto sampleRate = result.sampleRate();
  auto cmp = [=](const FrameFrequencies& f1, const FrameFrequencies& f2) {
    std::array<double, 5> mfcc1, mfcc2;
    computeMFCC(f1, mfcc1, sampleRate);
    computeMFCC(f2, mfcc2, sampleRate);
    auto r = 0.0;
    for(auto i = 0u; i < mfcc1.size(); i++) {
      auto diff = mfcc1[i] - mfcc2[i];
      r += diff * diff;
    }
    return r;
  };

  auto frames1 = toFFTdFrames(result);
  return compare(frames1, frames2, cmp) / frames1.size();
}

double compare_SegSNR(const Wave& result, const Wave& original) {
  constexpr auto FSize = 512;
  FrameQueue<FSize> fq1(result),
    fq2(original);
  auto frameCount = 0;
  double total = 0;
  while(fq1.hasNext() && fq2.hasNext()) {
    frameCount++;
    auto &b1 = fq1.next(), &b2 = fq2.next();

    auto thisFrame = 0.0;
    for(auto i = 0u; i < b1.size(); i++) {
      auto quot = std::pow(b1[i], 2);
      auto denom = std::pow(b2[i] - b1[i], 2);
      if(denom != 0)
        thisFrame += quot / denom;
    }
    total += std::log10(thisFrame);
  }
  assert(fq1.offset == fq2.offset);
  return 1 / (total * 10.0 / frameCount);
}

static auto APPLY_WINDOW_CMP = true;
static auto TIME_STEP = 0.01;

std::vector<FrameFrequencies> toFFTdFrames(const Wave& wave) {
  std::vector<FrameFrequencies> result;
  std::valarray<cdouble> buffer(FFT_SIZE);

  auto step = wave.toSamples(TIME_STEP);
  for(auto frameOffset = 0;
      frameOffset + buffer.size() < wave.length();
      frameOffset += step) {
    WaveData frame = wave.extractBySample(frameOffset, frameOffset + buffer.size());

    for(auto i = 0u; i < buffer.size(); i++) {
      buffer[i] = frame[i];
      if(APPLY_WINDOW_CMP)
        buffer[i] *= hann(i, buffer.size());
    }

    ft::fft(buffer);
    FrameFrequencies values;
    std::copy(std::begin(buffer), std::end(buffer), values.begin());

    result.push_back(values);
  }
  assert(result.size() > 0);
  return result;
}
