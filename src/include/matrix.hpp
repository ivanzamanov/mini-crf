#ifndef __MATRIX_H__
#define __MATRIX_H__

#include<string>
#include<iostream>
#include<fstream>

template<class Value>
class Matrix {
public:
  Matrix(int rows, int cols): r(rows), c(cols) {
    data = new Value[r * c];
  }

  ~Matrix() {
    delete[] data;
  }

  Value& operator()(int row, int col) {
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
  Value* data;
};

template<class T>
// Square matrix only
class FileMatrixReader {
public:
  FileMatrixReader(std::string file_name, unsigned long side): stream(file_name), side(side) { }

  std::ifstream stream;
  unsigned long side;

  T get(unsigned row, unsigned col) {
    T result;
    stream.seekg((row * side + col) * sizeof(result));
    stream.read((char*) &result, sizeof(result));
    return result;
  }

  ~FileMatrixReader() { stream.close(); }
};

template<class T>
// Square matrix only
class FileMatrixWriter {
public:
  FileMatrixWriter(std::string file_name, unsigned long side): stream(file_name), side(side) { }

  std::ofstream stream;
  unsigned long side;

  void put(unsigned row, unsigned col, const T& val) {
    stream.seekp((row * side + col) * sizeof(val));
    stream.write((char*) &val, sizeof(val));
  }

  ~FileMatrixWriter() { stream.flush(); stream.close(); }
};

#endif
