#ifndef __FOURIER_HPP__
#define __FOURIER_HPP__


template<typename V>
V fftCoef(const V* values, const int t, const int m) {
  return 0;
}

template<typename V>
void fft(const V* values, const int tMax, double* coefs, const int nCoef) {
  int mid = nCoef;
  coefs[nCoef] = fftCoef(values, tMax, 0);
  for(int i = 1; i <= nCoef; i++) {
    coefs[nCoef + i] = fftCoef(values, tMax, i);
    coefs[nCoef - i] = fftCoef(values, tMax, i);
  }
}

#endif
