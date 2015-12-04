#define MFCC_N 12
#define FRAMES_N 2

struct clPhonemeInstance {
  double mfcc_l[MFCC_N];
  double pitch_l;

  double mfcc_r[MFCC_N];
  double pitch_r;

  double duration;
  double energy;

  int label;
  int ctx_left;
  int ctx_right;
};

struct clVertexResult {
  double agg;
  int index;
};

// Features

double Pitch(const struct clPhonemeInstance prev,
             const struct clPhonemeInstance next) {
  double val = prev.pitch_r - next.pitch_l;
  return fabs(val);
}

double Duration(const struct clPhonemeInstance prev,
             const struct clPhonemeInstance next) {
  return fabs(prev.duration - next.duration);
}

double PitchState(const struct clPhonemeInstance prev,
             const struct clPhonemeInstance next) {
  return fabs(prev.pitch_r - next.pitch_r) + fabs(prev.pitch_l - next.pitch_l);
}

double Energy(const struct clPhonemeInstance prev,
             const struct clPhonemeInstance next) {
  double val = prev.energy / next.energy;
  return fabs(val);
}

double LeftContext(const struct clPhonemeInstance prev,
                   const struct clPhonemeInstance next) {
  return (prev.label == next.ctx_left) ? 0 : 1;
}

double MFCCDist(const struct clPhonemeInstance prev,
                const struct clPhonemeInstance next) {
  double result = 0;
  for (unsigned i = 0; i < MFCC_N; i++) {
    double diff = prev.mfcc_r[i] - next.mfcc_l[i];
    result += diff * diff;
  }
  return sqrt(result);
}

// End features

__kernel void transition(__global struct clPhonemeInstance* srcArr,
                         __global struct clPhonemeInstance* destArr,
                         __global double* outputArr,
                         __global int* rowSizePtr,
                         __global struct clPhonemeInstance* stateLabelPtr,
                         __global double* coeffs) {
  int x = get_global_id(0);
  int y = get_global_id(1);
  int rowSize = *rowSizePtr;

  const struct clPhonemeInstance src = srcArr[x];
  const struct clPhonemeInstance dest = destArr[y];
  const struct clPhonemeInstance state = *stateLabelPtr;

  double result = 0;
  // Keep order in sync with features.hpp:PhoneticFeatures

  // TR
  result += coeffs[0] * Pitch(src, dest);
  result += coeffs[1] * MFCCDist(src, dest);
  result += coeffs[2] * LeftContext(src, dest);

  // ST
  result += coeffs[3] * Duration(state, dest);
  result += coeffs[4] * PitchState(state, dest);
  result += coeffs[5] * Energy(state, dest);

  outputArr[x * rowSize + y] = result;
}

__kernel void findBest(__global double* transitions,
                       __global int* rowSizePtr,
                       __global struct clVertexResult* results) {
  int x = get_global_id(0);
  int rowSize = *rowSizePtr;

  long transitionOffset = x * rowSize;

  int index = 0;
  double best = transitions[transitionOffset];
  for(int i = 1; i < rowSize; i++) {
    if(transitions[transitionOffset + i] < best) {
      best = transitions[transitionOffset + i];
      index = i;
    }
  }
  results[x].index = index;
  results[x].agg = best;
}

__kernel void test(__global char* debug) {
  *debug = 'B';
}
