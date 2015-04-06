#ifndef __CRF_H__
#define __CRF_H__

#include<cstring>
#include<iostream>
#include<algorithm>

#include"util.hpp"
#include"../dot/dot.hpp"

using std::vector;

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
  const vector<Label>& label(int i) {
    return labels[i];
  };

  const vector<Input>& input(int i) {
    return inputs[i];
  };

  int size() const {
    return inputs.size();
  };

  void add(vector<Input>& input, vector<Input>& labels) {
    this->inputs.push_back(input);
    this->labels.push_back(labels);
  };

private:
  vector<vector<Input> > inputs;
  vector<vector<Label> > labels;
};

template<class LabelAlphabet>
class CRandomField {
public:
  typedef LabelAlphabet Alphabet;

  class BaseFunction {
  public:
    LabelAlphabet* alphabet;
    virtual ~BaseFunction() { };
  };

  class StateFunction : public BaseFunction {
  public:
    virtual double operator()(const vector<Label>&, int, const vector<Input>&) const {
      return 0;
    }

    virtual double operator()(const Label, int, const vector<Input>&) const {
      return 0;
    };
  };

  class TransitionFunction : public BaseFunction {
  public:
    virtual double operator()(const vector<Label>&, int, const vector<Input>&) const {
      return 0;
    }
    virtual double operator()(const Label, const Label, const int, const vector<Input>&) const {
      return 0;
    };
  };

  CRandomField(): g(), mu(), f(), lambda() { };

  CRandomField(vector<StateFunction*> sf, vector<TransitionFunction*> tf):
    g(sf), mu(), f(tf), lambda() {
    for(unsigned i = 0; i < f.size(); i++) {
      f[i]->alphabet = &label_alphabet;
      lambda.push_back(1);
    }

    for(unsigned i = 0; i < g.size(); i++) {
      g[i]->alphabet = &label_alphabet;
      mu.push_back(1);
    }
  };

  ~CRandomField() { };

  double probability_of(const vector<Label>& y, const vector<Input> x) const {
    return crf_probability_of(y, x, *this, lambda, mu);
  }

  // Vertex features
  vector<StateFunction*> g;
  // Vertex feature coefficients
  vector<double> mu;

  // Edge features
  vector<TransitionFunction*> f;
  // Edge feature coefficients
  vector<double> lambda;

  LabelAlphabet label_alphabet;
};

template<class CRF>
struct AllSumAccumulator {
  AllSumAccumulator(const CRF& crf,
                    const vector<Input>& inputs,
                    const vector<double>& mu,
                    const vector<double>& lambda):
    crf(crf), x(inputs), lambda(lambda), mu(mu) { }

  const CRF& crf;
  const vector<Input>& x;
  const vector<double>& lambda;
  const vector<double>& mu;
  double result = 0;
  int count = 0;

  void operator()(const vector<Label>& y) {
    double sum = 0;
    for(int i = 1; i < y.size(); i++) {
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
double crf_probability_of_2(const vector<Label>& y, const vector<Input>& x, CRF& crf, const vector<double>& lambda, const vector<double>& mu) {
  double numer = 0;
  for(int i = 1; i < y.size(); i++)
    numer += lambda[i - 1] * crf.f[i - 1](y, i, x);

  AllSumAccumulator<CRF> filter(crf, x, lambda, mu);
  // TODO: make proper iterator...
  crf.label_alphabet.iterate_sequences(x, filter);

  return numer - std::log(filter.result);
}

template<class CRF>
double crf_probability_of(const vector<Label>& y, const vector<Input>& x, CRF& crf, const vector<double>& lambdas, const vector<double>& mus) {
  double numer = 0;
  for(int i = 1; i < y.size(); i++) {
    for(int lambda = 0; lambda < crf.f.length(); lambda++) {
      auto func = crf.f[lambda];
      auto coef = lambdas[lambda];
      numer += coef * (*func)(y, i, x);
    }
  }

  for(int i = 0; i < y.size(); i++) {
    for(int mu = 0; mu < crf.g.length(); mu++) {
      auto func = crf.g[mu];
      auto coef = mus[mu];
      numer += coef * (*func)(y, i, x);
    }
  }

  double denom = norm_factor(x, crf, lambdas, mus);
  return numer - std::log(denom);
}


template<class CRF>
void max_path(const vector<Input>& x, CRF& crf, const vector<double>& lambda, const vector<double>& mu, vector<int>* max_path) {
  norm_factor(x, crf, lambda, mu, max_path);
}

template<class CRF>
double norm_factor(const vector<Input>& x, CRF& crf, const vector<double>& lambda, const vector<double>& mu) {
  return norm_factor(x, crf, lambda, mu, 0);
}

struct DefaultAggregations {
  // Usually sum, i.e. transition + child
  double concat(double d1, double d2) {
    return d1 + d2;
  }

  // +Inf, since this looks for minimal path
  double infinity() {
    return std::numeric_limits<double>::infinity();
  }

  // Usually the minimum of the two or if calculating
  // normalization factor - log(exp(d1) + exp(d2))
  double aggregate(double d1, double d2) {
    return std::min(d1, d2);
  }

  // Check whether this is infinity - for debugging purposes only
  bool isinf(double d) {
    return std::isinf(d);
  }

  // Return an iterator to the value of the best next child
  double* pick_best(double* it_start, double* it_end) {
    return std::min_element(it_start, it_end);
  }

  // Value of a transition from a last state, corresponding to a position
  // in the input, 1 by def.
  double empty() {
    return 1;
  }

  // Initialization value for aggregation, e.g. 0 for sum
  // or 1 for multiplication
  double init() {
    return 0;
  }
};

template<class CRF, class Functions=DefaultAggregations>
struct FunctionalAutomaton;

template<class CRF, class Functions=DefaultAggregations>
double norm_factor(const vector<Input>& x, CRF& crf, const vector<double>& lambda, const vector<double>& mu, vector<int>* max_path) {
  FunctionalAutomaton<CRF> a(crf.label_alphabet);
  a.lambda = lambda;
  a.mu = mu;
  a.f = crf.f;
  a.g = crf.g;
  a.x = x;

  return a.norm_factor(max_path);
}

template<class CRF, class Functions>
struct FunctionalAutomaton {
  FunctionalAutomaton(const typename CRF::Alphabet &alphabet): alphabet(alphabet) { }

  Functions funcs;
  const typename CRF::Alphabet &alphabet;
  vector<typename CRF::StateFunction*> g;
  vector<double> mu;
  vector<typename CRF::TransitionFunction*> f;
  vector<double> lambda;
  vector<Input> x;

  int alphabet_length() {
    return alphabet.size();
  }

  bool allowedState(int y, int x) {
    return alphabet.allowedState(y, x);
  }

  int child_index(int pos) {
    return 1 + (pos * alphabet_length());
  }

  template<bool includeState, bool includeTransition>
  double calculate_value(int src, int dest, int pos) {
    double result = 0;
    if(includeTransition) {
      for (unsigned i = 0; i < lambda.size(); i++) {
        auto* func = f[i];
        double coef = lambda[i];
        result += coef * (*func)(src, dest, pos, x);
      }
    }

    if(includeState) {
      for(unsigned i = 0; i < g.size(); i++) {
        auto* func = g[i];
        double coef = mu[i];
        result += coef * (*func)(dest, pos, x);
      }
    }
    return result;
  }

  template<bool includeState, bool includeTransition>
  void populate_transitions(double *tr_values, int src, int pos) {
    for(int dest = alphabet_length() - 1; dest >= 0; dest--) {
      double tr_value = calculate_value<includeState, includeTransition>(src, dest, pos);
      tr_values[dest] = tr_value;
    }
  }

  void aggregate_values(double* aggregate_destination, double* transition_values, double* child_values, int pos) {
    double &agg = *aggregate_destination;
    agg = funcs.init();
    for(int dest = alphabet_length() - 1; dest >= 0; dest--) {
      double increment = funcs.concat(transition_values[dest], child_values[dest]);
      agg = funcs.aggregate(increment, agg);
      transition_values[dest] = increment;
    }
  }

  double norm_factor(vector<int>* max_path) {
    int autom_size = alphabet_length() * x.size() + 1;
    double* table = new double[autom_size];
    // Will need for intermediate computations
    double* tr_values = new double[alphabet_length()];
    // Transitions q,i,y -> f
    int index = autom_size - 1;
    int pos = x.size() - 1;

    vector<int>* paths = 0;
    if(max_path) {
      paths = new vector<int>[alphabet_length()];
    }

    // transitions to final state
    for(int dest = alphabet_length() - 1; dest >= 0; dest--, index--) {
      // value of the last "column" of states
      table[index] = allowedState(dest, x[pos]) ? funcs.empty() : funcs.infinity();
    }

    // backwards, for every zero-based position in
    // the input sequence except the last one...
    for(pos--; pos >= 0; pos--) {
      // for every possible label...
      for(int src = alphabet_length() - 1; src >= 0; src--, index--) {
        // Short-circuit if different phoneme types
        if(allowedState(src, x[pos])) {
          // Obtain transition values to all children
          populate_transitions<true, true>(tr_values, src, pos);
          aggregate_values(table + index, tr_values, table + child_index(pos + 1), pos);

          // This, however, should never be 0
          /*if(table[index] == 0 || funcs.isinf(table[index])) {
             std::cerr << table[index] << " at " << index << std::endl;
          }*/

          if(max_path) {
            double* max = funcs.pick_best(tr_values, tr_values + alphabet_length());
            paths[src].push_back(max - tr_values);
          }

        } else {
          table[index] = funcs.infinity();

          if(max_path) {
            paths[src].push_back(-1);
          }
        }
      }
    }

    populate_transitions<true, false>(tr_values, 0, pos);
    aggregate_values(table + index, tr_values, table + child_index(pos + 1), pos);

    if(max_path) {
      double* max = funcs.pick_best(tr_values, tr_values + alphabet_length());
      int state = max - tr_values;
      max_path->push_back(state);

      for(int i = x.size() - 2; i >= 0; i--) {
        state = paths[state][i];
        max_path->push_back(state);
      }
      delete[] paths;
    }

    double denom = table[0];
    delete[] table;
    return denom;
  }
};

#endif
