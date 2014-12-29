#ifndef __CRF_H__
#define __CRF_H__

#include<cstring>

// Convenience class representing a sequence of objects
template<class _Item>
class Sequence {
  _Item get(int);
  _Item operator[](int pos) {
    return get(pos);
  };
  int length();
};

// Represents a feature function on an edge in the graph
// i.e. f(y, y', i, x) (in the case of a linear crf)
template<class _Label, class _Input>
class TransitionFunction {
  virtual double operator()(const _Label&, const _Label&, int, const Sequence<_Input>&);
};

// Represents a feature function on a vertex in the graph
// i.e. g(y, i, x)
template<class _Label, class _Input>
class StateFunction {
  virtual double operator()(const _Label&, int, const Sequence<_Input>&);
};

template<class _Label, class _Input>
class Corpus {
public:
  const Sequence<_Label>& labels(int i) {
    
  };

  const Sequence<_Input>& input(int i) {
    
  };

private:
  vector<Sequence<_Input>> 
};

// Will be used as the coefficient sequences
class CoefSequence : Sequence<double&> {
public:
  CoefSequence(int size): size(size) {
    data = new double[size];
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

template<class _Label, class _Input>
class CRandomField {
public:
  CRandomField(Sequence<StateFunction<_Label, _Input> > sf, Sequence<TransitionFunction<_Label, _Input> > tf):
    sf(sf), sf_c(sf.length()), tf(tf), tf_c(tf.length()) { };

  ~CRandomField() { };

  Sequence<StateFunction<_Label, _Input> > get_vertex_features() {
    return sf;
  };

  CoefSequence get_vertex_coefficients() {
    return sf_c;
  };

  Sequence<TransitionFunction<_Label, _Input> > get_edge_features() {
    return tf;
  };

  CoefSequence get_edge_coefficients() {
    return tf_c;
  };

  void train(Corpus<_Label, _Input> corpus) {

  };

private:
  // Vertex features
  Sequence<StateFunction<_Label, _Input> > sf;
  // Vertex feature coefficients
  CoefSequence sf_c;

  // Edge features
  Sequence<TransitionFunction<_Label, _Input> > tf;
  // Edge feature coefficients
  CoefSequence tf_c;
};

#endif

