#include"mfcc.hpp"

int main() {
  MFCCMatrix* matrix = readMFCCMatrix("test.Matrix");

  std::cout << "Success!" << std::endl;
  std::cout << "Size: " << matrix->rows() << "x" << matrix->cols() << std::endl;

  // for(int i = 0; i < matrix->rows(); i++) {
  //   for (int j = 0; j < matrix->cols(); j++) {
  //     std::cout << (*matrix)(i,j) << " ";
  //   }
  //   std::cout << std::endl;
  // }
}
