#include<sstream>

#include"util.hpp"

bool COLOR_ENABLED = true;
bool PRINT_SCALE = false;
bool REPORT_PROGRESS = false;

namespace util {
  double mult_exp(double x, double y) {
    if(y == 0)
      return 0;
    int sign = (y > 0) ? 1 : -1;
    double absX = std::abs(x);
    double logAbsY = std::log(std::abs(y));
    double mod = std::exp( absX + logAbsY );
    return sign * mod;
  }

  double mult(double x, double y) {
    if(x == 0 || y == 0)
      return 0;
    int sign = (x * y > 0) ? 1 : -1;
    return sign * std::exp( std::log(std::abs(x)) + std::log(std::abs(y)) );
  }
  
  double log_mult(double logX, double logY) {
    return logX + logY;
  }
  
  double log_sum(double logX, double logY) {
    if(logX > logY)
      std::swap(logX, logY);
    return log( exp(logY - logX) + 1) + logX;
  }

  double log_sum2(double logX, double logY) {
    return logX + log_sum(MY_E, logY - logX);
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
      double log_sum = std::log( std::exp(y - logAbsX) + 1) + logAbsX;
      return sign * std::exp ( log_sum );
    } else {
      sign = (x + y) > 0 ? 1 : -1;
      if(std::abs(x) > std::abs(y))
        std::swap(x, y);

      double logAbsX = std::log(std::abs(x));
      double log_sum = std::log( std::exp(std::log(std::abs(y)) - logAbsX) - 1) + logAbsX;
      return sign * std::exp ( log_sum );
    }
  }

  double sum(double x, double y) {
    if(x == 0)
      return y;
    if(y == 0)
      return x;
    if(x == -y)
      return 0;

    double logAbsX = std::log(std::abs(x));
    double logAbsY = std::log(std::abs(y));
    if(logAbsX > logAbsY) {
      std::swap(logAbsX, logAbsY);
      std::swap(x, y);
    }

    double result = std::log( std::exp(logAbsY - logAbsX) + 1) + logAbsX;
    int sign;
    if(x < 0 && y < 0) sign = -1;
    else if(x > 0 && y > 0) sign = 1;
    else sign = (y > 0 ? 1 : -1);

    return std::exp(result) * sign;
  }

  std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
      if(!(item == ""))
        elems.push_back(item);
    }
    return elems;
  }

  std::vector<std::string> split_string(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
  }
}

bool compare(std::string &s1, std::string &s2) {
  return s1 == s2;
}

bool compare(int& i1, int& i2) {
  return i1 == i2;
}
