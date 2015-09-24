#ifndef __FOURIER_HPP__
#define __FOURIER_HPP__

#include<cmath>

struct cdouble {
  cdouble(): real(0), img(0) { }
  cdouble(double r, double i): real(r), img(i) { }
  double real;
  double img;

  bool operator==(const cdouble& o) const {
    return real == o.real && img == o.img;
  }
};

namespace ft {
  void FT(double* values, int T, cdouble* frequencies, int F, int offsetF=0) {
    double period = T;
    double real, img;
    for(int f = 0; f < F; f++) {
      double freq = f + offsetF;
      real = img = 0;
      for(int t = 0; t < T; t++) {
        double arg = 2 * M_PI * t * freq / period;
        real += values[t] * cos(arg);
        img  -= values[t] * sin(arg);
      }
      frequencies[f] = cdouble(real / period, img / period);
    }
  }

  void rFT(cdouble* frequencies, int F, double* values, int T, int offsetF=0) {
    double period = T;
    for(int t = 0; t < T; t++) {
      double val = 0;
      for(int f = 0; f < F; f++) {
        double freq = f + offsetF;
        double arg = 2 * M_PI * t * freq / period;
        val += frequencies[f].real * cos(arg) - frequencies[f].img * sin(arg);
      }
      values[t] = val;
    }
  }
};

#endif
