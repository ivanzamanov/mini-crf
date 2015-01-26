#ifndef __MFCC_H__
#define __MFCC_H__

#include<string>
#include<sstream>
#include<iostream>
#include<fstream>

#include"matrix.hpp"

class MFCCMatrix : public Matrix {
public:
  MFCCMatrix(int rows, int cols): Matrix(rows, cols) { }
  double xmin, xmax, dx, x1, ymin, ymax, dy, y1;
};

static double nextValue(std::istream& input, std::string& buf) {
  std::getline(input, buf);

  std::stringstream stream(buf);
  std::string localBuf;
  double result;

  stream >> localBuf;
  stream >> localBuf;
  stream >> result;

  return result;
}

static double nextMatrixValue(std::istream& input, std::string& buf) {
  std::getline(input, buf);
  std::stringstream stream(buf);
  std::string localBuf;
  stream >> localBuf;
  stream >> localBuf;
  stream >> localBuf;
  stream >> localBuf;

  double result;
  stream >> result;
  return result;
}

MFCCMatrix* readMFCCMatrix(std::string filename) {
  std::ifstream input(filename.c_str());

  std::string buf;
  std::getline(input, buf); //file type
  std::getline(input, buf); //object class
  std::getline(input, buf); // empty line

  double xmin = nextValue(input, buf),
    xmax = nextValue(input, buf),
    nx = nextValue(input, buf),
    dx = nextValue(input, buf),
    x1 = nextValue(input, buf),
    ymin = nextValue(input, buf),
    ymax = nextValue(input, buf),
    ny = nextValue(input, buf),
    dy = nextValue(input, buf),
    y1 = nextValue(input, buf);

  MFCCMatrix* result = new MFCCMatrix(ny, nx);
  MFCCMatrix& m = *result;
  m.xmin = xmin;
  m.xmax = xmax;
  m.dx = dx;
  m.x1 = x1;
  m.ymin = ymin;
  m.ymax = ymax;
  m.dy = dy;
  m.y1 = y1;

  // std::cout << result;

  // std::getline(input, buf); // something like z [] []
  for(int i = 0; i < m.rows(); i++) {
    std::getline(input, buf); // meaningless line
    // std::cout << buf << std::endl;
    for(int j = 0; j < m.cols(); j++) {
      double val = nextMatrixValue(input, buf);
      m(i, j) = val;
      result->rows();
    }
  }
  return result;
}

#endif
