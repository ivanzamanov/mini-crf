#include "textgrid.hpp"
#include "mfcc.hpp"

int main() {
  MFCCMatrix* matrix = readMFCCMatrix("test.Matrix");
  std::cout << "Success parsing MFCC!" << std::endl;
  std::cout << "Size: " << matrix->rows() <<
            "x" << matrix->cols() << std::endl;
  TextGrid* grid = readIntervalsTextGrid("test.TextGrid");
  std::cout << "Success parsing TextGrid!" << std::endl;
  std::cout << "Size: " << grid->size() << std::endl;
}
