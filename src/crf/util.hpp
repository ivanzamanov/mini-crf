#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include<cmath>
#include<limits>

#define DEBUG(x)

#define COMPARE(x, y) x <= y
#define MAX std::numeric_limits<double>::max()

using namespace std;

namespace util {
  double mult_exp(double x, double y) {
    if(y == 0)
      return 0;
    int sign = (y > 0) ? 1 : -1;
    double absX = abs(x);
    double logAbsY = log(abs(y));
    double mod = exp( absX + logAbsY );
    return sign * mod;
  }

  double mult(double x, double y) {
    if(x == 0 || y == 0)
      return 0;
    int sign = (x * y > 0) ? 1 : -1;
    return sign * exp( log(abs(x)) + log(abs(y)) );
  }

  double sum(double x, double y) {
    return x + y;
    if(x == 0)
      return y;
    if(y == 0)
      return x;

    int sign = (x + y > 0) ? 1 : -1;
    return sign * log( exp( log(abs(y)) - log(abs(x))) + 1) + log(abs(x));
  }

  struct max_finder {
    max_finder():max_pos(-1), max_val(MAX) { }
    
    int max_pos;
    double max_val;
    
    void check(int pos, double val) {
      if(max_pos == -1 || COMPARE(max_val, val)) {
        max_pos = pos;
        max_val = val;
      }
    }
    
    void reset() {
      max_pos = -1;
      max_val = MAX;
    }
  };
};

#endif