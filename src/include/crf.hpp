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
                        vector<int>* max_path) {
  FunctionalAutomaton<CRF, Functions> a(crf);
  a.lambda = lambda;
  a.x = x;

  return a.traverse(max_path);
}

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

  bool allowedState(const typename CRF::Label& y, typename CRF::Input& x) {
    return alphabet.allowedState(y, x);
  }

  int child_index(int pos) {
    return 1 + pos * alphabet_length();
  }

  cost calculate_value(const typename CRF::Label& src,
                       const int destId,
                       const int pos,
                       typename CRF::Stats* stats = 0) {
    return calculate_value(src, alphabet.fromInt(destId), pos, stats);
  }

  cost calculate_value(const typename CRF::Label& src,
                       const typename CRF::Label& dest,
                       const int pos,
                       typename CRF::Stats* stats = 0) {
    typename CRF::Values vals;
    const auto f = [&](auto func) {
      return func(this->x[pos], pos ? src : dest, dest);
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

  std::pair<cost, unsigned> traverse_transitions(const Transition* const children,
                            unsigned children_length,
                            const typename CRF::Label& src,
                            const int pos) {
    auto cmp = [&](const Transition& tr1, const Transition& tr2) {
      return funcs.is_better(tr1.base_value, tr2.base_value);
    };
    // Holds 2 best paths
    pqueue<Transition, 2> pq;

    auto agg = funcs.empty();
    for(unsigned m = 0; m < children_length; m++) {
      auto& currentTr = children[m];
      // value of transition to that label
      auto transitionValue = calculate_value(src, currentTr.child, pos);
      // concatenation of the cost of the target label
      // and the transition value
      auto child_value = funcs.concat(currentTr.base_value, transitionValue);
      pq.push(Transition::make(currentTr.child, child_value), cmp);

      agg = funcs.aggregate(child_value, agg);
    }
    return std::make_pair(agg, pq[0].child);
  }

  void traverse_at_position(const typename CRF::Alphabet::LabelClass& allowed,
                            Transition* children,
                            Transition* next_children,
                            int children_length,
                            int pos,
                            Matrix<unsigned> &paths) {
    for(auto m = 0; m < allowed.size(); m++) {
      auto srcId = allowed[m];
      const auto& src = alphabet.fromInt(srcId);

      auto pair = traverse_transitions(children,
                                       children_length,
                                       src,
                                       pos + 1);
      next_children[m].set(srcId, pair.first);

      paths(srcId, pos) = pair.second;
    }
  }

  cost traverse(vector<int>* max_path_ptr,
                vector<int>* max_2_path_ptr=0) {
    Progress prog(x.size());

    // Will need for intermediate computations
    Transition children_a[alphabet_length()];
    Transition next_children_a[alphabet_length()];

    Transition* children = children_a,
      *next_children = next_children_a;

    unsigned children_length = 0;
    unsigned next_children_length = 0;

    int pos = x.size() - 1;

    Matrix<unsigned> paths(alphabet_length(), x.size());
    Matrix<unsigned> paths_2(alphabet_length(), x.size());

    // transitions to final state
    // value of the last "column" of states
    for(auto id : alphabet.get_class(x[pos]))
      children[children_length++].set(id, funcs.empty());
    prog.update();

    // backwards, for every zero-based position in
    // the input sequence except the last one...
    for(pos--; pos >= 0; pos--) {
      // for every possible label...
      const auto& allowed = alphabet.get_class(x[pos]);

      next_children_length = allowed.size();
      assert(children_length > 0);

      traverse_at_position(allowed, children,
                           next_children,
                           children_length, pos,
                           paths);

      children_length = 0;
      std::swap(children, next_children);
      std::swap(children_length, next_children_length);
      prog.update();
    }

    auto p = traverse_transitions(children,
                                  children_length,
                                  alphabet.fromInt(0),
                                  0);
    prog.finish();

    if(max_path_ptr) {
      vector<int>& best_path = *max_path_ptr;
      unsigned best_child = p.second;
      best_path.push_back(best_child);
      for(int i = 0; i <= (int) x.size() - 2; i++) {
        best_child = paths(best_child, i);
        best_path.push_back(best_child);
      }
    }

    return p.first;
  }
};

template<class CRF>
cost concat_cost(const vector<typename CRF::Label>& y,
                 CRF& crf,
                 const typename CRF::Values& lambda,
                 const vector<typename CRF::Input>& inputs,
                 typename CRF::Stats* stats = 0) {
  FunctionalAutomaton<CRF, MinPathFindFunctions> a(crf);
  a.lambda = lambda;
  a.x = inputs;

  cost result = 0;
  for(unsigned i = 0; i < y.size(); i++)
    result += a.calculate_value((i == 0) ? y[i] : y[i-1], y[i], i, stats);
  return result;
}

#endif
