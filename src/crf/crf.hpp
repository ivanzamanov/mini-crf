#ifndef __CRF_H__
#define __CRF_H__

#include<cmath>
#include<vector>
#include<cstring>
#include<limits>
#include<iostream>

// Convenience class representing a sequence of objects
template<class _Item>
class Sequence {
public:
  Sequence(int size): size(size) {
    data = new _Item[size];
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
    delete[] data;
  };

  const _Item& operator[](int pos) const {
    if(pos < 0 || pos >= size)
      return data[0]; // TODO: fix

    return data[pos];
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

template<class Key, class Value>
struct Pair {
  Pair(const Key& key, const Value& value): key(key), value(value) { }
  const Key key;
  const Value value;
};


typedef int Label;
typedef int Input;

class Corpus {
public:
  const Sequence<Label>& label(int i) {
    return labels[i];
  };

  const Sequence<Input>& input(int i) {
    return inputs[i];
  };

  int length() const {
    return inputs.size();
  };

  void add(Sequence<Input>& input, Sequence<Input>& labels) {
    this->inputs.push_back(input);
    this->labels.push_back(labels);
  };

private:
  std::vector<Sequence<Input> > inputs;
  std::vector<Sequence<Label> > labels;
};

// Will be used as the coefficient sequences
typedef Sequence<double> CoefSequence;

template<class LabelAlphabet>
class CRandomField {
public:
  class BaseFunction {
  public:
    LabelAlphabet* alphabet;
    virtual ~BaseFunction() { };
  };
  
  class StateFunction : public BaseFunction {
  public:
    virtual double operator()(const Sequence<Label>&, int, const Sequence<Input>&) const {
      return 0;
    }
    virtual double operator()(const Label, int, const Sequence<Input>&) const {
      return 0;
    };
  };

  class TransitionFunction : public BaseFunction {
  public:
    virtual double operator()(const Sequence<Label>&, int, const Sequence<Input>&) const {
      return 0;
    }
    virtual double operator()(const Label, const Label, const int, const Sequence<Input>&) const {
      return 0;
    };
  };

  CRandomField(Sequence<StateFunction*> sf, Sequence<TransitionFunction*> tf):
    g(sf), mu(sf.length()), f(tf), lambda(tf.length()) {
    for(int i = 0; i < f.length(); i++) {
      f[i]->alphabet = &label_alphabet;
    }

    for(int i = 0; i < g.length(); i++) {
      g[i]->alphabet = &label_alphabet;
    }
  };

  ~CRandomField() { };

  double probability_of(const Sequence<Label>& y, const Sequence<Input> x) const {
    return crf_probability_of(y, x, *this, lambda, mu);
  }

  // Vertex features
  Sequence<StateFunction*> g;
  // Vertex feature coefficients
  CoefSequence mu;

  // Edge features
  Sequence<TransitionFunction*> f;
  // Edge feature coefficients
  CoefSequence lambda;

  LabelAlphabet label_alphabet;
};

template<class CRF>
struct AllSumAccumulator {
  AllSumAccumulator(const CRF& crf,
                    const Sequence<Input>& inputs,
                    const CoefSequence& mu,
                    const CoefSequence& lambda):
    crf(crf), x(inputs), lambda(lambda), mu(mu) { }

  const CRF& crf;
  const Sequence<Input>& x;
  const CoefSequence& lambda;
  const CoefSequence& mu;
  double result = 0;
  int count = 0;

  void operator()(const Sequence<Label>& y) {
    double sum = 0;
    for(int i = 1; i < y.length(); i++) {
      for(int j = 0; j < crf.f.length(); j++) {
        sum += lambda[i] * crf.f[j](y, i, x);
      }
      for(int j = 0; j < crf.g.length(); j++) {
        sum += mu[i] * crf.g[j](y, i, x);
      }
    }
    result += std::exp(sum);
  }
};

template<class CRF>
double crf_probability_of_2(const Sequence<Label>& y, const Sequence<Input>& x, CRF& crf, const CoefSequence& lambda, const CoefSequence& mu) {
  double numer = 0;
  for(int i = 1; i < y.length(); i++)
    numer += lambda[i - 1] * crf.f[i - 1](y, i, x);

  AllSumAccumulator<CRF> filter(crf, x, lambda, mu);
  // TODO: make proper iterator...
  crf.label_alphabet.iterate_sequences(x, filter);

  return numer - std::log(filter.result);
}

template<class CRF>
double crf_probability_of(const Sequence<Label>& y, const Sequence<Input>& x, CRF& crf, const CoefSequence& lambda, const CoefSequence& mu) {
  double numer = 0;
  for(int i = 1; i < y.length(); i++)
    numer += lambda[i - 1] * (*crf.f[i - 1])(y, i, x);

  double denom = norm_factor(x, crf, lambda, mu);
  return numer - std::log(denom);
}

double child_index(int pos, int alphabet_len) {
  return 1 + (pos * alphabet_len);
}

template<class CRF>
double state_value(CRF& crf, const CoefSequence& mu, const Sequence<Input>& x, int label, int pos) {
  double result = 0;
  for(int i = 0; i < crf.g.length(); i++)
    result += mu[i] * (*crf.g[i])(label, pos, x);
  return result;
}

template<class CRF_T>
double transition_value(CRF_T& crf, const CoefSequence& lambda, const CoefSequence& mu, const Sequence<Input>& x, int label1, int label2, int pos) {
  if(!crf.label_alphabet.allowedTransition(label1, label2))
    return 0;

  double result = 0;
  for (int i = 0; i < lambda.length(); i++) {
    const typename CRF_T::TransitionFunction* func = crf.f[i];
    double coef = lambda[i];
    result += coef * (*func)(label1, label2, pos, x);
  }
  result += state_value(crf, mu, x, label2, pos);
  return result;
}

#include"../dot/dot.hpp"
template<class CRF>
double norm_factor(const Sequence<Input>& x, CRF& crf, const CoefSequence& lambda, const CoefSequence& mu) {
  return norm_factor(x, crf, lambda, mu, 0);
}

template<class CRF>
double norm_factor(const Sequence<Input>& x, CRF& crf, const CoefSequence& lambda, const CoefSequence& mu, std::vector<int>* max_path) {
  DotPrinter printer("automaton.dot");
  //DotPrinter printer(0);
  printer.start();
  int alphabet_len = crf.label_alphabet.phonemes.length;
  int autom_size = alphabet_len * x.length() + 2;
  double* table = new double[autom_size];
  // Transitions q,i,y -> f
  int index = autom_size - 1;
  table[index] = 1;
  printer.node(index, -2);
  index--;

  std::vector<int>* paths;
  if(max_path) {
    paths = new std::vector<int>[alphabet_len];
  }

  // transitions to final state
  for(int dest = 0; dest < alphabet_len; dest++, index--) {
    table[index] = 1;
    printer.node(index, dest);
    printer.edge(index, autom_size - 1, ' ', 1);
  }
  
  // backwards, for every position in the input sequence...
  for(int k = x.length() - 2; k >= 0; k--)
    // for every possible label...
    for(int src = alphabet_len - 1; src >= 0; src--, index--) {
      int child = child_index(k + 1, alphabet_len);
      table[index] = 0;
      printer.node(index, src);

      int max_label = -1;
      double max_increment = std::numeric_limits<double>::min();
      for(int dest = alphabet_len - 1; dest >= 0; dest--) {
        double tr_value = transition_value(crf, lambda, mu, x, src, dest, k);
        double increment = tr_value * table[child + dest];

        table[index] += increment;
        if(max_label == -1 || max_increment <= increment) {
          max_increment = increment;
          max_label = dest;
        }
        printer.edge(index, child + dest, ' ', tr_value);
      }
      if(paths)
        paths[src].push_back(max_label);
    }

  int child = child_index(0, alphabet_len);
  table[index] = 0;
  printer.node(index, -1);
  int max_path_index = -1;
  double max_val = std::numeric_limits<double>::min();
  for(int src = 0; src < alphabet_len; src++) {
    double increment = state_value(crf, mu, x, src, 0) * table[child + src];
    table[index] += increment;
    printer.edge(index, child + src, ' ', increment);

    paths[src].push_back(src);
    if(max_path_index == -1 || max_val <= increment) {
      max_path_index = src;
      max_val = increment;
    }
  }

  if(paths) {
    *max_path = paths[max_path_index];
    delete[] paths;
  }

  double denom = table[0];
  delete table;
  printer.end();
  return denom;
}

#endif
