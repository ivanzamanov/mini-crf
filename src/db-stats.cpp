#include<algorithm>

#include"opencl-utils.hpp"
#include"gridsearch.hpp"
#include"tool.hpp"

using namespace tool;

void print_test() {
  sclHard hardware;
  sclSoft software;
  int found = 0;
  // Get the hardware
  hardware = sclGetGPUHardware( 0, &found );
  // Get the software
  software = sclGetCLSoftware( "features.cl", "transition", hardware );

  const int N = 10000;
  clPhonemeInstance *input1 = new clPhonemeInstance[N];
  clPhonemeInstance *input2 = new clPhonemeInstance[N];
  cl_double *output = new cl_double[N * N];
  for(int i = 0; i < N; i++) {
    input1[i] = input2[i] = toCL(alphabet_synth.fromInt(i));
    for(int j = 0; j < N; j++)
      output[i * N + j] = 0;
  }
  
  // The lib only works with 2-dimensional tasks
  size_t work_groups[2] = {N, N}, work_items[2] = {1, 1};
  sclManageArgsLaunchKernel( hardware, software, work_groups, work_items,
                             " %r %r %w %r",
                             sizeof(clPhonemeInstance) * N, input1,
                             sizeof(clPhonemeInstance) * N, input2,
                             sizeof(cl_double) * N * N, output,
                             sizeof(cl_int), &N);

  //for(int i = 0; i < N * N; i++)
    //  std::cout << output[i] << ' ';
  std::cout << std::endl;
  delete[] input1;
  delete[] input2;
}

bool Progress::enabled = true;
std::string gridsearch::Comparisons::metric = "";
std::string gridsearch::Comparisons::aggregate = "";

int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);
  Options opts;
  if(!init_tool(argc, argv, &opts))
    return 1;

  std::cout << "Size: " << alphabet_synth.size() << std::endl;
  std::cout << "Corpus: " << corpus_synth.size() << std::endl;
  print_test();
  return 0;
}
