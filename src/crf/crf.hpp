#ifndef __CRF_H__
#define __CRF_H__

#include<cstring>
#include<iostream>
#include<algorithm>

#include"alphabet.hpp"
#include"../dot/dot.hpp"

using std::vector;

class Corpus {
public:
  const vector<Label>& label(int i) {
    return labels[i];
  };

  const vector<Input>& input(int i) {
    return inputs[i];
  };

  unsigned size() const {
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
  for(unsigned i = 1; i < y.size(); i++) {
    for(unsigned lambda = 0; lambda < crf.f.size(); lambda++) {
      auto func = crf.f[lambda];
      auto coef = lambdas[lambda];
      numer += coef * (*func)(y, i, x);
    }
  }

  for(unsigned i = 0; i < y.size(); i++) {
    for(unsigned mu = 0; mu < crf.g.size(); mu++) {
      auto func = crf.g[mu];
      auto coef = mus[mu];
      numer += coef * (*func)(y, i, x);
    }
  }

  double denom = norm_factor(x, crf, lambdas, mus);
  return numer - std::log(denom);
}

struct Transition {
  Transition(int child, double value):
    value(value), child(child) { }

  double value;
  int child;
};

struct MinPathFindFunctions {
  // Usually sum, i.e. transition + child
  double concat(double d1, double d2) {
    return d1 + d2;
  }

  // Usually the minimum of the two or if calculating
  // normalization factor - log(exp(d1) + exp(d2))
  double aggregate(double d1, double d2) {
    return std::min(d1, d2);
  }

  // Return an iterator to the value of the best next child
  vector<Transition>::const_iterator pick_best(vector<Transition>& children) {
    return std::min_element(children.begin(), children.end(), compare);
  }

  static bool compare(const Transition& t1, const Transition& t2) {
    return t1.value < t2.value;
  }

  // Value of a transition from a last state, corresponding to a position
  // in the input, 1 by def.
  double empty() {
    return 0;
  }
};

template<class CRF, class Functions=MinPathFindFunctions>
struct FunctionalAutomaton;

template<class CRF, class Functions=MinPathFindFunctions>
double traverse_automaton(const vector<Input>& x, CRF& crf, const vector<double>& lambda, const vector<double>& mu, vector<int>* max_path) {
  FunctionalAutomaton<CRF> a(crf.label_alphabet);
  a.lambda = lambda;
  a.mu = mu;
  a.f = crf.f;
  a.g = crf.g;
  a.x = x;

  return a.traverse(max_path);
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
    return 1 + pos * alphabet_length();
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
  void populate_transitions(vector<Transition>* children, int src, int pos) {
    for(auto it = children->begin(); it != children->end(); it++) {
      double transition = calculate_value<includeState, includeTransition>(src, (*it).child, pos);
      (*it).value = funcs.concat((*it).value, transition);
    }
  }

  void aggregate_values(double* aggregate_destination, vector<Transition>* children) {
    double &agg = *aggregate_destination;
    auto it = children->begin();
    agg = (*it).value;
    for(++it; it != children->end(); it++) {
      agg = funcs.aggregate((*it).value, agg);
    }
  }

  double traverse(vector<int>* max_path) {
    // Will need for intermediate computations
    vector<Transition>* children = new vector<Transition>();
    vector<Transition>* next_children = new vector<Transition>();

    int pos = x.size() - 1;

    vector<int>* paths = 0;
    if(max_path)
      paths = new vector<int>[alphabet_length()];

    // transitions to final state
    // value of the last "column" of states
    for(int dest = alphabet_length() - 1; dest >= 0; dest--) {
      if(allowedState(dest, x[pos]))
        children->push_back(Transition(dest, funcs.empty()));
    }

    // backwards, for every zero-based position in
    // the input sequence except the last one...
    for(pos--; pos >= 0; pos--) {
      // for every possible label...
      for(int src = alphabet_length() - 1; src >= 0; src--) {
        // Short-circuit if different phoneme types
        vector<Transition> temp_children = *children;
        if(allowedState(src, x[pos])) {
          // Obtain transition values to all children
          populate_transitions<true, true>(&temp_children, src, pos + 1);
          double value;
          aggregate_values(&value, &temp_children);

          next_children->push_back(Transition(src, value));
        }

        if(max_path) {
          auto max = funcs.pick_best(temp_children);
          paths[src].push_back((*max).child);
        }
      }

      children->clear();
      std::swap(children, next_children);
    }

    populate_transitions<true, false>(children, 0, 0);
    double value;
    aggregate_values(&value, children);

    if(max_path) {
      auto max = funcs.pick_best(*children);
      max_path->push_back((*max).child);

      int state = (*max).child;
      for(int i = x.size() - 2; i >= 0; i--) {
        state = paths[state][i];
        max_path->push_back(state);
      }
      delete[] paths;
    }

    return value;
  }
};

template<class CRF>
void max_path(const vector<Input>& x, CRF& crf, const vector<double>& lambda, const vector<double>& mu, vector<int>* max_path) {
  traverse_automaton(x, crf, lambda, mu, max_path);
}

struct NormFactorFunctions {
  // Usually sum, i.e. transition + child
  double concat(double d1, double d2) {
    return d1 + d2;
  }

  // Usually the minimum of the two or if calculating
  // normalization factor - log(exp(d1) + exp(d2))
  double aggregate(double d1, double d2) {
    return util::log_sum(d1, d2);
  }

  // Value of a transition from a last state, corresponding to a position
  // in the input, 1 by def.
  double empty() {
    return 1;
  }
};

template<class CRF>
double norm_factor(const vector<Input>& x, CRF& crf, const vector<double>& lambda, const vector<double>& mu) {
  return traverse_automaton<CRF, NormFactorFunctions>(x, crf, lambda, mu, 0);
}

#endif
