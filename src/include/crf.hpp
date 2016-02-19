#ifndef __CRF_H__
#define __CRF_H__

#include<algorithm>
#include<cassert>
#include<array>

#include"alphabet.hpp"

#ifdef USE_OPENCL
#include"opencl-utils.hpp"
#else
#include"types.hpp"
#endif

#include"matrix.hpp"

using std::vector;
using namespace util;

template<class _LabelAlphabet, class _Input, class _Features>
class CRandomField;

template<class Input, class Label>
class _Corpus;

template<class CRF>
struct FunctionalAutomaton;

#ifdef USE_OPENCL
template<typename CRF,
         bool includeState,
         bool includeTransition>
void traverse_at_position_automaton(FunctionalAutomaton<CRF> &a,
                                    const typename CRF::Alphabet::LabelClass& allowed,
                                    Transition* children,
                                    Transition* next_children,
                                    int children_length,
                                    int pos,
                                    Matrix<unsigned> &paths);
#endif

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

template<class CRF>
cost traverse_automaton(const vector<typename CRF::Input>& x,
                        CRF& crf,
                        const typename CRF::Values& lambda,
                        vector<int>* max_path) {
  FunctionalAutomaton<CRF> a(crf);
  a.lambda = lambda;
  a.x = x;

  return a.traverse(max_path);
}

template<class CRF>
struct FunctionalAutomaton {
  FunctionalAutomaton(const CRF& crf): crf(crf), alphabet(crf.alphabet()), lambda(crf.lambda) { }

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
                       const typename CRF::Label& dest,
                       int pos,
                       typename CRF::Stats* stats = 0) {
    typename CRF::Values vals;
    const auto f = [&](auto func) {
      return func(this->x[pos], pos == 0 ? dest : src, dest);
    };
    tuples::Invoke<CRF::features::size,
                   decltype(CRF::features::Functions)>{}(vals, f);
    cost result = 0;
    for(unsigned i = 0; i < vals.size(); i++) {
      result += vals[i] * lambda[i];
      if(stats) {
        stats->get(pos)[i] = vals[i] * lambda[i];
      }
    }
    return result;
  }

  cost traverse_transitions(const Transition* children,
                            unsigned children_length,
                            const typename CRF::Label& src,
                            int pos, unsigned& max_child) {
    unsigned m = 0;
    Transition transition;

    transition = children[m];
    auto child = alphabet.fromInt(transition.child);
    cost transitionValue = calculate_value(src, child, pos);
    cost child_value = funcs.concat(transition.base_value, transitionValue);

    unsigned max_child_tmp = transition.child;
    cost prev_value = child_value;

    cost agg;
    agg = child_value;
    for(++m; m < children_length; ++m) {
      transition = children[m];
      child = alphabet.fromInt(transition.child);
      transitionValue = calculate_value(src, child, pos);
      child_value = funcs.concat(transition.base_value, transitionValue);
      if(funcs.is_better(child_value, prev_value)) {
        max_child_tmp = transition.child;
        prev_value = child_value;
      }

      agg = funcs.aggregate(child_value, agg);
    }

    max_child = max_child_tmp;
    return agg;
  }

  void traverse_at_position(const typename CRF::Alphabet::LabelClass& allowed,
                            Transition* children,
                            Transition* next_children,
                            int children_length,
                            int pos,
                            Matrix<unsigned> &paths) {
    for(unsigned m = 0; m < allowed.size(); m++) {
      auto srcId = allowed[m];

      const typename CRF::Label& src = alphabet.fromInt(srcId);
      unsigned max_child;
      cost value = traverse_transitions(children,
                                        children_length,
                                        src,
                                        pos + 1, max_child);
      next_children[m].set(srcId, value);

      paths(srcId, pos) = max_child;
    }
  }

  cost traverse(vector<int>* max_path_ptr) {
    Progress prog(x.size());

    vector<int> max_path;
    // Will need for intermediate computations
    Transition* children_a = new Transition[alphabet_length()],
      *next_children_a = new Transition[alphabet_length()];

    Transition* children = children_a,
      *next_children = next_children_a;

    unsigned children_length = 0;
    unsigned next_children_length = 0;

    int pos = x.size() - 1;

    Matrix<unsigned> paths(alphabet_length(), x.size());
    vector<unsigned> options;

    // transitions to final state
    // value of the last "column" of states
    const typename CRF::Alphabet::LabelClass& allowed = alphabet.get_class(x[pos]);
    options.push_back(allowed.size());
    for(auto it = allowed.begin(); it != allowed.end() ; it++) {
      children[children_length++].set( *it, funcs.empty());
    }
    prog.update();

    // backwards, for every zero-based position in
    // the input sequence except the last one...
    for(pos--; pos >= 0; pos--) {
      // for every possible label...
      const typename CRF::Alphabet::LabelClass& allowed = alphabet.get_class(x[pos]);
      options.push_back(allowed.size());

      next_children_length = allowed.size();
      assert(children_length > 0);

#ifdef USE_OPENCL
      traverse_at_position_automaton<CRF, true, true>(*this, allowed, children,
                                                      next_children,
                                                      children_length, pos,
                                                      paths);
#else
      traverse_at_position(allowed, children,
                           next_children,
                           children_length, pos,
                           paths);
#endif
      children_length = 0;
      std::swap(children, next_children);
      std::swap(children_length, next_children_length);
      prog.update();
    }

    unsigned max_child;
    cost value = traverse_transitions(children,
                                      children_length,
                                      alphabet.fromInt(0),
                                      0, max_child);
    prog.finish();

    max_path.push_back(max_child);
    for(int i = 0; i <= (int) x.size() - 2; i++) {
      max_child = paths(max_child, i);
      max_path.push_back(max_child);
    }

    if(max_path_ptr)
      *max_path_ptr = max_path;

    delete[] children_a;
    delete[] next_children_a;

    return value;
  }
};

template<class CRF>
cost max_path(const vector<typename CRF::Input>& x,
              CRF& crf,
              const typename CRF::Values& lambda,
              vector<int>* max_path) {
  return traverse_automaton(x, crf, lambda, max_path);
}

template<class CRF>
cost concat_cost(const vector<typename CRF::Label>& y,
                 CRF& crf,
                 const typename CRF::Values& lambda,
                 const vector<typename CRF::Input>& inputs,
                 typename CRF::Stats* stats = 0) {
  FunctionalAutomaton<CRF> a(crf);
  a.lambda = lambda;
  a.x = inputs;

  cost result = 0;
  for(unsigned i = 0; i < y.size(); i++)
    result += a.calculate_value((i == 0) ? y[i] : y[i-1], y[i], i, stats);
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

#ifdef USE_OPENCL
template<typename CRF,
         bool includeState,
         bool includeTransition>
void traverse_at_position_automaton(FunctionalAutomaton<CRF> &a,
                                    const typename CRF::Alphabet::LabelClass& allowed,
                                    Transition* children,
                                    Transition* next_children,
                                    int children_length,
                                    int pos,
                                    Matrix<unsigned> &paths) {
  initCL();
  clPhonemeInstance *sources, *dests;
  const int srcCount = allowed.size();
  const int destCount = children_length;

  sources = new clPhonemeInstance[srcCount];
  dests = new clPhonemeInstance[destCount];
  cl_double* outputs = new cl_double[srcCount * destCount];
  memset(outputs, 0, sizeof(cl_double) * srcCount * destCount);
  cl_int coefCount = a.crf.features.VSIZE + a.crf.features.ESIZE;
  cl_double* coefficients = new double[coefCount];
  clVertexResult* bests = new clVertexResult[srcCount];
  clPhonemeInstance stateLabel = toCL( a.x[pos] );

  int coefIndex = 0;
  for(auto& c : a.lambda)
    coefficients[coefIndex++] = c * includeTransition;

  for(unsigned m = 0; m < srcCount; m++) {
    auto srcId = allowed[m];
    sources[m] = toCL( a.alphabet.fromInt(srcId) );
  }

  for(int m = 0; m < destCount; m++) {
    auto destId = children[m].child;
    dests[m] = toCL( a.alphabet.fromInt(destId) );
  }

  size_t work_groups[2] = {(size_t) srcCount, (size_t) destCount};
  size_t work_items[2] = {1, 1};

  size_t inputSizeSrc = sizeof(clPhonemeInstance) * srcCount;
  size_t inputSizeDest = sizeof(clPhonemeInstance) * destCount;
  cl_int destCountCL = destCount;

  sclManageArgsLaunchKernel(util::hardware, util::SOFT_FEATURES, work_groups, work_items,
                            " %r %r %w %r %r %r",
                            inputSizeSrc, sources,
                            inputSizeDest, dests,
                            sizeof(cl_double) * srcCount * destCount, outputs,
                            sizeof(cl_int), &destCountCL,
                            sizeof(clPhonemeInstance), &stateLabel,
                            sizeof(cl_double) * coefCount, coefficients);

  /*#ifdef DEBUG

    for(int x = 0; x < srcCount; x++) {
    for(int y = 0; y < destCount; y++) {
    cost verify;
    auto p1 = a.alphabet.fromInt(allowed[x]);
    auto p2 = a.alphabet.fromInt(children[y].child);
    verify = a.total_cost(p1, p2, pos);
    INFO(verify << " vs " << outputs[x * destCount + y]);
    assert(std::abs(verify - outputs[x * destCount + y]) < 0.01);
    }
    }

    #endif*/

  work_groups[0] = srcCount; work_groups[1] = 1;
  sclManageArgsLaunchKernel(util::hardware, util::SOFT_BEST, work_groups, work_items,
    " %r %r %w",
    sizeof(cl_double) * srcCount * destCount, outputs,
    sizeof(cl_int), &destCountCL,
    sizeof(clVertexResult) * srcCount, bests);

  // And just copy back
  for(unsigned m = 0; m < allowed.size(); m++) {
  auto srcId = allowed[m];

  double value = bests[m].agg;
  next_children[m].set(srcId, value);

  paths(srcId, pos) = bests[m].index;
}

  // ... And clean up
  delete[] bests;
  delete[] outputs;
  delete[] coefficients;
  delete[] dests;
  delete[] sources;
}

#endif

#endif
