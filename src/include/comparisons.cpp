#include<algorithm>

#include"comparisons.hpp"

extern double hann(double i, int size);

typedef std::array<double, 24> CriticalBands;
static CriticalBands toCriticalBands(const FrameFrequencies& fq, int sampleRate);
static CriticalBands toSpectralSlopes(const CriticalBands& bands);

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

double compare_LogSpectrumCritical(const Wave& result,
                                   const std::vector<FrameFrequencies>& frames2) {
  auto sampleRate = result.sampleRate();
  auto cmp = [=](const FrameFrequencies& f1, const FrameFrequencies& f2) {
    auto bands1 = toCriticalBands(f1, sampleRate);
    auto bands2 = toCriticalBands(f2, sampleRate);

    auto diff = 0.0;
    for(auto i = 0u; i < bands1.size(); i++)
      diff += std::pow( std::log10(bands1[i] / bands2[i]), 2);
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

//static const CriticalBands BARK_CENTERS {{ 50, 150, 250, 350, 450, 570, 700, 840, 1000, 1170, 1370, 1600, 1850, 2150, 2500, 2900, 3400, 4000, 4800, 5800, 7000, 8500, 10500, 13500 }};
static const std::array<int, 25> BARK_BAND_EDGES {{ 0, 100, 200, 300, 400, 510, 630, 770, 920, 1080, 1270, 1480, 1720, 2000, 2320, 2700, 3150, 3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500 }};

static CriticalBands toCriticalBands(const FrameFrequencies& fq, int sampleRate) {
  auto samples = fq.size();
  auto fromHertz = [=](double hertz) {
    return std::round(hertz * (double) samples / sampleRate);
  };

  CriticalBands powers;
  std::fill(powers.begin(), powers.end(), 0);

  for(auto i = 0u; i < powers.size(); i++) {
    auto botF = fromHertz(BARK_BAND_EDGES[i]),
      topF = fromHertz(BARK_BAND_EDGES[i + 1]);
    for(; botF <= topF; botF++) {
      assert(botF >= 0 && botF < fq.size());
      powers[i] += std::norm(fq[botF]);
    }
  }
  return powers;
}

static CriticalBands toSpectralSlopes(const CriticalBands& bands) {
  CriticalBands slopes;
  slopes[slopes.size() - 1] = 0;
  for(auto i = 1u; i < bands.size(); i++)
    slopes[i - 1] = bands[i] - bands[i-1];
  return slopes;
}

static double findPeak(const CriticalBands& bands,
                       const CriticalBands& slopes, int i) {
  if(slopes[i] > 0) {
    while(slopes[i + 1] > 0 && i + 1 < (int) slopes.size())
      i++;
  } else {
    while(slopes[i - 1] < 0 && i - 1 >= 0)
      i--;
  }
  return bands[i];
}

static CriticalBands computeCriticalBandWeights(const CriticalBands& bands,
                                                const CriticalBands& slopes) {
  CriticalBands nearestPeaks;
  for(auto i = 0u; i < nearestPeaks.size(); i++)
    nearestPeaks[i] = findPeak(bands, slopes, i);
  auto dbMax = *std::max_element(bands.begin(), bands.end());
  CriticalBands w;
  for(auto i = 0u; i < nearestPeaks.size(); i++)
    w[i] = 20 / (20 + dbMax - bands[i]) * 1 / (1 + nearestPeaks[i] - bands[i]);
  return w;
}

// As defined in Performance Assesment Method For Speech Enhancement Systems
double compare_WSS(const Wave& result, const std::vector<FrameFrequencies>& frames2) {
  auto sampleRate = result.sampleRate();
  auto cmp = [=](const FrameFrequencies& f1, const FrameFrequencies& f2) {
    auto bands1 = toCriticalBands(f1, sampleRate);
    std::transform(bands1.begin(), bands1.end(), bands1.begin(), [](double d) { return 10 * std::log10(d); });
    auto slopes1 = toSpectralSlopes(bands1);
    auto weights1 = computeCriticalBandWeights(bands1, slopes1);

    auto bands2 = toCriticalBands(f2, sampleRate);
    std::transform(bands2.begin(), bands2.end(), bands2.begin(), [](double d) { return 10 * std::log10(d); });
    auto slopes2 = toSpectralSlopes(bands2);
    auto weights2 = computeCriticalBandWeights(bands2, slopes2);

    auto result = 0.0;
    for(auto i = 1u; i < slopes1.size(); i++)
      result += weights1[i] * weights2[i] / 2 * std::pow(slopes1[i] - slopes2[i], 2);
    return result;
  };

  auto frames1 = toFFTdFrames(result);
  return compare(frames1, frames2, cmp) / frames1.size();
}

static auto APPLY_WINDOW_CMP = true;
static auto TIME_STEP = 0.01;

std::vector<FrameFrequencies> toFFTdFrames(const Wave& wave) {
  double constexpr NORM = std::numeric_limits<short>::max();
  std::vector<FrameFrequencies> result;
  std::valarray<cdouble> buffer(FFT_SIZE);

  auto step = wave.toSamples(TIME_STEP);
  for(auto frameOffset = 0;
      frameOffset + buffer.size() < wave.length();
      frameOffset += step) {
    WaveData frame = wave.extractBySample(frameOffset, frameOffset + buffer.size());

    for(auto i = 0u; i < buffer.size(); i++) {
      buffer[i] = frame[i] / NORM;
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
