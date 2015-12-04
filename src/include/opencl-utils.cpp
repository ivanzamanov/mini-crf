#include<cassert>
#include"opencl-utils.hpp"

using namespace util;

sclHard util::hardware;
sclSoft util::SOFT_FEATURES;
sclSoft util::SOFT_BEST;

static bool CL_INITED = false;
void util::initCL() {
  if(!CL_INITED) {
    CL_INITED = true;
    int found;
    util::hardware = sclGetGPUHardware( 0, &found );
    assert(found);
    util::SOFT_FEATURES = sclGetCLSoftware(FEATURES_CL_FILE.data(),
                                           FEATURES_KERNEL_NAME.data(),
                                           util::hardware);
    util::SOFT_BEST = sclGetCLSoftware(FEATURES_CL_FILE.data(),
                                       BEST_KERNEL_NAME.data(),
                                       util::hardware);
  }
}

clPhonemeInstance util::toCL(const PhonemeInstance& p) {
  clPhonemeInstance result = {
    .duration = p.log_duration,
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
