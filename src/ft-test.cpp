#include"fourier.hpp"
#include"util.hpp"

int main() {
  const int T = 100;
  double values[T];
  for(int i = 0; i < T; i++) {
    double t = -M_PI / 2 + i / (double) T * 2 * M_PI;
    values[i] = cos(2.0 * t);
  }

  const int F = 100;
  cdouble frequencies[T];
  ft::FT(values, T, frequencies, F);
  double inversed[T];
  ft::rFT(frequencies, F, inversed, T);
}
