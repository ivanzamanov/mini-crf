#include"fourier.hpp"
#include"util.hpp"

int main() {
  const int T = 512;
  std::complex<double> values[T];
  for(int i = 0; i < T; i++) {
    double t = -M_PI / 2 + i / (double) T * 2 * M_PI;
    values[i] = cos(2.0 * t);
  }
  std::valarray<std::complex<double>> valArr(values, T);
  std::valarray<std::complex<double>> copy = valArr;

  ft::fft(valArr);
  ft::ifft(valArr);

  double err = 0;
  for(int i = 0; i < T; i++) {
    err += std::abs(valArr[i] - copy[i]);
  }
  std::cout << "Err: " << err << std::endl;
}
