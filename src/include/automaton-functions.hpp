#ifndef __AUTOMATON_FUNCTIONS__
#define __AUTOMATON_FUNCTIONS__

#include "types.hpp"
#include "util.hpp"

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

struct MinPathFindFunctions {
  cost concat(cost d1, cost d2) {
    return d1 + d2;
  }

  cost aggregate(cost d1, cost d2) {
    return std::min(d1, d2);
  }

  bool is_better(cost t1, cost t2) {
    return t1 < t2;
  }

  cost empty() {
    return 0;
  }
};

struct MaxPathFindFunctions {
  cost concat(cost d1, cost d2) {
    return d1 + d2;
  }

  cost aggregate(cost d1, cost d2) {
    return std::max(d1, d2);
  }

  bool is_better(cost t1, cost t2) {
    return t1 > t2;
  }

  cost empty() {
    return 0;
  }
};

#endif
