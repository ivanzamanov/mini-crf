#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include<cmath>
#include<limits>
#include<vector>

#define DEBUG(x) x
#define MY_E 2.71828182845904523536028747135266250 // e

namespace util {
  double mult_exp(double x, double y);
  double mult(double x, double y);
  double log_mult(double logX, double logY);
  double log_sum(double logX, double logY);
  double log_sum2(double logX, double logY);
  double sum_log(double x, double y);
};

template<class T, int _length>
struct FixedArray {
  T values[_length];

  T& operator[](int n) {
    return values[n];
  };

  const T& operator[](int n) const {
    return values[n];
  };

  int length() {
    return _length;
  }
};

template<class T>
struct Array {
  int length;
  T* data;

  T& operator[](int n) {
    return data[n];
  };

  const T& operator[](int n) const {
    return data[n];
  };
};

template<class T>
Array<T> to_array(std::vector<T> &v) {
  Array<T> result;
  result.data = new T[v.size()];
  result.length = v.size();
  auto it = v.begin();
  int i = 0;
  while(it != v.end()) {
    result[i] = *it;
    i++;
    ++it;
  }
  return result;
}

#endif
