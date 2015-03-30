#ifndef __CRF_TRAINING_H__
#define __CRF_TRAINING_H__

#include<iostream>
#include"crf.hpp"

struct GradientValues {
  GradientValues(int lambda_size, int mu_size)
    : lambda_values(lambda_size), mu_values(mu_size) { }
  CoefSequence lambda_values;
  CoefSequence mu_values;
};

template<class CRF>
struct TransitionSumAccumulator {
  TransitionSumAccumulator(const CRF& crf,
                           const Sequence<Input>& inputs,
                           int k,
                           const CoefSequence lambda):
    crf(crf), x(inputs), k(k), lambda(lambda) { }
  const CRF& crf;
  const Sequence<Input>& x;
  int k;
  const CoefSequence& lambda;
  double result = 0;
  void operator()(const Sequence<Label>& y) {
    double prob = crf.probability_of(y, x);
    double sum = 0;
    for(int i = 1; i < y.length(); i++) {
      sum += lambda[k] * (*crf.f[k])(y, i, x);
    }
    result = prob * sum;
  }
};

template<class CRF>
struct StateSumAccumulator {
  StateSumAccumulator(const CRF& crf,
                      const Sequence<Input>& inputs,
                      int k,
                      const CoefSequence& mu):
    crf(crf), x(inputs), k(k), mu(mu) { }
  const CRF& crf;
  const Sequence<Input>& x;
  int k;
  const CoefSequence& mu;
  double result = 0;
  void operator()(const Sequence<Label>& y) {
    double prob = crf.probability_of(y, x);
    double sum = 0;
    for(int i = 1; i < y.length(); i++) {
      sum += mu[k] * (*crf.g[k])(y, i, x);
    }
    result = prob * sum;
  }
};

template<class CRF>
class NaiveGDCompute {
public:
  GradientValues calculate(CRF& crf, Corpus& corpus,
                           CoefSequence& lambda, CoefSequence& mu) {
    GradientValues result(lambda.length(), mu.length());

    std::cout << "Lambda gradient" << std::endl;
    for(int i = 0; i < lambda.length(); i++) {
      result.lambda_values[i] = computeLambdaGradient(i, lambda, mu, crf, corpus);
    }
    std::cout << '\n';

    std::cout << "Mu gradient" << std::endl;
    for(int i = 0; i < mu.length(); i++) {
      result.mu_values[i] = computeMuGradient(i, lambda, mu, crf, corpus);
    }
    std::cout << '\n';

    return result;
  };

  double computeLambdaGradient(int pos, CoefSequence& lambda, CoefSequence& mu,
                               CRF& crf, Corpus& corpus) {
    double result = 0;

    for(int c = 0; c < corpus.length(); c++) {
      (std::cout << ".").flush();
      const Sequence<Label>& y = corpus.label(c);

      const Sequence<Input>& x = corpus.input(c);
      double A = 0;

      for(int j = 0; j < y.length(); j++) {
        A += lambda[pos] * (*crf.f[pos])(y, pos + 1, x);
      }

      result += A - norm_factor(x, crf, lambda, mu);
    }

    return result;
  };

  double computeMuGradient(int pos, CoefSequence& lambda, CoefSequence& mu,
                           CRF& crf, Corpus& corpus) {
    double result = 0;

    for(int c = 0; c < corpus.length(); c++) {
      const Sequence<Label>& y = corpus.label(c);
      const Sequence<Input>& x = corpus.input(c);
      double A = 0;

      for(int j = 0; j < y.length(); j++) {
        A += mu[pos] * (*crf.g[pos])(y, pos + 1, x);
      }

      // iterate over all label vectors
      StateSumAccumulator<CRF> filter(crf, x, pos, mu);
      // TODO: make proper iterator...
      crf.label_alphabet.iterate_sequences(x, filter);
      result += A - filter.result;
    }

    return result;
  };

};

template<class CRF>
bool errorObjectiveReached(CRF& crf, Corpus& corpus) {
  return false;
}

template<class CRF>
void trainGradientDescent(CRF& crf, Corpus& corpus) {
  trainGradientDescent<CRF, NaiveGDCompute>(crf, corpus);
}

CoefSequence concat(CoefSequence& s1, CoefSequence& s2) {
  CoefSequence result(s1.length() + s2.length());
  for(int i = 0; i < s1.length(); i++)
    result[i] = s1[i];
  for(int i = 0; i < s2.length(); i++) {
    result[s1.length() + i] = s2[i];
  }
  return result;
}

CoefSequence operator+(CoefSequence& s1, CoefSequence& s2) {
  CoefSequence result(s1.length() + s2.length());
  for(int i = 0; i < s1.length(); i++)
    result[i] = s1[i] + s2[i];
  return result;
}

template<class CRF>
double likelihood(CoefSequence lambdas, CoefSequence mu, Corpus& corpus, CRF& crf) {
  double likelihood = 0;
  for(int c = 0; c < corpus.length(); c++) {
    const Sequence<Label>& y = corpus.label(c);
    const Sequence<Input>& x = corpus.input(c);

    likelihood += crf_probability_of(y, x, crf, lambdas, mu);
  }
  std::cout << "Likelihood " << likelihood << std::endl;
  return likelihood;
}

template<class CRF>
double calculateStepSize(GradientValues& direction, CRF& crf, Corpus& corpus) {
  std::cout << "Step size" << std::endl;
  double alpha = 0.0001d;
  double beta = 0.5d;
  double t = 1;
  CoefSequence lambdas = crf.lambda;
  CoefSequence mu = crf.mu;
  CoefSequence gradientVector = concat(direction.lambda_values, direction.mu_values);
  double corpusLikelihood = likelihood(lambdas, mu, corpus, crf);
  double gradTimesDir = 0;
  for(int i = 0; i < gradientVector.length(); i++) {
    gradTimesDir += -gradientVector[i] * gradientVector[i];
  }

  double left = likelihood(lambdas + direction.lambda_values, mu + direction.mu_values, corpus, crf);
  double right = corpusLikelihood + alpha * t * gradTimesDir;
  while(left > right) {
    left = likelihood(lambdas + direction.lambda_values, mu + direction.mu_values, corpus, crf);
    right = corpusLikelihood + alpha * t * gradTimesDir;
    t = beta * t;
  }
  return t;
}

template<class CRF, template <typename> class GDCompute>
void trainGradientDescent(CRF& crf, Corpus& corpus) {
  CoefSequence lambda(crf.lambda.length());
  CoefSequence mu(crf.mu.length());
  GDCompute<CRF> gradient;

  for(int i = 0; i < 100; i++) {
    std::cout << "Iteration " << i << std::endl;
    GradientValues values = gradient.calculate(crf, corpus, lambda, mu);
    double stepSize = calculateStepSize(values, crf, corpus);

    for(int i = 0; i < crf.lambda.length(); i++)
      crf.lambda[i] += -values.lambda_values[i] * stepSize;

    for(int i = 0; i < crf.mu.length(); i++)
      crf.mu[i] += -values.mu_values[i] * stepSize;
  }

  crf.lambda = lambda;
  crf.mu = mu;
}

#endif
