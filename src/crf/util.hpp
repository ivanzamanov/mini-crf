#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include<cmath>
#include<limits>

#define DEBUG(x) x

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
  
  double log_mult(double logX, double logY) {
    return logX + logY;
  }
  
  double log_sum(double logX, double logY) {
    if(logX > logY)
      std::swap(logX, logY);
    return log( exp(logY - logX) + 1) + logX;
  }
  
  double sum_log(double x, double y) {
    if(x == 0)
      return y;
    if(y == 0)
      return x;
    if(x == -y)
      return 0;

    bool same_sign = x * y > 0;
    int sign;
    if(same_sign) {
      sign = (x > 0) ? 1 : -1;

      double logAbsX = x;
      double log_sum = log( exp(y - logAbsX) + 1) + logAbsX;
      return sign * exp ( log_sum );
    } else {
      sign = (x + y) > 0 ? 1 : -1;
      if(abs(x) > abs(y))
        std::swap(x, y);

      double logAbsX = log(abs(x));
      double log_sum = log( exp(log(abs(y)) - logAbsX) - 1) + logAbsX;
      return sign * exp ( log_sum );
    }
  }

  
  double sum(double x, double y) {
    if(x == 0)
      return y;
    if(y == 0)
      return x;
    if(x == -y)
      return 0;

    bool same_sign = x * y > 0;
    int sign;
    if(same_sign) {
      sign = (x > 0) ? 1 : -1;

      double logAbsX = log(abs(x));
      double log_sum = log( exp(log(abs(y)) - logAbsX) + 1) + logAbsX;
      return sign * exp ( log_sum );
    } else {
      sign = (x + y) > 0 ? 1 : -1;
      if(abs(x) > abs(y))
        std::swap(x, y);

      double logAbsX = log(abs(x));
      double log_sum = log( exp(log(abs(y)) - logAbsX) - 1) + logAbsX;
      return sign * exp ( log_sum );
    }
  }
  
  double sub(double x, double y) {
    return sum(x, -y);
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