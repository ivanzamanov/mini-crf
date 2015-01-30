#include"crf/crf.hpp"
#include"crf/training.hpp"

struct LabelAlphabet {
  template<class Filter>
  void iterate_sequences(const Sequence<Input>& input, Filter& filter) {

  }
};

typedef CRandomField<LabelAlphabet> CRF;

int main() {
  Sequence<StateFunction> state_functions(1);
  Sequence<TransitionFunction> transition_functions(1);

  CRF crf(state_functions, transition_functions);
  const int N = 10;
  Sequence<int> input(N);
  Sequence<int> labels(N);
  for (int i = 0; i < N; i++) {
    input[i] = i;
    labels[i] = i;
  }
  Corpus corpus;
  corpus.add(input, labels);

  trainGradientDescent<CRF, NaiveGDCompute>(crf, corpus);
  return 0;
}
