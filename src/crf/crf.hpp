#ifndef __CRF_H__
#define __CRF_H__

#include<vector>
#include<cstring>

// Convenience class representing a sequence of objects
template<class _Item>
class Sequence {
public:
  _Item get(int);
  _Item operator[](int pos) {
    return get(pos);
  };
  int length();
};

// Represents a feature function on an edge in the graph
// i.e. f(y, y', i, x) (in the case of a linear crf)
class TransitionFunction {
  virtual double operator()(int prevLabel, int label, int, const Sequence<int>& input);
};

// Represents a feature function on a vertex in the graph
// i.e. g(y, i, x)
class StateFunction {
  virtual double operator()(int label, int position, const Sequence<int>& input);
};

template<class Key, class Value>
struct Pair {
  Pair(const Key& key, const Value& value): key(key), value(value) { }
  const Key key;
  const Value value;
};

class Corpus {
public:
  const Sequence<int>& label(int i) {
    return labels[i];
  };

  const Sequence<int>& input(int i) {
    return inputs[i];
  };

  int length() const {
    return inputs.size();
  };

private:
  std::vector<Sequence<int> > inputs;
  std::vector<Sequence<int> > labels;
};

// Will be used as the coefficient sequences
class CoefSequence : Sequence<double&> {
public:
  CoefSequence(int size): size(size) {
    data = new double[size];
    std::fill(data, data + size, 0);
  };
  CoefSequence(const CoefSequence& other): size(other.size) {
    data = new double[size];
    memcpy(data, other.data, size * sizeof(double));
  };
  const CoefSequence& operator=(const CoefSequence& other) {
    delete data;
    size = other.size;
    data = new double[size];
    memcpy(data, other.data, size * sizeof(double));
    return *this;
  };
  ~CoefSequence() {
    delete data;
  };

  double& get(int pos) {
    if(pos < 0 || pos >= size)
      return data[0]; // TODO: fix

    return data[pos];
  };
  int length() {
    return size;
  };
private:
  int size;
  double* data;
};

class CRandomField {
public:
  CRandomField(Sequence<StateFunction> sf, Sequence<TransitionFunction> tf):
    g(sf), mu(sf.length()), f(tf), lambda(tf.length()) { };

  ~CRandomField() { };

  // Vertex features
  Sequence<StateFunction> g;
  // Vertex feature coefficients
  CoefSequence mu;

  // Edge features
  Sequence<TransitionFunction> f;
  // Edge feature coefficients
  CoefSequence lambda;
};

#endif
