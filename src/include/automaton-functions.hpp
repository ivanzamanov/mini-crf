#ifndef __AUTOMATON_FUNCTIONS__
#define __AUTOMATON_FUNCTIONS__

#include<limits>

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

  // Which value to choose as "better", doesn't matter in this case
  bool is_better(cost, cost) {
    return false;
  }

  // Value of a transition from a last state, corresponding to a position
  // in the input, 1 by def.
  cost empty() {
    return 1;
  }

  // A "worst-case" value, i.e. everything will be
  // preferred over it
  cost worst() {
    return std::numeric_limits<cost>::max();
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

  cost worst() {
    return std::numeric_limits<cost>::max();
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

  cost worst() {
    return std::numeric_limits<cost>::min();
  }
};

#endif
