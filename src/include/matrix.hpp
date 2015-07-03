#ifndef __MATRIX_H__
#define __MATRIX_H__

#include<string>
#include<iostream>
#include<fstream>

template<class Value>
class Matrix {
public:
  Matrix(unsigned rows, unsigned cols): r(rows), c(cols) {
    unsigned long total = r * c;
    data = new Value[total];
  }

  ~Matrix() {
    delete[] data;
  }

  Value& operator()(unsigned row, unsigned col) {
    if(row >= rows() ||  col >= cols()) {
      throw "Invalid element requested";
    }

    unsigned long index = row * c + col;
    return data[index];
  }

  unsigned rows() const {
    return r;
  }

  unsigned cols() const {
    return c;
  }

  unsigned long r;
  unsigned long c;
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
