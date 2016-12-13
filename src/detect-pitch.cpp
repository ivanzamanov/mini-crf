#include <cassert>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <queue>

#include "wav.hpp"
#include "util.hpp"

using std::vector;

typedef double frequency;

frequency MIN_F = 50;
frequency MAX_F = 600;

vector<frequency> pitchInFrame(Wave wav, double start, double size);

int main(int, char** argv) {
  Wave wav(argv[1]);

  auto window = 0.015;
  auto timeStep = 0.005;
  auto duration = wav.duration();


  vector<vector<frequency>> candidates;
  for(double start = 0; start + window < duration; start += timeStep)
    candidates.push_back(pitchInFrame(wav, start, window));
}

template<class T>
struct mean_accumulator {
  T mean = 0;
  int n = 0;
  void add(T val) {
    n++;
    mean = mean + (val - mean) / n;
  }
};
struct candidate {
  frequency f;
  double c;
};

/*template<class It>
double mean(It& begin, const It& end) {
  mean_accumulator<double> acc;
  for(; begin != end; ++begin)
    acc.add(*begin);
  return acc.mean;
  }*/

vector<frequency> pitchInFrame(Wave wav, double start, double size) {
  auto cmp = [=](const candidate& c1, const candidate& c2) { return c1.c > c2.c; };
  auto pq = std::priority_queue<candidate, std::vector<candidate>, decltype(cmp)>(cmp);

  auto start_sample = wav.toSamples(start);
  auto sample_size = wav.toSamples(size);

  auto mean = (double)
    std::accumulate(wav.data.get() + start_sample,
                    wav.data.get() + start_sample + sample_size,
                    0,
                    [](short v1, short v2) { return v1 + v2; }) / sample_size;

  auto stdDev = (double)
    std::accumulate(wav.data.get() + start_sample,
                    wav.data.get() + start_sample + sample_size,
                    0,
                    [](short v1, short v2) { return v1 + std::pow(v2, 2); }) / sample_size;

  std::cout << "Mean: " << mean << '\n';

  auto avgXsq = 0.0;
  auto avgYsq = 0.0;
  for(frequency f = MIN_F; f < MAX_F; f++) {
    double period = 1 / f;
    unsigned offset = wav.toSamples(period);

    auto num = 0.0;
    for(auto i = 0; i < sample_size; i++) {
      auto index_x = start + i;
      auto index_y = start + (i + offset) % sample_size;

      auto xi = wav[index_x];
      auto yi = wav[index_y];

      num += (xi - avg) * (yi - avg);
      avgXsq += std::pow(xi, 2);
      avgYsq += std::pow(yi, 2);
    }
    avgXsq /= sample_size;
    avgXsq = std::sqrt(avgXsq - avgSq);

    avgYsq /= sample_size;
    avgYsq = std::sqrt(avgYsq - avgSq);

    auto correlation = num / (avgXsq * avgYsq);

    std::cout << correlation << " " << avgXsq << " " << avgYsq << '\n';

    assert(correlation <= 1);
    assert(correlation >= -1);
  }

  auto result = vector<frequency>();
  for(auto i = 0; i < 5; i++) {
    result.push_back(pq.top().f);
    std::cout << pq.top().f << " " << pq.top().c << '\n';
    pq.pop();
  }
  return result;
}
