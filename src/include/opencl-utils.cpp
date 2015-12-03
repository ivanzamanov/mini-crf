#include"opencl-utils.hpp"

using namespace util;

clPhonemeInstance util::toCL(const PhonemeInstance& p) {
  clPhonemeInstance result = {
    .duration = p.duration,
    .energy = p.energy,
    .label = p.label,
    .ctx_left = p.ctx_left,
    .ctx_right = p.ctx_right,
  };

  for(int i = 0; i < MFCC_N; i++) {
    result.mfcc_l[i] = p.frames[0].mfcc[i];
    result.mfcc_r[i] = p.frames[1].mfcc[i];
  }

  result.pitch_l = p.pitch_contour[0];
  result.pitch_r = p.pitch_contour[1];
  return result;
}
