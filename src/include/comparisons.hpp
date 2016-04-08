#ifndef __COMPARISONS_HPP__
#define __COMPARISONS_HPP__

#include"wav.hpp"
#include"fourier.hpp"
#include"util.hpp"
#include"libmfcc.hpp"

constexpr size_t FFT_SIZE = 512;
typedef std::array<cdouble, FFT_SIZE> FrameFrequencies;

std::vector<FrameFrequencies> toFFTdFrames(const Wave& wave);

double compare_LogSpectrum(const Wave&, const std::vector<FrameFrequencies>&);
double compare_MFCC(const Wave&, const std::vector<FrameFrequencies>&);

double compare_SegSNR(const Wave& result, const Wave& original);

template<class Frame, class Container>
void computeMFCC(const Frame& f, Container& h, int sampleRate) {
  double data[f.size()];
  for(auto i = 0u; i < f.size(); i++) data[i] = f[i].real();
  for(auto i = 0u; i < h.size(); i++)
    h[i] = GetCoefficient(data, sampleRate, 42, f.size(), i + 1);
}

template<class CMP>
double compare(const std::vector<FrameFrequencies>& frames1,
               const std::vector<FrameFrequencies>& frames2,
               CMP cmp) {
  bool check = std::abs((int) frames1.size() - (int) frames2.size()) <= 0;
  if(!check) {
    ERROR("Frames: " << frames1.size() << " vs " << frames2.size());
  }
  assert(check);

  int minSize = std::min(frames1.size(), frames2.size());
  double value = 0;
  for(auto j = 0; j < minSize; j++)
    value += cmp(frames1[j], frames2[j]);
  return value;
}

struct Comparisons {
  Comparisons()
    :LogSpectrum(0) { }

  Comparisons(double, double ls)
    :LogSpectrum(ls) { }

  Comparisons(const Comparisons& o)
    :LogSpectrum(o.LogSpectrum) { }

  //double ItakuraSaito;
  double LogSpectrum;
  double SegSNR;
  double MFCC;

  void fill(Wave& dist, Wave& original) {
    auto frames = toFFTdFrames(original);
    LogSpectrum = compare_LogSpectrum(dist, frames);
    SegSNR = compare_SegSNR(dist, original);
    MFCC = compare_MFCC(dist, frames);
  }

  bool operator<=(const Comparisons& o) const {
    return LogSpectrum <= o.LogSpectrum;
  }

  const Comparisons operator+(const Comparisons o) const {
    Comparisons result(0, LogSpectrum + o.LogSpectrum);
    return result;
  }

  double value() const {
    return LogSpectrum;
  }

  static Comparisons dummy() {
    static int c = 0;
    c++;
    return Comparisons(0, - (c * c) % 357);
  }

  static void aggregate(const std::vector<Comparisons>& params,
                        Comparisons* sum=0,
                        Comparisons* max=0,
                        Comparisons* avg=0) {
    Comparisons sumTemp, avgTemp;
    auto maxIndex = 0;
    for(auto i = 0u; i < params.size(); i++) {
      if(params[i] <= params[maxIndex])
        maxIndex = i;
      sumTemp = sumTemp + params[i];
    }
    avgTemp.LogSpectrum = sumTemp.value() / params.size();
    //avgTemp.ItakuraSaito = sumTemp.ItakuraSaito / params.size();
    if(sum)
      *sum = sumTemp;
    if(max)
      *max = params[maxIndex];
    if(avg)
      *avg = avgTemp;
  }
};

#endif
