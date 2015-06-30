#ifndef __CRF_H__
#define __CRF_H__

#include<cstring>
#include<iostream>
#include<algorithm>

#include"alphabet.hpp"
#include"types.hpp"
#include"matrix.hpp"
#include"dot/dot.hpp"

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

template<class _LabelAlphabet, class _Input, class _Features>
class CRandomField {
public:
  typedef _LabelAlphabet Alphabet;
  typedef typename Alphabet::Label Label;
  typedef _Input Input;
  typedef Corpus<Input, Label> TrainingCorpus;

  typedef cost (*EdgeFeature)(const Label&, const Label&, const int, const vector<Input>&);
  typedef cost (*VertexFeature)(const Label&, int, const vector<Input>&);

  CRandomField(): mu(sizeof(features.g) / sizeof(features.g[0])),
                  lambda(sizeof(features.f) / sizeof(features.f[0]))
  { };

  probability probability_of(const vector<Label>& y, const vector<Input>& x) const {
    return crf_probability_of(y, x, *this, lambda, mu);
  };

  probability probability_of(const vector<Label>& y, const vector<Input>& x, const vector<coefficient>& lambdas, const vector<coefficient>& mus) {
    probability numer = 0;
    for(unsigned i = 1; i < y.size(); i++) {
      for(unsigned lambda = 0; lambda < this->lambda.size(); lambda++) {
        auto func = features.f[lambda];
        auto coef = lambdas[lambda];
        numer += coef * (*func)(y, i, x);
      }
    }

    for(unsigned i = 0; i < y.size(); i++) {
      for(unsigned mu = 0; mu < this->mu.size(); mu++) {
        auto func = features.g[mu];
        auto coef = mus[mu];
        numer += coef * (*func)(y, i, x);
      }
    }

    probability denom = norm_factor(x, *this, lambdas, mus);
    return numer - std::log(denom);
  };

  _Features features;

  // Vertex feature coefficients
  vector<coefficient> mu;

  // Edge feature coefficients
  vector<coefficient> lambda;

  _LabelAlphabet& alphabet() const { return *label_alphabet; };

  _LabelAlphabet *label_alphabet;
};


template<class CRF>
struct FunctionalAutomaton;

template<class CRF>
cost traverse_automaton(const vector<typename CRF::Input>& x, CRF& crf, const vector<coefficient>& lambda, const vector<coefficient>& mu, vector<int>* max_path) {
  FunctionalAutomaton<CRF> a(crf);
  a.lambda = lambda;
  a.mu = mu;
  a.x = x;

  return a.traverse(max_path);
}

template<class CRF>
struct FunctionalAutomaton {
  FunctionalAutomaton(const CRF& crf): crf(crf), alphabet(crf.alphabet()), mu(crf.mu), lambda(crf.lambda) { }

  struct Transition {
    Transition() { }
    Transition(int child, cost value):
      base_value(value), child(child) { }

    void set(int child, cost value) {
      this->child = child;
      this->base_value = value;
    }

    cost base_value;
    int child;
  };

  struct MinPathFindFunctions {
    // Usually sum, i.e. transition + child
    cost concat(cost d1, cost d2) {
      return d1 + d2;
    }

    // Usually the minimum of the two or if calculating
    // normalization factor - log(exp(d1) + exp(d2))
    cost aggregate(cost d1, cost d2) {
      return std::min(d1, d2);
    }

    bool is_better(cost t1, cost t2) {
      return t1 < t2;
    }

    // Value of a transition from a last state, corresponding to a position
    // in the input, 1 by def.
    cost empty() {
      return 0;
    }
  };

  MinPathFindFunctions funcs;
  const CRF& crf;
  const typename CRF::Alphabet &alphabet;
  vector<coefficient> mu;
  vector<coefficient> lambda;
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

  cost concat_cost(const typename CRF::Label& src, const typename CRF::Label& dest, int pos) {
    return calculate_value<false, true>(src, dest, pos);
  }

  cost total_cost(const typename CRF::Label& src, const typename CRF::Label& dest, int pos) {
    return calculate_value<true, true>(src, dest, pos);
  }

  cost state_cost(const typename CRF::Label& src, const typename CRF::Label& dest, int pos) {
    return calculate_value<true, false>(src, dest, pos);
  }

  template<bool includeState, bool includeTransition>
  cost calculate_value(const typename CRF::Label& src, const typename CRF::Label& dest, int pos) {
    cost result = 0;
    if(includeTransition) {
      for (unsigned i = 0; i < lambda.size(); i++) {
        auto* func = crf.features.f[i];
        cost coef = lambda[i];
        result += coef * (*func)(src, dest, pos, x);
      }
    }

    if(includeState) {
      for(unsigned i = 0; i < mu.size(); i++) {
        auto* func = crf.features.g[i];
        cost coef = mu[i];
        result += coef * (*func)(dest, pos, x);
      }
    }
    return result;
  }

  template<bool includeState, bool includeTransition>
  cost traverse_transitions(const Array<Transition> children, const typename CRF::Label& src, int pos, unsigned& max_child) {
    unsigned m = 0;

    auto child = alphabet.fromInt(children[m].child);
    cost transition = calculate_value<includeState, includeTransition>(src, child, pos);
    cost child_value = funcs.concat(children[m].base_value, transition);
    max_child = children[m].child;
    cost prev_value = child_value;

    cost agg;
    agg = child_value;
    for(++m; m < children.length; ++m) {
      child = alphabet.fromInt(children[m].child);
      transition = calculate_value<includeState, includeTransition>(src, child, pos);
      child_value = funcs.concat(children[m].base_value, transition);
      if(funcs.is_better(child_value, prev_value)) {
        max_child = children[m].child;
        prev_value = child_value;
      }

      agg = funcs.aggregate(child_value, agg);
    }

    return agg;
  }

  cost traverse(vector<int>* max_path_ptr) {
    Progress prog(x.size());

    vector<int> max_path;
    // Will need for intermediate computations
    Array<Transition> children, next_children;
    children.init(alphabet_length());
    children.length = 0;
    next_children.init(alphabet_length());
    next_children.length = 0;

    int pos = x.size() - 1;

    Matrix<unsigned> paths(alphabet_length(), x.size());
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

      next_children.length = allowed.size();
      //      #pragma omp parallel for
      for(unsigned m = 0; m < allowed.size(); m++) {
        auto srcId = allowed[m];

        const typename CRF::Label& src = alphabet.fromInt(srcId);
        unsigned max_child;
        cost value = traverse_transitions<true, true>(children, src, pos + 1, max_child);
        next_children[m].set(srcId, value);

        paths(srcId, pos) = max_child;
      }

      children.length = 0;
      std::swap(children, next_children);
      prog.update();
    }

    unsigned max_child;
    cost value = traverse_transitions<true, false>(children, alphabet.fromInt(0), 0, max_child);
    prog.finish();

    max_path.push_back(max_child);
    for(unsigned i = 0; i <= x.size() - 2; i++) {
      max_child = paths(max_child, i);
      max_path.push_back(max_child);
    }

    if(max_path_ptr)
      *max_path_ptr = max_path;

    children.destroy();
    next_children.destroy();

    return value;
  }
};

template<class CRF>
cost max_path(const vector<typename CRF::Input>& x, CRF& crf, const vector<coefficient>& lambda, const vector<coefficient>& mu, vector<int>* max_path) {
  return traverse_automaton(x, crf, lambda, mu, max_path);
}

template<class CRF>
cost concat_cost(const vector<typename CRF::Label>& y, CRF& crf, const vector<coefficient>& lambda, const vector<coefficient>& mu, const vector<typename CRF::Input>& inputs) {
  FunctionalAutomaton<CRF> a(crf);
  a.lambda = lambda;
  a.mu = mu;
  a.x = inputs;

  cost result = 0;
  for(unsigned i = 1; i < y.size(); i++)
    result += a.concat_cost(y[i-1], y[i], i);
  return result;
}

template<class CRF>
cost state_cost(const vector<typename CRF::Label>& y, CRF& crf, const vector<cost>& lambda, const vector<cost>& mu, const vector<typename CRF::Input>& inputs) {
  FunctionalAutomaton<CRF> a(crf);
  a.lambda = lambda;
  a.mu = mu;
  a.x = inputs;

  cost result = 0;
  for(unsigned i = 0; i < y.size(); i++)
    result += a.state_cost(y[i], y[i], i);

  return result;
}

template<class CRF>
cost total_cost(const vector<typename CRF::Label>& y, CRF& crf, const vector<cost>& lambda, const vector<cost>& mu, const vector<typename CRF::Input>& inputs) {
  FunctionalAutomaton<CRF> a(crf);
  a.lambda = lambda;
  a.mu = mu;
  a.x = inputs;

  cost result = 0;
  for(unsigned i = 1; i < y.size(); i++) {
    result += a.total_cost(y[i-1], y[i], i);
  }
  result += a.state_cost(y[0], y[0], 0);
  return result;
}

struct NormFactorFunctions {
  // Usually sum, i.e. transition + child
  cost concat(cost d1, cost d2) {
    return d1 + d2;
  }

  // Usually the minimum of the two or if calculating
  // normalization factor - log(exp(d1) + exp(d2))
  cost aggregate(cost d1, cost d2) {
    return util::log_sum(d1, d2);
  }

  // Value of a transition from a last state, corresponding to a position
  // in the input, 1 by def.
  cost empty() {
    return 1;
  }
};

template<class CRF>
cost norm_factor(const vector<typename CRF::Input>& x, CRF& crf, const vector<cost>& lambda, const vector<cost>& mu) {
  return traverse_automaton<CRF, NormFactorFunctions>(x, crf, lambda, mu, 0);
}

#endif
