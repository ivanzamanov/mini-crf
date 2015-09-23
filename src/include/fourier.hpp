#ifndef __FOURIER_HPP__
#define __FOURIER_HPP__

#include<cmath>

struct cdouble {
  cdouble(): real(0), img(0) { }
  cdouble(double r, double i): real(r), img(i) { }
  double real;
  double img;
};

namespace ft {
  void FT(double* values, cdouble* frequencies, int T) {
    double period = T;
    double real, img;
    for(int f = 0; f < T; f++) {
      real = img = 0;
      for(int t = 0; t < T; t++) {
        double arg = 2 * M_PI * (double) t * (double) f / period;
        real += values[t] * cos(arg);
        img  -= values[t] * sin(arg);
      }
      frequencies[f] = cdouble(real / period, img / period);
    }
  }

  void rFT(cdouble* frequencies, double* values, int T) {
    double period = T;
    for(int t = 0; t < T; t++) {
      double val = 0;
      for(int f = 0; f < T; f++) {
        double arg = 2 * M_PI * t * f / period;
        val += frequencies[f].real * cos(arg) - frequencies[f].img * sin(arg);
      }
      values[t] = val;
    }
  }
};

#endif
