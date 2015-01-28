#ifndef __CRF_H__
#define __CRF_H__

#include<vector>
#include<cstring>

// Convenience class representing a sequence of objects
template<class _Item>
class Sequence {
public:
  Sequence(int size): size(size) {
    data = new _Item[size];
    std::fill(data, data + size, 0);
  };
  Sequence(const Sequence<_Item>& other): size(other.size) {
    data = new _Item[size];
    memcpy(data, other.data, size * sizeof(_Item));
  };
  const Sequence<_Item>& operator=(const Sequence<_Item>& other) {
    delete data;
    size = other.size;
    data = new _Item[size];
    memcpy(data, other.data, size * sizeof(_Item));
    return *this;
  };
  ~Sequence() {
    delete data;
  };

  _Item& operator[](int pos) {
    return get(pos);
  };

  _Item& get(int pos) {
    if(pos < 0 || pos >= size)
      return data[0]; // TODO: fix

    return data[pos];
  };
  int length() const {
    return size;
  };
private:
  int size;
  _Item* data;
};

typedef int Label;
typedef int Input;

// Represents a feature function on an edge in the graph
// i.e. f(y, y', i, x) (in the case of a linear crf)
class TransitionFunction {
  virtual double operator()(const Sequence<Label>, int, const Sequence<Input>&);
};

// Represents a feature function on a vertex in the graph
// i.e. g(y, i, x)
class StateFunction {
  virtual double operator()(const Sequence<Label>, int, const Sequence<Input>&);
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
  std::vector<Sequence<Input> > inputs;
  std::vector<Sequence<Label> > labels;
};

// Will be used as the coefficient sequences
typedef Sequence<double> CoefSequence;

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
