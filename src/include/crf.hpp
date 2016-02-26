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

template<class Functions, class CRF>
cost traverse_automaton(const vector<typename CRF::Input>& x,
                        CRF& crf,
                        const typename CRF::Values& lambda,
                        vector<int>* best_path=0,
                        vector<int>* s_best_path=0) {
  FunctionalAutomaton<CRF, Functions> a(crf);
  a.lambda = lambda;
  a.x = x;

  return a.traverse(best_path, s_best_path).first;
}

template<class CRF, class Functions>
struct FunctionalAutomaton {
  struct TransitionPair {
    Transition first, second;
  };

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
                       typename CRF::Stats* stats = 0) {
    return calculate_value(alphabet.fromInt(srcOrState), alphabet.fromInt(destId), pos, stats);
  }

  cost calculate_value(const typename CRF::Label& srcOrState,
                       const int destId,
                       const int pos,
                       typename CRF::Stats* stats = 0) {
    return calculate_value(srcOrState, alphabet.fromInt(destId), pos, stats);
  }

  cost calculate_value(const typename CRF::Label& srcOrState,
                       const typename CRF::Label& dest,
                       const int pos,
                       typename CRF::Stats* stats = 0) {
    typename CRF::Values vals;
    // TODO: Could move up
    const bool isTransition = (x.size() > 1) && (pos != x.size() - 1);
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

  std::tuple<cost, unsigned, cost, unsigned>
  traverse_transitions(const TransitionPair* const children,
                       unsigned children_length,
                       int src,
                       const int pos) {
    auto cmp = [&](const Transition& tr1, const Transition& tr2) {
      return funcs.is_better(tr1.base_value, tr2.base_value);
    };
    // Holds 2 best paths
    pqueue<Transition, 2> pq;
    for(auto& tr : pq)
      tr.set(0, funcs.worst());

    auto agg = funcs.empty();
    auto s_agg = funcs.empty();
    for(unsigned m = 0; m < children_length; m++) {
      auto& currentTr = children[m];
      // value of transition to that label
      auto transitionValue = calculate_value(src, currentTr.first.child, pos);
      // concatenation of the cost of the target label
      // and the transition value
      auto child_value = funcs.concat(currentTr.first.base_value, transitionValue);
      pq.push(Transition::make(currentTr.first.child, child_value), cmp);
      agg = funcs.aggregate(child_value, agg);

      // for the second best path...
      transitionValue = calculate_value(src, currentTr.second.child, pos);
      child_value = funcs.concat(currentTr.second.base_value, transitionValue);
      pq.push(Transition::make(currentTr.second.child, child_value), cmp);

      s_agg = funcs.aggregate(child_value, s_agg);
    }
    return std::make_tuple(agg, pq[0].child, s_agg, pq[1].child);
  }

  std::tuple<cost, unsigned, cost, unsigned>
  traverse_at_position(const typename CRF::Alphabet::LabelClass& allowed,
                       TransitionPair* children,
                       TransitionPair* next_children,
                       int children_length,
                       int pos,
                       Matrix<unsigned> &paths,
                       Matrix<unsigned> &paths_2) {
    auto cmp = [&](const Transition& tr1, const Transition& tr2) {
      return funcs.is_better(tr1.base_value, tr2.base_value);
    };
    pqueue<Transition, 2> pq;
    for(auto& tr : pq)
      tr.set(0, funcs.worst());
    for(auto m = 0; m < allowed.size(); m++) {
      auto srcId = allowed[m];

      auto t = traverse_transitions(children,
                                    children_length,
                                    srcId,
                                    pos);
      auto agg = std::get<0>(t);
      auto s_agg = std::get<2>(t);
      auto c = std::get<1>(t);
      auto s_c = std::get<3>(t);
      next_children[m].first.set(srcId, agg);
      next_children[m].second.set(srcId, s_agg);
      paths(srcId, pos) = c;
      paths_2(srcId, pos) = s_c;

      pq.push(Transition::make(srcId, agg), cmp);
      pq.push(Transition::make(srcId, s_agg), cmp);
    }
    return std::make_tuple(pq[0].base_value, pq[0].child, pq[1].base_value, pq[1].child);
  }

  std::pair<cost, cost> traverse(vector<int>* best_path_ptr=0,
                                 vector<int>* s_best_path_ptr=0) {
    Progress prog(x.size());

    // Will need for intermediate computations
    TransitionPair children_a[alphabet_length()];
    TransitionPair next_children_a[alphabet_length()];

    TransitionPair* children = children_a,
      *next_children = next_children_a;

    unsigned children_length = 0;
    unsigned next_children_length = 0;

    int pos = x.size() - 1;

    Matrix<unsigned> paths(alphabet_length(), x.size());
    Matrix<unsigned> paths_2(alphabet_length(), x.size());

    // transitions to final state
    // value of the last "column" of states
    // meaning, if length == 1, then
    // the value will be the state cost
    for(auto id : alphabet.get_class(x[pos])) {
      children[children_length].first.set(id, funcs.empty());
      children[children_length].second.set(id, funcs.worst());
      children_length++;
    }
    prog.update();

    // backwards, for every zero-based position in
    // the input sequence except the last one...
    std::tuple<cost, unsigned, cost, unsigned> t;
    for(; pos >= 0; pos--) {
      // for every possible label...
      const auto& allowed = alphabet.get_class(x[pos]);

      next_children_length = allowed.size();
      assert(children_length > 0);

      t = traverse_at_position(allowed, children,
                               next_children,
                               children_length, pos,
                               paths,
                               paths_2);

      children_length = 0;
      std::swap(children, next_children);
      std::swap(children_length, next_children_length);
      prog.update();
    }
    prog.finish();

    if(best_path_ptr) {
      vector<int>& best_path = *best_path_ptr;
      unsigned best_child = std::get<1>(t);
      best_path.push_back(best_child);
      for(int i = 0; i <= (int) x.size() - 2; i++) {
        best_child = paths(best_child, i);
        best_path.push_back(best_child);
      }
    }

    if(s_best_path_ptr) {
      vector<int>& s_best_path = *s_best_path_ptr;
      unsigned s_best_child = std::get<3>(t);
      s_best_path.push_back(s_best_child);
      for(int i = 0; i <= (int) x.size() - 2; i++) {
        s_best_child = paths_2(s_best_child, i);
        s_best_path.push_back(s_best_child);
      }
    }

    return std::make_pair(std::get<0>(t), std::get<2>(t));
  }
};

template<class CRF>
cost concat_cost(const vector<typename CRF::Label>& y,
                 CRF& crf,
                 const typename CRF::Values& lambda,
                 const vector<typename CRF::Input>& x,
                 typename CRF::Stats* stats = 0) {
  assert(y.size() > 0);
  FunctionalAutomaton<CRF, MinPathFindFunctions> a(crf);
  a.lambda = lambda;
  a.x = x;

  int i = x.size() - 1;
  cost result = a.calculate_value(y[i], y[i], i, stats);
  for(i--; i >= 0; i--)
    result += a.calculate_value(y[i], y[i + 1], i, stats);
  return result;
}

#endif
