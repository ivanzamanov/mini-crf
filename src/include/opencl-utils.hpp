#ifndef __OPENCL_UTILS_HPP__
#define __OPENCL_UTILS_HPP__

#include"types.hpp"
#include"simpleCL.hpp"

namespace util {
  const std::string FEATURES_CL_FILE = "features.cl";
  const std::string FEATURES_KERNEL_NAME = "transition";
  const std::string BEST_KERNEL_NAME = "findBest";

  struct clVertexResult {
    cl_double agg;
    cl_int index;
  };

  struct clPhonemeInstance {
    cl_double mfcc_l[MFCC_N];
    cl_double pitch_l;
  
    cl_double mfcc_r[MFCC_N];
    cl_double pitch_r;
  
    cl_double duration;
    cl_double energy;

    cl_int label;
    cl_int ctx_left;
    cl_int ctx_right;
  };

  clPhonemeInstance toCL(const PhonemeInstance& p);
}


#endif
