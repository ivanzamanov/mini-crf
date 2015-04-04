#ifndef __CRF_H__
#define __CRF_H__

#include<cmath>
#include<vector>
#include<cstring>
#include<limits>
#include<iostream>
#include<algorithm>

#include"util.hpp"
#include"../dot/dot.hpp"

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
      lambda[i] = 1;
    }

    for(int i = 0; i < g.length(); i++) {
      g[i]->alphabet = &label_alphabet;
      mu[i] = 1;
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
double crf_probability_of(const Sequence<Label>& y, const Sequence<Input>& x, CRF& crf, const CoefSequence& lambdas, const CoefSequence& mus) {
  double numer = 0;
  for(int i = 1; i < y.length(); i++) {
    for(int lambda = 0; lambda < crf.f.length(); lambda++) {
      auto func = crf.f[lambda];
      auto coef = lambdas[lambda];
      numer += coef * (*func)(y, i, x);
    }
  }

  for(int i = 0; i < y.length(); i++) {
    for(int mu = 0; mu < crf.g.length(); mu++) {
      auto func = crf.g[mu];
      auto coef = mus[mu];
      numer += coef * (*func)(y, i, x);
    }
  }

  double denom = norm_factor(x, crf, lambdas, mus);
  return numer - std::log(denom);
}

double child_index(int pos, int alphabet_len) {
  return 1 + (pos * alphabet_len);
}

template<class CRF>
double state_value(CRF& crf, const CoefSequence& mu, const Sequence<Input>& x, int label, int pos) {
  if(crf.g.length() == 0)
    return 1;
  
  double result = 0;
  for(int i = 0; i < crf.g.length(); i++) {
    result += mu[i] * (*crf.g[i])(label, pos, x);
  }
  return result;
}

template<class CRF_T>
double transition_value(CRF_T& crf, const Sequence<Input>& x, int label1, int label2, int pos) {
  return transition_value(crf, x, crf.lambda, crf.mu, label1, label2, pos);
}

template<class CRF_T>
double transition_value(CRF_T& crf, const CoefSequence& lambda, const CoefSequence& mu, const Sequence<Input>& x, int label1, int label2, int pos) {
  double result = 0;
  if(crf.f.length() == 0)
    return 1;
  for (int i = 0; i < lambda.length(); i++) {
    auto* func = crf.f[i];
    double coef = lambda[i];
    result += coef * (*func)(label1, label2, pos, x);
  }
  result += state_value(crf, mu, x, label2, pos);
  return result;
}

template<class CRF>
void max_path(const Sequence<Input>& x, CRF& crf, const CoefSequence& lambda, const CoefSequence& mu, std::vector<int>* max_path) {
  norm_factor(x, crf, lambda, mu, max_path);
}

template<class CRF>
double norm_factor(const Sequence<Input>& x, CRF& crf, const CoefSequence& lambda, const CoefSequence& mu) {
  return norm_factor(x, crf, lambda, mu, 0);
}

double min(double* arr, int n, int& index) {
  double result = MAX;
  for(int i = 0; i < n; i++) {
    if(COMPARE(arr[i], result)) {
      result = arr[i];
      index = i;
    }
  }
  return result;
}

template<class CRF>
double norm_factor(const Sequence<Input>& x, CRF& crf, const CoefSequence& lambda, const CoefSequence& mu, std::vector<int>* max_path) {
  DEBUG(DotPrinter printer("automaton.dot");
        printer.start();
        )

  int alphabet_len = crf.label_alphabet.phonemes.length;
  int autom_size = alphabet_len * x.length() + 2;
  double* table = new double[autom_size];

  // Will need for intermediate computations
  double* tr_values = new double[alphabet_len];
  // A slice of the table, actually...
  double* child_values;
  // Transitions q,i,y -> f
  int index = autom_size - 1;
  table[index] = 1;
  index--;

  DEBUG(printer.node(index, -2);)

  std::vector<int>* paths = 0;
  if(max_path) {
    paths = new std::vector<int>[alphabet_len];
  }

  int pos = x.length() - 1;
  // transitions to final state
  for(int dest = 0; dest < alphabet_len; dest++, index--) {
    // value of the last "column" of states
    bool is_allowed = crf.label_alphabet.allowedState(dest, x[pos]);
    table[index] = is_allowed;
    DEBUG(
          printer.node(index, dest);
          printer.edge(index, autom_size - 1, ' ', 1);
          );
  }
  
  // backwards, for every zero-based position in
  // the input sequence except the last one...
  for(pos--; pos >= 0; pos--) {
    // for every possible label...
    DEBUG(std::cerr << "At position " << pos << std::endl);

    for(int src = alphabet_len - 1; src >= 0; src--, index--) {
      // Short-circuit if different phoneme types
      if(crf.label_alphabet.allowedState(src, x[pos])) {
        int child = child_index(pos + 1, alphabet_len);
      
        for(int dest = alphabet_len - 1; dest >= 0; dest--) {
          double tr_value;
          if(!crf.label_alphabet.allowedTransition(src, dest)) {
            tr_value = 0;
          } else {
            tr_value = transition_value(crf, lambda, mu, x, src, dest, pos);
          }
        
          tr_values[dest] = tr_value;

          DEBUG(printer.edge(index, child + dest, ' ', tr_value));
        }

        child_values = table + child;
        // Initialize with one transition's value
        table[index] = 0;
        for(int dest = alphabet_len - 1; dest >= 0; dest--) { // Exclude that one transition
          // tr_value is the power of the transition' actual value
          double tr_value = tr_values[dest];
          // child_value is the log of the child's actual value
          double child_value = child_values[dest];

          double increment = util::mult(tr_value, child_value);
          tr_values[dest] = increment;
          double d = util::sum(increment, table[index]);
          table[index] = d;
        }

        DEBUG(printer.node(index, table[index]));

        // This, however, should never be 0
        if(table[index] == 0 || std::isinf(table[index])) {
          std::cerr << table[index] << " at " << index << std::endl;
          table[index] = std::numeric_limits<double>::min();
        }
      }

      if(max_path) {
        double* max = std::max_element(tr_values, tr_values + alphabet_len);
        paths[src].push_back(max - tr_values);
      }
    }
  }

  DEBUG(printer.node(index, -1));

  int child = child_index(0, alphabet_len);
  table[index] = 0; // Again, initialize with one state and exclude from further sums
  for(int src = 0; src < alphabet_len; src++) {
    double state_val = state_value(crf, mu, x, src, 0);
    double increment = util::mult(state_val, table[child + src]);
    tr_values[src] = increment;
    table[index] = util::sum(table[index], increment);

    DEBUG(printer.edge(index, child + src, ' ', increment));
  }

  if(max_path) {
    double* max = std::max_element(tr_values, tr_values + alphabet_len);
    max_path->push_back(max - tr_values);
    for(auto it = paths[max - tr_values].rbegin(); it != paths[max - tr_values].rend(); it++)
      max_path->push_back(*it);
    delete[] paths;
  }

  double denom = table[0];
  delete[] table;
  DEBUG(printer.end());
  return denom;
}

#endif

