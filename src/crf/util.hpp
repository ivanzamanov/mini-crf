#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include<iostream>
#include<cmath>
#include<limits>
#include<vector>

#define DEBUG(x) x
#define MY_E 2.71828182845904523536028747135266250 // e

template<class Key, class Value>
struct Pair {
  Pair(const Key& key, const Value& value): key(key), value(value) { }
  const Key key;
  const Value value;
};

namespace util {
  double mult_exp(double x, double y);
  double mult(double x, double y);
  double log_mult(double logX, double logY);
  double log_sum(double logX, double logY);
  double log_sum2(double logX, double logY);
  double sum_log(double x, double y);
  double sum(double x, double y);

  int parse_int(const std::string& str);
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

  unsigned length() const {
    return _length;
  }
};

template<class T>
struct ArrayIteratorBase {
  ArrayIteratorBase(T* data): data(data) { }
  T* data;

  T& operator*() { return *data; }

  void increment() { data++; }
  void decrement() { data--; }

  bool operator==(const ArrayIteratorBase<T>& o) const { return data == o.data; }
  bool operator!=(const ArrayIteratorBase<T>& o) const { return data != o.data; }
};

template<class T, bool reverse = false>
struct ArrayIterator : public ArrayIteratorBase<T> {
  ArrayIterator(T* data): ArrayIteratorBase<T>(data) { }

  void operator++() { if(reverse) ArrayIteratorBase<T>::decrement(); else ArrayIteratorBase<T>::increment(); }
};

template<class T>
struct Array {
  Array() {
    length = 0;
    data = 0;
  }

  unsigned length;
  T* data;

  T& operator[](int n) { return data[n]; };
  const T& operator[](int n) const { return data[n]; };

  ArrayIterator<T> begin() { return ArrayIterator<T>(data); }
  ArrayIterator<T> end() { return ArrayIterator<T>(data + length); }

  ArrayIterator<T, true> rbegin() { return ArrayIterator<T, true>(data + length - 1); }
  ArrayIterator<T, true> rend() { return ArrayIterator<T, true>(data - 1); }

  void init(unsigned length) {
    this->length = length;
    data = new T[length];
  }
};

template<class T>
void singleton_array(Array<T>& result, T el) {
  result.init(1);
  result.data[0] = el;
}

bool compare(std::string &, std::string&);
bool compare(int& i1, int& i2);

template<class T>
bool compare(std::vector<T> &v1, std::vector<T> &v2) {
  bool same = v1.size() == v2.size();
  for(unsigned i = 0; i < v1.size(); i++)
    same = same && v1[i] == v2[i];
  return same;
}

template<class T>
bool compare(Array<T> a1, Array<T> a2) {
  bool same = a1.length == a2.length;
  for(unsigned i = 0; i < a1.length; i++)
    same = same && compare(a1[i], a2[i]);
  return same;
}

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

struct BinaryWriter {
  BinaryWriter(std::ostream* str): s(str), bytes(0) { }
  std::ostream* const s;
  unsigned bytes;

  template<class T>
  BinaryWriter& w(const T& val) {
    unsigned size = sizeof(val);
    s->write((char*) &val, size);
    bytes += size;
    return *this;
  }

  template<class T>
  BinaryWriter& operator<<(const T& val) {
    return w(val);
  }
};

struct BinaryReader {
  BinaryReader(std::istream* str): s(str), bytes(0) { }
  std::istream* const s;
  unsigned bytes;

  template<class T>
  BinaryReader& r(T& val) {
    unsigned size = sizeof(val);
    s->read((char*) &val, size);
    bytes += size;
    return *this;
  }

  template<class T>
  BinaryReader& operator>>(T& val) {
    return r(val);
  }
};

#endif
