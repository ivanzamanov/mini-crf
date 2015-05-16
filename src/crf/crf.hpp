#ifndef __CRF_H__
#define __CRF_H__

#include<cstring>
#include<iostream>
#include<algorithm>

#include"alphabet.hpp"
#include"matrix.hpp"
#include"../dot/dot.hpp"

using std::vector;

const double TRAINING_RATIO = 0.3;

template<class Input, class Label>
class Corpus {
public:
  vector<Label>& label(int i) {
    return labels[i];
  };

  vector<Input>& input(int i) {
    return inputs[i];
  };

  unsigned size() const {
    return inputs.size();
  };

  void add(vector<Input>& input, vector<Label>& labels) {
    this->inputs.push_back(input);
    this->labels.push_back(labels);
  };

  Corpus<Input, Label> training_part() {
    Corpus<Input, Label> result;
    unsigned count = size() / TRAINING_RATIO;
    for(unsigned i = 0; i < count; i++)
      result.add(inputs[i], labels[i]);
    return result;
  };

  Corpus<Input, Label> testing_part() {
    Corpus<Input, Label> result;
    unsigned count = size() / TRAINING_RATIO;
    for(unsigned i = count; i < size(); i++)
      result.add(inputs[i], labels[i]);
    return result;
  };

private:
  vector<vector<Input> > inputs;
  vector<vector<Label> > labels;
};

template<class LabelAlphabet, class InputT>
class CRandomField {
public:
  typedef LabelAlphabet Alphabet;
  typedef typename Alphabet::Label Label;
  typedef InputT Input;
  typedef Corpus<Input, Label> TrainingCorpus;

  class BaseFunction {
  public:
    LabelAlphabet* alphabet;
    virtual ~BaseFunction() { };
  };

  class StateFunction : public BaseFunction {
  public:
    virtual double operator()(const Label&, int, const vector<Input>&) const = 0;
  };

  class TransitionFunction : public BaseFunction {
  public:
    unsigned short id;
    virtual double operator()(const Label&, const Label&, const int, const vector<Input>&) const = 0;
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

  double probability_of(const vector<Label>& y, const vector<Input>& x) const {
    return crf_probability_of(y, x, *this, lambda, mu);
  };

  double probability_of(const vector<Label>& y, const vector<Input>& x, const vector<double>& lambdas, const vector<double>& mus) {
    double numer = 0;
    for(unsigned i = 1; i < y.size(); i++) {
      for(unsigned lambda = 0; lambda < f.size(); lambda++) {
        auto func = f[lambda];
        auto coef = lambdas[lambda];
        numer += coef * (*func)(y, i, x);
      }
    }

    for(unsigned i = 0; i < y.size(); i++) {
      for(unsigned mu = 0; mu < g.size(); mu++) {
        auto func = g[mu];
        auto coef = mus[mu];
        numer += coef * (*func)(y, i, x);
      }
    }

    double denom = norm_factor(x, *this, lambdas, mus);
    return numer - std::log(denom);
  };

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
struct FunctionalAutomaton;

template<class CRF>
double traverse_automaton(const vector<typename CRF::Input>& x, CRF& crf, const vector<double>& lambda, const vector<double>& mu, vector<int>* max_path) {
  FunctionalAutomaton<CRF> a(crf.label_alphabet);
  a.lambda = lambda;
  a.mu = mu;
  a.f = crf.f;
  a.g = crf.g;
  a.x = x;

  return a.traverse(max_path);
}

template<class CRF>
struct FunctionalAutomaton {
  FunctionalAutomaton(const typename CRF::Alphabet &alphabet): alphabet(alphabet) { }
  FunctionalAutomaton(const CRF& crf): alphabet(crf.label_alphabet), g(crf.g), mu(crf.mu), f(crf.f), lambda(crf.lambda) { }

  struct Transition {
    Transition() { }
    Transition(int child, double value):
      base_value(value), child(child) { }

    void set(int child, double value) {
      this->child = child;
      this->base_value = value;
    }

    double base_value;
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
    
    bool is_better(double t1, double t2) {
      return t1 < t2;
    }

    // Value of a transition from a last state, corresponding to a position
    // in the input, 1 by def.
    double empty() {
      return 0;
    }
  };

  MinPathFindFunctions funcs;
  const typename CRF::Alphabet &alphabet;
  vector<typename CRF::StateFunction*> g;
  vector<double> mu;
  vector<typename CRF::TransitionFunction*> f;
  vector<double> lambda;
  vector<typename CRF::Input> x;

  int alphabet_length() {
    return alphabet.size();
  }

  bool allowedState(const typename CRF::Label& y, typename CRF::Input& x) {
    return alphabet.allowedState(y, x);
  }

  int child_index(int pos) {
    return 1 + pos * alphabet_length();
  }

  double concat_cost(const typename CRF::Label& src, const typename CRF::Label& dest, int pos) {
    return calculate_value<false, true>(src, dest, pos);
  }

  double total_cost(const typename CRF::Label& src, const typename CRF::Label& dest, int pos) {
    return calculate_value<true, true>(src, dest, pos);
  }

  double state_cost(const typename CRF::Label& src, const typename CRF::Label& dest, int pos) {
    return calculate_value<true, false>(src, dest, pos);
  }

  template<bool includeState, bool includeTransition>
  double calculate_value(const typename CRF::Label& src, const typename CRF::Label& dest, int pos) {
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
  double traverse_transitions(Array<Transition> children, const typename CRF::Label& src, int pos, unsigned& max_child) {
    unsigned m = 0;
    max_child = m;

    auto child = alphabet.fromInt(children[m].child);
    double transition = calculate_value<includeState, includeTransition>(src, child, pos);
    children[m].value = funcs.concat(children[m].base_value, transition);
    double prev_value = children[m].value;

    double agg;
    agg = children[m].value;
    for(++m; m < children.length; ++m) {
      child = alphabet.fromInt(children[m].child);
      transition = calculate_value<includeState, includeTransition>(src, child, pos);
      double child_value = funcs.concat(children[m].base_value, transition); 
      if(funcs.is_better(prev_value, child_value)) {
        max_child = m;
        prev_value = child_value;
      }
      
      children[m].value = child_value;
      agg = funcs.aggregate(children[m].value, agg);
    }

    return agg;
  }

  double traverse(vector<int>* max_path_ptr) {
    Progress prog(x.size());

    vector<int> max_path;
    // Will need for intermediate computations
    Array<Transition> children, next_children;
    children.init(alphabet_length());
    children.length = 0;
    next_children.init(alphabet_length());
    next_children.length = 0;

    int pos = x.size() - 1;

    Matrix paths(alphabet_length(), x.size());
    vector<unsigned> options;

    // transitions to final state
    // value of the last "column" of states
    const typename CRF::Alphabet::LabelClass& allowed = alphabet.get_class(x[pos]);
    options.push_back(allowed.size());
    for(auto it = allowed.begin(); it != allowed.end() ; it++) {
      auto dest = *it;
      children[children.length++].set(dest, funcs.empty());
    }
    prog.update();

    // backwards, for every zero-based position in
    // the input sequence except the last one...
    for(pos--; pos >= 0; pos--) {
      // for every possible label...
      const typename CRF::Alphabet::LabelClass& allowed = alphabet.get_class(x[pos]);
      options.push_back(allowed.size());
      for(unsigned m = 0; m < allowed.size(); m++) {
        auto srcId = allowed[m];

        const typename CRF::Label& src = alphabet.fromInt(srcId);
        unsigned max_child;
        double value = traverse_transitions<true, true>(children, src, pos + 1, max_child);
        next_children[next_children.length++].set(srcId, value);

        paths(srcId, pos) = max_child;
      }

      children.length = 0;
      std::swap(children, next_children);
      prog.update();
    }

    unsigned max_child;
    double value = traverse_transitions<true, false>(children, alphabet.fromInt(0), 0, max_child);
    prog.finish();

    max_path.push_back(max_child);
    for(unsigned i = 0; i <= x.size() - 2; i++) {
      max_child = paths(max_child, i);
      max_path.push_back(max_child);
    }

    if(max_path_ptr)
      *max_path_ptr = max_path;
    return value;
  }
};

template<class CRF>
double max_path(const vector<typename CRF::Input>& x, CRF& crf, const vector<double>& lambda, const vector<double>& mu, vector<int>* max_path) {
  return traverse_automaton(x, crf, lambda, mu, max_path);
}

template<class CRF>
double concat_cost(const vector<typename CRF::Label>& y, CRF& crf, const vector<double>& lambda, const vector<double>& mu, const vector<typename CRF::Input>& inputs) {
  FunctionalAutomaton<CRF> a(crf.label_alphabet);
  a.lambda = lambda;
  a.mu = mu;
  a.f = crf.f;
  a.g = crf.g;
  a.x = inputs;

  double result = 0;
  for(unsigned i = 1; i < y.size(); i++) {
    result += a.concat_cost(y[i-1], y[i], i);
  }
  return result;
}

template<class CRF>
double state_cost(const vector<typename CRF::Label>& y, CRF& crf, const vector<double>& lambda, const vector<double>& mu, const vector<typename CRF::Input>& inputs) {
  FunctionalAutomaton<CRF> a(crf.label_alphabet);
  a.lambda = lambda;
  a.mu = mu;
  a.f = crf.f;
  a.g = crf.g;
  a.x = inputs;

  double result = 0;
  for(unsigned i = 0; i < y.size(); i++) {
    result += a.state_cost(y[i], y[i], i);
  }
  return result;
}

template<class CRF>
double total_cost(const vector<typename CRF::Label>& y, CRF& crf, const vector<double>& lambda, const vector<double>& mu, const vector<typename CRF::Input>& inputs) {
  FunctionalAutomaton<CRF> a(crf.label_alphabet);
  a.lambda = lambda;
  a.mu = mu;
  a.f = crf.f;
  a.g = crf.g;
  a.x = inputs;

  double result = 0;
  for(unsigned i = 1; i < y.size(); i++) {
    result += a.total_cost(y[i-1], y[i], i);
  }
  result += a.state_cost(y[0], y[0], 0);
  return result;
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
double norm_factor(const vector<typename CRF::Input>& x, CRF& crf, const vector<double>& lambda, const vector<double>& mu) {
  return traverse_automaton<CRF, NormFactorFunctions>(x, crf, lambda, mu, 0);
}

#endif
