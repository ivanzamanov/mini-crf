#ifndef __CRF_TRAINING_H__
#define __CRF_TRAINING_H__

#include"crf.hpp"

class GDCompute {
public:
  virtual void calculate(CRandomField& crf, Corpus& corpus,
                         CoefSequence& lambda, CoefSequence& mu) = 0;
};

class NaiveGDCompute : public GDCompute {
  void calculate(CRandomField& crf, Corpus& corpus,
                 CoefSequence& lambda, CoefSequence& mu) {

  }
};

bool errorObjectiveReached(CRandomField& crf, Corpus& corpus) {
  return false;
}

void trainGradientDescent(CRandomField& crf, Corpus& corpus) {
  CoefSequence lambda(crf.lambda.length());
  CoefSequence mu(crf.mu.length());
  GDCompute* gradient = new NaiveGDCompute();

  while(!errorObjectiveReached(crf, corpus)) {
    gradient->calculate(crf, corpus,
                       lambda, mu);
  }

  crf.lambda = lambda;
  crf.mu = mu;
}

#endif
