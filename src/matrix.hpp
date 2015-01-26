#ifndef __MATRIX_H__
#define __MATRIX_H__

class Matrix {
public:
  Matrix(int rows, int cols): r(rows), c(cols) {
    data = new double[r * c];
  }

  ~Matrix() {
    delete data;
  }

  double& operator()(int row, int col) {
    if(row >= rows() || row < 0 || col < 0 || col >= cols()) {
      throw "Invalid element requested";
    }

    return data[row * c + col];
  }

  int rows() const {
    return r;
  }

  int cols() const {
    return c;
  }

  int r;
  int c;
  double* data;

  double xmin, xmax, dx, x1, ymin, ymax, dy, y1;
};

#endif
