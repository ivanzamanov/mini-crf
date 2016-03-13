#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include<iostream>
#include<sstream>
#include<cmath>
#include<cstring>
#include<limits>
#include<vector>
#include<tuple>

#define TEST(x) std::cerr << #x << ":\n" ; x(); std::cerr << "OK" << std::endl;

enum ColorCode {
  RED      = 31,
  GREEN    = 32,
  YELLOW   = 33,
  BLUE     = 34,
  DEFAULT  = 39
};

extern bool COLOR_ENABLED;
extern bool PRINT_SCALE;
extern bool REPORT_PROGRESS;

struct Color {
  Color(int code=ColorCode::DEFAULT): code(code) { }
  int code;

  friend std::ostream& operator<<(std::ostream& str, const Color& c) {
    return str << "\033[" << c.code << "m";
  }
};

#define DEBUG(x) ;
#define LOG(x) std::cerr << x << std::endl
#define LOG_COLOR(h, x, color) std::cerr << Color(ColorCode::color) << h << Color(ColorCode::DEFAULT) << x << std::endl
#define LOG_COLOR_OPT(h, x, color) if(COLOR_ENABLED) { LOG_COLOR(h, x, color); } else { LOG(h << x); }
#define INFO(x) LOG_COLOR_OPT("INFO: ", x, GREEN)
#define WARN(x) LOG_COLOR_OPT("WARN: ", x, YELLOW)
#define ERROR(x) LOG_COLOR_OPT("ERROR: ", x, RED)

#define PRINT_SCALE(x) if(PRINT_SCALE) LOG(x)

#define MY_E 2.71828182845904523536028747135266250 // e

template<class Key, class Value>
struct Pair {
  Pair(const Key& key, const Value& value): key(key), value(value) { }
  const Key key;
  const Value value;
};

namespace util {
  float mult_exp(float x, float y);
  float mult(float x, float y);
  float log_mult(float logX, float logY);
  float log_sum(float logX, float logY);
  float log_sum2(float logX, float logY);
  float sum_log(float x, float y);
  float sum(float x, float y);

  template<class T>
  T parse(const std::string& str) {
    T result;
    std::stringstream stream(str);
    stream >> result;
    return result;
  }

  template<class T, class Func>
  void each(T* ptr, int length, Func f) {
    for(int i = 0; i < length; i++)
      f(ptr[i]);
  }

  template<class T, class Func>
  void each2(T* ptr, int length, Func f) {
    for(int i = 0; i < length; i++)
      f(i, ptr[i]);
  }

  template<class T, class Func>
  void transform(T* ptr, int length, Func f) {
    for(int i = 0; i < length; i++)
      ptr[i] = f(i, ptr[i]);
  }

  template<class T, class Func>
  void transform(std::vector<T>& v, Func f) {
    for(unsigned i = 0; i < v.size(); i++)
      v[i] = f(i, v[i]);
  }

  template<class T, class Func>
  void each(std::vector<T> &v, Func f) {
    for(auto& el : v)
      f(el);
  }

  template<class T, class Func>
  void each(const std::vector<T> &v, Func f) {
    for(auto& el : v)
      f(el);
  }

  template<class T, class Func>
  void each2(std::vector<T> &v, Func f) {
    int i = 0;
    for(auto& el : v)
      f(i++, el);
  }

  std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
  std::vector<std::string> split_string(const std::string &s, char delim);

  template<class V, class Acc>
  std::ostream& join_output(std::ostream& o, const V& values, Acc acc, std::string sep="") {
    o << acc(values[0]);
    for(auto i = 1u; i < values.size(); i++)
      o << sep << acc(values[i]);
    return o;
  }
};

bool compare(std::string &, std::string&);
bool compare(int& i1, int& i2);

template<class T>
bool compare(std::vector<T> &v1, std::vector<T> &v2) {
  bool same = v1.size() == v2.size();
  for(unsigned i = 0; i < v1.size(); i++)
    same = same && v1[i] == v2[i];
  return same;
}

struct BinaryWriter {
  BinaryWriter(std::ostream* str): s(str), bytes(0) { }
  std::ostream* const s;
  unsigned bytes;

  BinaryWriter& w(const std::string& val) { return wVec(val); }
  template<class T>
  BinaryWriter& w(const std::vector<T>& val) { return wVec(val); }

  template<class T>
  BinaryWriter& wVec(T& val) {
    (*this) << (unsigned) val.size();
    for(auto& el : val)
      (*this) << el;
    return (*this);
  }

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

  BinaryReader& r(std::string& val) { return rVec(val); }
  template<class T>
  BinaryReader& r(std::vector<T>& val) { return rVec(val); }

  template<class T>
  BinaryReader& rVec(T& val) {
    unsigned len = 0;
    (*this) >> len;
    val.resize(len);
    for(auto& el : val)
      (*this) >> el;
    return (*this);
  }

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

  bool ok() {
    return (bool) *s;
  }
};

struct Progress {
  static bool enabled;
  
  Progress(unsigned total=0, std::string prefix="Progress: "): total(total), progress(0), prefix(prefix) { }
  unsigned total, progress;
  std::string prefix;

  void update() {
    if(REPORT_PROGRESS) {
      std::cerr << '\r' << prefix << progress++ << '/' << total << " ";
      std::cerr.flush();
    }
  }

  void finish() {
    if(REPORT_PROGRESS)
      std::cerr << " Done " << std::endl;
  }

};

template<class T, unsigned size>
struct pqueue : std::array<T, size> {
  explicit pqueue(): std::array<T, size>() { }

  template<class Comparator>
  pqueue& push(const T& v, const Comparator& cmp) {
    auto& t = *this;

    auto i = 0u;
    while(i < size && cmp(t[i], v))
      i++;
    if(i >= size) return *this;

    int j = ((int) size) - 2;
    while(j >= (int) i) {
      t[j + 1] = t[j];
      j--;
    }

    t[i] = v;
    return *this;
  }
};

template<class T, class... Ts>
std::array<T, sizeof...(Ts)> make_array(Ts... args) {
  return std::array<T, sizeof...(Ts)>{{args...}};
}

namespace tuples {
  template<unsigned size,
           class Tuple>
  struct Invoke {
    template<class Container, class Invoker>
    void operator()(Container& c, const Invoker& inv) const {
      auto val = inv( std::get<size - 1>(Tuple{}) );
      c[size - 1] = val;
      Invoke<size - 1, Tuple>{}(c, inv);
    }
  };
  template<class Tuple>
  struct Invoke<1, Tuple> {
    template<class Container, class Invoker>
    void operator()(Container& c, const Invoker& inv) const {
      c[0] = inv(std::get<0>(Tuple{}));
    }
  };

  template<bool flag>
  struct ConditionalInvoke {
    template<class Func, class V1, class V2, class V3>
    double operator()(const Func f, const bool flag2, const V1&, const V2& p2, const V3& p3) const {
      if(flag2)
        return f(p2, p3);
      return 0.0;
    }
  };

  template<>
  struct ConditionalInvoke<true> {
    template<class Func, class V1, class V2, class V3>
    double operator()(const Func f, const bool flag2, const V1& state, const V2& prev, const V3& next) const {
      if(flag2)
        return f(state, prev);
      else
        return f(state, next);
    }
  };

  template<class Label, class X>
  struct Applicator {
    const int pos;
    const bool isTransition;
    const Label& srcOrState;
    const Label& dest;
    const X& x;

    template<class F>
    double operator()(F func) const {
      // either we're not calculating the last position
      // or this is a state function
      return ConditionalInvoke<F::is_state>{}(func, isTransition,
                                              x[pos], srcOrState, dest);
    }
  };
}

#endif
