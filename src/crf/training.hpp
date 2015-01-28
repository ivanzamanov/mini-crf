#ifndef __CRF_TRAINING_H__
#define __CRF_TRAINING_H__

#include"crf.hpp"

typedef Sequence<double> ValueSequence;

struct GradientValues {
  GradientValues(int lambda_size, int mu_size)
    : lambda_values(lambda_size), mu_values(mu_size) { }
  ValueSequence lambda_values;
  ValueSequence mu_values;
};

class GDCompute {
public:
  GradientValues calculate(CRandomField& crf, Corpus& corpus,
                           CoefSequence& lambda, CoefSequence& mu) {
    GradientValues result(lambda.length(), mu.length());

    for(int i = 0; i < lambda.length(); i++) {
      result.lambda_values[i] = computeLambdaGradient(i, lambda, mu, crf, corpus);
    }

    for(int i = 0; i < lambda.length(); i++) {
      result.mu_values[i] = computeMuGradient(i, lambda, mu, crf, corpus);
    }

    return result;
  };

  virtual double computeLambdaGradient(int pos, CoefSequence& lambda, CoefSequence& mu,
                                       CRandomField& crf, Corpus& corpus) = 0;

  virtual double computeMuGradient(int pos, CoefSequence& lambda, CoefSequence& mu,
                                   CRandomField& crf, Corpus& corpus) = 0;
};

class NaiveGDCompute : public GDCompute {
  virtual double computeLambdaGradient(int pos, CoefSequence& lambda, CoefSequence& mu,
                                       CRandomField& crf, Corpus& corpus) {
    double result = 0;
    for(int c = 0; c < corpus.length(); c++) {
      const Sequence<Label>& y = corpus.label(c);
      const Sequence<Input>& x = corpus.input(c);

      double A = 0;
      for(int j = 0; j < y.length(); j++) {
        A += lambda[pos] * crf.f[pos](y, pos, x);
      }

      double B = 0;
      // iterate over all label vectors,
      
      result += A - B;
    }
    return result;
  };

  virtual double computeMuGradient(int pos, CoefSequence& lambda, CoefSequence& mu,
                                   CRandomField& crf, Corpus& corpus) {

  };
};

bool errorObjectiveReached(CRandomField& crf, Corpus& corpus) {
  return false;
}

void trainGradientDescent(CRandomField& crf, Corpus& corpus) {
  CoefSequence lambda(crf.lambda.length());
  CoefSequence mu(crf.mu.length());
  GDCompute* gradient = new NaiveGDCompute();

  while(!errorObjectiveReached(crf, corpus)) {
    GradientValues values = gradient->calculate(crf, corpus, lambda, mu);
    // Update lambdas...
  }

  crf.lambda = lambda;
  crf.mu = mu;
}

#endif
