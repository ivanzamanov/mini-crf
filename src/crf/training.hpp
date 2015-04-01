#ifndef __CRF_TRAINING_H__
#define __CRF_TRAINING_H__

#include<iostream>
#include<cmath>
#include"crf.hpp"

struct GradientValues {
  GradientValues(int lambda_size, int mu_size)
    : lambda_values(lambda_size), mu_values(mu_size) { }
  CoefSequence lambda_values;
  CoefSequence mu_values;
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
      if( c == 2)
        break;
      (std::cout << ".").flush();
      const Sequence<Label>& y = corpus.label(c);

      const Sequence<Input>& x = corpus.input(c);
      double A = 0;

      for(int j = 0; j < y.length(); j++) {
        auto func = crf.f[pos];
        auto coef = lambda[pos];
        A += coef * (*func)(y, pos + 1, x);
      }

      double norm = norm_factor(x, crf, lambda, mu);
      result += std::log(A) - std::log(norm);
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
        auto coef = mu[pos];
        auto func = crf.g[pos];
        A += coef * (*func)(y, pos + 1, x);
      }

      double norm = norm_factor(x, crf, lambda, mu);
      result += log(A) - log(norm);
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

    double proba = crf_probability_of(y, x, crf, lambdas, mu);
    likelihood += proba;
  }
  std::cout << "Likelihood " << likelihood << std::endl;
  return likelihood;
}

template<class CRF>
double calculateStepSize(GradientValues& direction, CRF& crf, Corpus& corpus) {
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
  std::cout << "Step size " << t << std::endl;
  return t;
}

template<class CRF, template <typename> class GDCompute>
void trainGradientDescent(CRF& crf, Corpus& corpus) {
  CoefSequence lambda(crf.lambda.length());
  for(int i = 0; i < lambda.length(); i++)
    lambda[i] = 1;
  CoefSequence mu(crf.mu.length());
  for(int i = 0; i < mu.length(); i++)
    mu[i] = 1;
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
