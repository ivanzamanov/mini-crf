#ifndef __FOURIER_HPP__
#define __FOURIER_HPP__

#include<complex>
#include<cmath>
#include<valarray>

typedef std::complex<double> cdouble;

namespace ft {
  template<class Val, class Freqs=cdouble*>
  void FT(Val* values, int T, Freqs& frequencies, int F) {
    double period = T;
    double real, img;
    for(int f = 0; f < F; f++) {
      double freq = f;
      real = img = 0;
      for(int t = 0; t < T; t++) {
        double arg = 2 * M_PI * t * freq / period;
        real += values[t] * cos(arg);
        img  -= values[t] * sin(arg);
      }
      frequencies[f] = cdouble(real / period, img / period);
    }
  }

  template<class Val>
  void rFT(cdouble* frequencies, int F, Val* values, int T, int period) {
    for(int t = 0; t < T; t++) {
      double val = 0;
      for(int f = 0; f < F; f++) {
        double freq = f;
        double arg = 2 * M_PI * t * freq / period;
        val += frequencies[f].real() * cos(arg) - frequencies[f].imag() * sin(arg);
      }
      values[t] = val;
    }
  }
 
  template<class Complex>
  void fft(std::valarray<Complex>& x) {
    const size_t N = x.size();
    if (N <= 1) return;
 
    // divide
    std::valarray<Complex> even = x[std::slice(0, N/2, 2)];
    std::valarray<Complex>  odd = x[std::slice(1, N/2, 2)];
 
    // conquer
    fft(even);
    fft(odd);
 
    // combine
    for (size_t k = 0; k < N/2; ++k) {
        auto t = std::polar(1.0, -2 * M_PI * k / N) * odd[k];
        x[k    ] = even[k] + t;
        x[k+N/2] = even[k] - t;
      }
  }

  // inverse fft (in-place)
  template<class Complex>
  void ifft(std::valarray<Complex>& x) {
    // conjugate the complex numbers
    x = x.apply(std::conj);
 
    // forward fft
    fft(x);
 
    // conjugate the complex numbers again
    x = x.apply(std::conj);
 
    // scale the numbers
    x /= x.size();
  }
};

#endif
