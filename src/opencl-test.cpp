#include<cassert>
#include<vector>
#include"features.hpp"
#include"crf.hpp"
#include"gridsearch.hpp"

cl_double emptyMFCC[MFCC_N] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

PhonemeInstance emptyPI() {
  PhonemeInstance result;
  result.pitch_contour[0] = result.pitch_contour[1] = 1;
  result.energy = 1;
  result.start = 0;
  result.duration = result.end = 1;
  result.label = 1;
  result.ctx_left = 1;
  result.ctx_right = 1;
  for(int i = 0; i < MFCC_N; i++)
    result.frames[0].mfcc[i] = result.frames[1].mfcc[i] = emptyMFCC[i];
  return result;
}

clPhonemeInstance emptyCLPI() {
  return toCL(emptyPI());
}

void doTest() {
  initCL();
  PhoneticFeatures f;

  clPhonemeInstance *sources, *dests;
  const int srcCount = 1;
  const int destCount = 1;

  sources = new clPhonemeInstance[srcCount];
  dests = new clPhonemeInstance[destCount];
  cl_double* outputs = new cl_double[srcCount * destCount];
  memset(outputs, 10, sizeof(cl_double) * srcCount * destCount);
  const cl_int coefCount = f.VSIZE + f.ESIZE;
  cl_double coefficients[coefCount] = {1, 1, 1, 1, 1, 1};
  std::vector<coefficient> coefVector({1, 1, 1, 1, 1, 1});
  std::vector<PhonemeInstance> xVec;
  xVec.push_back(emptyPI());
  clVertexResult* bests = new clVertexResult[srcCount];
  clPhonemeInstance stateLabel;

  sources[0] = emptyCLPI();
  dests[0] = emptyCLPI();
  stateLabel = emptyCLPI();

  size_t work_groups[2] = { (size_t) srcCount, (size_t) destCount };
  size_t work_items[2] = {1, 1};

  cl_int destCountCL = destCount;
  *outputs = 10;
  sclManageArgsLaunchKernel(util::hardware, util::SOFT_FEATURES, work_groups, work_items,
                            "%r %r %w %r %r %r",
                            sizeof(clPhonemeInstance) * srcCount, sources,
                            sizeof(clPhonemeInstance) * destCount, dests,
                            sizeof(cl_double) * srcCount * destCount, outputs,
                            sizeof(cl_int), & destCountCL,
                            sizeof(clPhonemeInstance), & stateLabel,
                            sizeof(cl_double) * coefCount, coefficients);

#ifdef DEBUG

  for(int x = 0; x < srcCount; x++) {
    for(int y = 0; y < destCount; y++) {
      cost verify = 0;
      auto p1 = emptyPI();
      verify += f.invoke_transition(p1, p1, 0, xVec, coefVector, coefVector);
      verify += f.invoke_state(p1, p1, 0, xVec, coefVector, coefVector);
      INFO(verify);
      INFO(outputs[x * destCount + y]);
      assert(verify == outputs[x * destCount + y]);
    }
  }
  
#endif

  INFO(*outputs);
  work_groups[0] = srcCount, work_groups[1] = 1;
  sclManageArgsLaunchKernel(util::hardware, util::SOFT_BEST, work_groups, work_items,
                            " %r %r %w",
                            sizeof(cl_double) * srcCount * destCount, outputs,
                            sizeof(cl_int), &destCountCL,
                            sizeof(clVertexResult) * srcCount, bests);

  // ... And clean up
  delete[] bests;
  delete[] outputs;
  delete[] dests;
  delete[] sources;
}

bool Progress::enabled = true;
std::string gridsearch::Comparisons::metric = "";
std::string gridsearch::Comparisons::aggregate = "";

int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);
  doTest();

  return 0;
}
