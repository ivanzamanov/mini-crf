#ifndef __CRF_TRAINING_H__
#define __CRF_TRAINING_H__

#include<iostream>
#include<cmath>
#include"crf.hpp"

struct GradientValues {
  GradientValues(int lambda_size, int mu_size)
    : lambda_values(lambda_size), mu_values(mu_size) { }
  std::vector<double> lambda_values;
  std::vector<double> mu_values;
};

template<class CRF>
class NaiveGDCompute {
public:
  GradientValues calculate(CRF& crf, typename CRF::TrainingCorpus& corpus,
                           std::vector<double>& lambda, std::vector<double>& mu) {
    GradientValues result(lambda.size(), mu.size());

    std::cerr << "Lambda gradient" << std::endl;
    for(unsigned i = 0; i < lambda.size(); i++) {
      result.lambda_values[i] = computeLambdaGradient(i, lambda, mu, crf, corpus);
    }
    std::cerr << '\n';

    std::cerr << "Mu gradient" << std::endl;
    for(unsigned i = 0; i < mu.size(); i++) {
      result.mu_values[i] = computeMuGradient(i, lambda, mu, crf, corpus);
    }
    std::cerr << '\n';

    return result;
  };

  double computeLambdaGradient(int pos, std::vector<double>& lambda, std::vector<double>& mu,
                               CRF& crf, typename  CRF::TrainingCorpus& corpus) {
    double result = 0;

    for(unsigned c = 0; c < corpus.size(); c++) {
      if( c == 2)
        break;
      (std::cerr << ".").flush();
      const std::vector<typename CRF::Label>& y = corpus.label(c);

      const std::vector<typename CRF::Input>& x = corpus.input(c);
      double A = 0;

      for(unsigned j = 0; j < y.size(); j++) {
        auto func = crf.f[pos];
        auto coef = lambda[pos];
        A += coef * (*func)(y, pos + 1, x);
      }

      double norm = norm_factor(x, crf, lambda, mu);
      result += A - norm;
    }

    return result;
  };

  double computeMuGradient(int pos, std::vector<double>& lambda, std::vector<double>& mu,
                           CRF& crf, typename CRF::TrainingCorpus& corpus) {
    double result = 0;

    for(unsigned c = 0; c < corpus.size(); c++) {
      const std::vector<typename CRF::Label>& y = corpus.label(c);
      const std::vector<typename CRF::Input>& x = corpus.input(c);
      double A = 0;

      for(unsigned j = 0; j < y.size(); j++) {
        auto coef = mu[pos];
        auto func = crf.g[pos];
        A += coef * (*func)(y, pos + 1, x);
      }

      double norm = norm_factor(x, crf, lambda, mu);
      result += A - norm;
    }

    return result;
  };

};

template<class CRF>
bool errorObjectiveReached(CRF& crf, typename CRF::TrainingCorpus& corpus) {
  return false;
}

template<class CRF>
void trainGradientDescent(CRF& crf, typename CRF::TrainingCorpus& corpus) {
  trainGradientDescent<CRF, NaiveGDCompute>(crf, corpus);
}

std::vector<double> concat(std::vector<double>& s1, std::vector<double>& s2) {
  std::vector<double> result(s1.size() + s2.size());
  for(unsigned i = 0; i < s1.size(); i++)
    result[i] = s1[i];
  for(unsigned i = 0; i < s2.size(); i++) {
    result[s1.size() + i] = s2[i];
  }
  return result;
}

std::vector<double> operator+(std::vector<double>& s1, std::vector<double>& s2) {
  std::vector<double> result(s1.size() + s2.size());
  for(unsigned i = 0; i < s1.size(); i++)
    result[i] = s1[i] + s2[i];
  return result;
}

template<class CRF>
double likelihood(std::vector<double> lambdas, std::vector<double> mu, typename CRF::TrainingCorpus& corpus, CRF& crf) {
  double likelihood = 0;
  for(unsigned c = 0; c < corpus.size(); c++) {
    const std::vector<typename CRF::Label>& y = corpus.label(c);
    const std::vector<typename CRF::Input>& x = corpus.input(c);

    double proba = crf.probability_of(y, x, crf, lambdas, mu);
    likelihood += proba;
  }
  std::cerr << "Likelihood " << likelihood << std::endl;
  return likelihood;
}

template<class CRF>
double calculateStepSize(GradientValues& direction, CRF& crf, typename CRF::TrainingCorpus& corpus) {
  double alpha = 0.0001;
  double beta = 0.5;
  double t = 1;
  std::vector<double> lambdas = crf.lambda;
  std::vector<double> mu = crf.mu;
  std::vector<double> gradientVector = concat(direction.lambda_values, direction.mu_values);
  double corpusLikelihood = likelihood(lambdas, mu, corpus, crf);
  double gradTimesDir = 0;
  for(unsigned i = 0; i < gradientVector.size(); i++)
    gradTimesDir += -gradientVector[i] * gradientVector[i];

  double left = likelihood(lambdas + direction.lambda_values, mu + direction.mu_values, corpus, crf);
  double right = corpusLikelihood + alpha * t * gradTimesDir;
  while(left > right) {
    left = likelihood(lambdas + direction.lambda_values, mu + direction.mu_values, corpus, crf);
    right = corpusLikelihood + alpha * t * gradTimesDir;
    t = beta * t;
  }
  std::cerr << "Step size " << t << std::endl;
  return t;
}

template<class CRF, template <typename> class GDCompute>
void trainGradientDescent(CRF& crf, typename CRF::TrainingCorpus& corpus) {
  std::vector<double> lambda(crf.lambda.size());
  for(unsigned i = 0; i < lambda.size(); i++)
    lambda[i] = 1;
  std::vector<double> mu(crf.mu.size());
  for(unsigned i = 0; i < mu.size(); i++)
    mu[i] = 1;
  GDCompute<CRF> gradient;

  for(int k = 0; k < 100; k++) {
    std::cerr << "Iteration " << k << std::endl;
    GradientValues values = gradient.calculate(crf, corpus, lambda, mu);
    std::cerr << "Calculating step size" << std::endl;
    double stepSize = calculateStepSize(values, crf, corpus);

    for(unsigned i = 0; i < crf.lambda.size(); i++)
      crf.lambda[i] += -values.lambda_values[i] * stepSize;

    for(unsigned i = 0; i < crf.mu.size(); i++)
      crf.mu[i] += -values.mu_values[i] * stepSize;
  }

  crf.lambda = lambda;
  crf.mu = mu;
}

#endif
