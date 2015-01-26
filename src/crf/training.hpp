#ifndef __CRF_TRAINING_H__
#define __CRF_TRAINING_H__

#include"crf.hpp"

void trainNaive(CRandomField& crf, Corpus& corpus) {

}

void trainIIS(CRandomField& crf, Corpus& corpus) {
  // Edge feature updates
  for (int k = 0; k < crf.lambda.length(); k++) {
    
  }

  // Vertex feature updates
  for (int k = 0; k < crf.mu.length(); k++) {
    
  }
}

#endif
