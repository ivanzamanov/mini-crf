#ifndef __CRF_H__
#define __CRF_H__

#include<algorithm>
#include<cassert>
#include<array>
#include<utility>

#include"alphabet.hpp"
#include"automaton-functions.hpp"

#include"types.hpp"

#include"matrix.hpp"

using std::vector;
using namespace util;

template<class _LabelAlphabet, class _Input, class _Features>
class CRandomField;

template<class Input, class Label>
class _Corpus;

template<class CRF, class Functions>
struct FunctionalAutomaton;

const double TRAINING_RATIO = 0.3;

template<class Input, class Label>
class _Corpus {
public:
  void set_max_size(int max_size) {
    this->max_size = max_size;
  }

  vector<Label>& label(int i) {
    return labels[i];
  };

  vector<Input>& input(int i) {
    return inputs[i];
  };

  unsigned size() const {
    if(max_size > 0)
      return max_size;
    return inputs.size();
  };

  void add(vector<Input>& input, vector<Label>& labels) {
    this->inputs.push_back(input);
    this->labels.push_back(labels);
  };

  _Corpus<Input, Label> training_part() {
    _Corpus<Input, Label> result;
    unsigned count = size() / TRAINING_RATIO;
    for(unsigned i = 0; i < count; i++)
      result.add(inputs[i], labels[i]);
    return result;
  };

  _Corpus<Input, Label> testing_part() {
    _Corpus<Input, Label> result;
    unsigned count = size() / TRAINING_RATIO;
    for(unsigned i = count; i < size(); i++)
      result.add(inputs[i], labels[i]);
    return result;
  };

private:
  int max_size = -1;
  vector<vector<Input> > inputs;
  vector<vector<Label> > labels;
};

template<unsigned size>
struct FeatureStats : public std::array<cost, size> {
  FeatureStats(): std::array<cost, size>() {
    for(unsigned i = 0; i < size; i++)
      (*this)[i] = 0;
  }
};

template<unsigned size>
struct PathFeatureStats : public std::vector<FeatureStats<size>> {
  FeatureStats<size>& get(unsigned pos) {
    std::vector<FeatureStats<size>>::resize(std::max(pos + 1, size));
    return (*this)[pos];
  }
};

template<class _LabelAlphabet,
         class _Input,
         class _Features>
class CRandomField {
public:
  typedef _LabelAlphabet Alphabet;
  typedef typename Alphabet::Label Label;
  typedef _Input Input;
  typedef _Corpus<Input, Label> TrainingCorpus;
  typedef _Features features;
  typedef cost (*FeatureFunction)(const Input&, const Label&, const Label&);
  typedef std::array<coefficient, features::size> Values;

  typedef PathFeatureStats<features::size> Stats;

  void set(const std::string& feature, coefficient coef) {
    int index = -1;
    for(unsigned i = 0; i < features::Names.size(); i++)
      if(features::Names[i] == feature)
        index = i;
    assert(index >= 0);
    lambda[index] = coef;
  }

  Values lambda;

  _LabelAlphabet& alphabet() const { return *label_alphabet; };
  _LabelAlphabet *label_alphabet;
};

template<class Functions,
         class CRF,
         unsigned kBest = 1>
std::array<cost, kBest> traverse_automaton(const vector<typename CRF::Input>& x,
                                           CRF& crf,
                                           const typename CRF::Values& lambda,
                                           vector<int>* best_path) {
  FunctionalAutomaton<CRF, Functions> a(crf);
  a.lambda = lambda;
  a.x = x;

  return a.template traverse<kBest>(best_path);
}

template<bool flag>
struct ConditionalInvoke {
  template<class Func, class V1, class V2, class V3>
  auto operator()(Func f, bool flag2, const V1&, const V2& p2, const V3& p3) {
    if(flag2)
      return f(p2, p3);
    return 0.0f;
  }
};

template<>
struct ConditionalInvoke<true> {
  template<class Func, class V1, class V2, class V3>
  auto operator()(Func f, bool flag2, const V1& state, const V2& prev, const V3& next) {
    if(flag2)
      return f(state, prev);
    else
      return f(state, next);
  }
};

template<class CRF, class Functions>
struct FunctionalAutomaton {
  FunctionalAutomaton(const CRF& crf): crf(crf),
                                       alphabet(crf.alphabet()),
                                       lambda(crf.lambda) { }

  Functions funcs;
  const CRF& crf;
  const typename CRF::Alphabet &alphabet;
  typename CRF::Values lambda;
  vector<typename CRF::Input> x;

  int alphabet_length() {
    return alphabet.size();
  }

  cost calculate_value(const int srcOrState,
                       const int destId,
                       const int pos,
                       bool isTransition,
                       typename CRF::Stats* stats = 0) {
    return calculate_value(alphabet.fromInt(srcOrState),
                           alphabet.fromInt(destId),
                           pos, isTransition,
                           stats);
  }

  cost calculate_value(const typename CRF::Label& srcOrState,
                       const typename CRF::Label& dest,
                       const int pos,
                       bool isTransition,
                       typename CRF::Stats* stats = 0) {
    typename CRF::Values vals;
    // TODO: Could move up
    const auto f = [&](auto func) {
      // either we're not calculating the last position
      // or this is a state function
      return ConditionalInvoke<decltype(func)::is_state>{}(func, isTransition,
                                                           x[pos], srcOrState, dest);
    };
    tuples::Invoke<CRF::features::size,
                   decltype(CRF::features::Functions)>{}(vals, f);

    cost result = 0;
    for(unsigned i = 0; i < vals.size(); i++)
      result += vals[i] * lambda[i];

    if(stats) {
      for(unsigned i = 0; i < vals.size(); i++)
        stats->get(pos)[i] = vals[i] * lambda[i];
    }

    return result;
  }

  template<class TrArray, unsigned kBest>
  std::array<Transition, kBest>
  traverse_transitions(const TrArray* const children,
                       unsigned children_length,
                       int src,
                       const int pos) {
    auto cmp = [&](const Transition& tr1, const Transition& tr2) {
      return funcs.is_better(tr1.base_value, tr2.base_value);
    };
    // Holds 2 best paths
    pqueue<Transition, kBest> pq;
    for(auto& tr : pq)
      tr.set(0, funcs.worst());

    const bool isTransition = (x.size() > 1) && (pos != x.size() - 1);
    if(isTransition) {
      for(unsigned m = 0; m < children_length; m++) {
        auto& currentTr = children[m];
        for(auto& tr : currentTr) {
          // value of transition to that label
          auto transitionValue = calculate_value(src, tr.child, pos, true);
          // concatenation of the cost of the target label
          // and the transition value
          auto child_value = funcs.concat(tr.base_value, transitionValue);
          pq.push(Transition::make(tr.child, child_value), cmp);
        }
      }
    } else {
      auto transitionValue = calculate_value(src, src, pos, false);
      // concatenation of the cost of the target label
      // and the transition value
      auto child_value = funcs.concat(transitionValue, funcs.empty());
      pq.push(Transition::make(src, child_value), cmp);
    }
    return pq;
  }

  template<class TrArray, unsigned kBestValues>
  TrArray
  traverse_at_position(const typename CRF::Alphabet::LabelClass& allowed,
                       TrArray* children,
                       TrArray* next_children,
                       int children_length,
                       int pos,
                       Matrix<unsigned> &paths) {
    auto cmp = [&](const Transition& tr1, const Transition& tr2) {
      return funcs.is_better(tr1.base_value, tr2.base_value);
    };
    pqueue<Transition, kBestValues> pq;
    for(auto& tr : pq)
      tr.set(0, funcs.worst());
    for(auto m = 0; m < allowed.size(); m++) {
      auto srcId = allowed[m];

      auto t = traverse_transitions<TrArray, kBestValues>(children,
                                                          children_length,
                                                          srcId,
                                                          pos);
      unsigned i = 0;
      paths(srcId, pos) = t[0].child;
      for(auto& tr : t) {
        // The ith best path from srcId
        // passes through tr.child
        next_children[m][i].set(srcId, tr.base_value);
        pq.push(Transition::make(srcId, tr.base_value), cmp);
        i++;
      }
    }
    return pq;
  }

  template<unsigned kBestValues = 1>
  std::array<cost, kBestValues> traverse(vector<int>* best_path) {
    static_assert(kBestValues > 0, "At least 1 best val");
    Progress prog(x.size());

    typedef std::array<Transition, kBestValues> TrArray;
    // Will need for intermediate computations
    TrArray children_a[alphabet_length()];
    TrArray next_children_a[alphabet_length()];

    TrArray* children = children_a,
      *next_children = next_children_a;

    unsigned children_length = 0;
    unsigned next_children_length = 0;

    int pos = x.size() - 1;

    //Matrix<std::array<unsigned, kBestPaths>> paths(alphabet_length(), x.size());
    Matrix<unsigned> paths(alphabet_length(), x.size());
    
    // transitions to final state
    // value of the last "column" of states
    // meaning, if length == 1, then
    // the value will be the state cost
    for(auto id : alphabet.get_class(x[pos])) {
      for(auto& tr : children[children_length])
        tr.set(id, funcs.empty());
      children_length++;
    }
    prog.update();

    // backwards, for every zero-based position in
    // the input sequence except the last one...
    TrArray t;
    for(; pos >= 0; pos--) {
      // for every possible label...
      const auto& allowed = alphabet.get_class(x[pos]);

      next_children_length = allowed.size();
      assert(children_length > 0);

      t = traverse_at_position<TrArray,
                               kBestValues>(allowed, children,
                                            next_children,
                                            children_length, pos,
                                            paths);

      children_length = 0;
      std::swap(children, next_children);
      std::swap(children_length, next_children_length);
      prog.update();
    }
    prog.finish();

    if(best_path) {
      auto& path = *best_path;
      auto child = t[0].child;
      path.push_back(child);
      for(unsigned i = 0; i < x.size() - 1; i++) {
        child = paths(child, i);
        path.push_back(child);
      }
    }

    std::array<cost, kBestValues> result;
    unsigned i = 0;
    for(auto& t : t) {
      result[i] = t.base_value;
      i++;
    }
    return result;
  }
};

template<class CRF>
cost concat_cost(const vector<typename CRF::Label>& y,
                 CRF& crf,
                 const typename CRF::Values& lambda,
                 const vector<typename CRF::Input>& x,
                 typename CRF::Stats* stats = 0) {
  assert(x.size() > 0 && x.size() == y.size());
  FunctionalAutomaton<CRF, MinPathFindFunctions> a(crf);
  a.lambda = lambda;
  a.x = x;

  int i = x.size() - 1;
  cost result = a.calculate_value(y[i], y[i], i, false, stats);
  for(i--; i >= 0; i--)
    result += a.calculate_value(y[i], y[i + 1], i, true, stats);
  return result;
}

#endif
