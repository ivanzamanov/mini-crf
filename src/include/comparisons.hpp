#ifndef __COMPARISONS_HPP__
#define __COMPARISONS_HPP__

#include<cmath>

#include"wav.hpp"
#include"fourier.hpp"
#include"util.hpp"
#include"libmfcc.hpp"

constexpr size_t FFT_SIZE = 512;
typedef std::array<cdouble, FFT_SIZE> FrameFrequencies;

struct CmpValues {
  std::vector<double> v;
  void add(double d) {
    //if(std::isnan(d))
    //  return;
    v.push_back(d);
  }

  unsigned size() const { return v.size(); }
  double& operator[](unsigned i) { return v[i]; }
};

std::vector<FrameFrequencies> toFFTdFrames(const Wave&, CmpValues* = 0);

double compare_LogSpectrum(const Wave&, const std::vector<FrameFrequencies>&, CmpValues* = 0);
double compare_LogSpectrumCritical(const Wave&, const std::vector<FrameFrequencies>&, CmpValues* = 0);
double compare_MFCC(const Wave&, const std::vector<FrameFrequencies>&, CmpValues* = 0);
double compare_WSS(const Wave&, const std::vector<FrameFrequencies>&, CmpValues* = 0);
double compare_SegSNR(const Wave& result, const Wave& original, CmpValues* = 0);

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

  Comparisons(const Comparisons& o)
    :LogSpectrum(o.LogSpectrum) { }

  double LogSpectrum;
  double LogSpectrumCritical;
  double SegSNR;
  double MFCC;
  double WSS;

  void fill(Wave& dist, Wave& original) {
    auto frames = toFFTdFrames(original);
    LogSpectrum = compare_LogSpectrum(dist, frames);
    LogSpectrumCritical = compare_LogSpectrumCritical(dist, frames);
    SegSNR = compare_SegSNR(dist, original);
    MFCC = compare_MFCC(dist, frames);
    WSS = compare_WSS(dist, frames);
  }

  static double compare(const Wave& signal,
                        const Wave& originalSignal,
                        const std::vector<FrameFrequencies>& original,
                        std::string metric) {
    if(metric == "MFCC")
      return compare_MFCC(signal, original);
    if(metric == "LogSpectrum")
      return compare_LogSpectrum(signal, original);
    if(metric == "LogSpectrumCritical")
      return compare_LogSpectrumCritical(signal, original);
    if(metric == "WSS")
      return compare_WSS(signal, original);
    if(metric == "SegSNR")
      return compare_SegSNR(originalSignal, signal);
    assert(false); // No such metric
  }

  static void aggregate(const std::vector<double>& params,
                        double* sum=0,
                        double* max=0,
                        double* avg=0) {
    double sumTemp = 0,
      avgTemp = 0;
    auto maxIndex = 0;
    for(auto i = 0u; i < params.size(); i++) {
      if(params[i] <= params[maxIndex])
        maxIndex = i;
      sumTemp = sumTemp + params[i];
    }
    avgTemp = sumTemp / params.size();
    if(sum)
      *sum = sumTemp;
    if(max)
      *max = params[maxIndex];
    if(avg)
      *avg = avgTemp;
  }
};

struct ComparisonDetails : public Comparisons {
  CmpValues LogSpectrumValues,
    LogSpectrumCriticalValues,
    SegSNRValues,
    MFCCValues,
    WSSValues;

  CmpValues times;

  void fill(Wave& dist, Wave& original) {
    auto frames = toFFTdFrames(original, &times);
    LogSpectrum = compare_LogSpectrum(dist, frames, &LogSpectrumValues);
    LogSpectrumCritical = compare_LogSpectrumCritical(dist, frames, &LogSpectrumCriticalValues);
    SegSNR = compare_SegSNR(dist, original, &SegSNRValues);
    MFCC = compare_MFCC(dist, frames, &MFCCValues);
    WSS = compare_WSS(dist, frames, &WSSValues);
  }  
};

#endif
