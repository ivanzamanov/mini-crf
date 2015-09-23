#include"fourier.hpp"
#include"util.hpp"

int main() {
  const int T = 100;
  double values[T];
  for(int i = 0; i < T; i++) {
    double t = -M_PI / 2 + i / (double) T * 2 * M_PI;
    values[i] = cos(2.0 * t);
  }

  cdouble frequencies[T];
  ft::FT(values, frequencies, T);
  double inversed[T];
  ft::rFT(frequencies, inversed, T);

  double err = 0;
  for(int i = 0; i < T; i++) {
    err += std::abs(inversed[i] - values[i]);
    std::cout << frequencies[i].real << " " << frequencies[i].img << " " << inversed[i] <<  " " << values[i] << std::endl;
  }

  std::cout << "Error: " << err << std::endl;
}
