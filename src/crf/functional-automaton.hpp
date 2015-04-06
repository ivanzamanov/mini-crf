#ifndef __FUNCTIONAL_AUTOMATON_H__
#define __FUNCTIONAL_AUTOMATON_H__

#include<cstring>
#include<iostream>
#include<algorithm>

#include"util.hpp"
#include"../dot/dot.hpp"

using std::vector;

template<class CRF>
void max_path(const vector<Input>& x, CRF& crf, const vector<double>& lambda, const vector<double>& mu, vector<int>* max_path) {
  norm_factor(x, crf, lambda, mu, max_path);
}

template<class CRF>
double norm_factor(const vector<Input>& x, CRF& crf, const vector<double>& lambda, const vector<double>& mu) {
  return norm_factor(x, crf, lambda, mu, 0);
}

template<class CRF>
double norm_factor(const vector<Input>& x, CRF& crf, const vector<double>& lambda, const vector<double>& mu, vector<int>* max_path) {
  DEBUG(DotPrinter printer("automaton.dot");
        printer.start();
        )

  int alphabet_len = crf.label_alphabet.phonemes.length;
  int autom_size = alphabet_len * x.size() + 2;
  double* table = new double[autom_size];

  // Will need for intermediate computations
  double* tr_values = new double[alphabet_len];
  // A slice of the table, actually...
  double* child_values;
  // Transitions q,i,y -> f
  int index = autom_size - 1;
  table[index] = 1;
  index--;

  DEBUG(printer.node(index, -2);)

  vector<int>* paths = 0;
  if(max_path) {
    paths = new vector<int>[alphabet_len];
  }

  int pos = x.size() - 1;
  // transitions to final state
  for(int dest = alphabet_len - 1; dest >= 0; dest--, index--) {
    // value of the last "column" of states
    bool is_allowed = crf.label_alphabet.allowedState(dest, x[pos]);
    table[index] = is_allowed ? 1 : std::numeric_limits<double>::infinity();
    DEBUG(
          printer.node(index, dest);
          printer.edge(index, autom_size - 1, ' ', 1);
          );
  }
  
  // backwards, for every zero-based position in
  // the input sequence except the last one...
  for(pos--; pos >= 0; pos--) {
    // for every possible label...
    for(int src = alphabet_len - 1; src >= 0; src--, index--) {
      // Short-circuit if different phoneme types
      if(crf.label_alphabet.allowedState(src, x[pos])) {
        int child = child_index(pos + 1, alphabet_len);
      
        for(int dest = alphabet_len - 1; dest >= 0; dest--) {
          double tr_value;
          tr_value = transition_value(crf, lambda, mu, x, src, dest, pos);
          tr_values[dest] = tr_value;

          DEBUG(printer.edge(index, child + dest, ' ', tr_value));
        }

        child_values = table + child;
        // tr_value is the power of the transition' actual value
        double tr_value = tr_values[0];
        // child_value is the log of the child's actual value
        double child_value = child_values[0];
        table[index] = tr_value + child_value;
        tr_values[0] = tr_value + child_value;

        for(int dest = alphabet_len - 1; dest > 0; dest--) {
          //double increment = util::sum(tr_value, child_value);
          //double d = util::sum(increment, table[index]);
          tr_value = tr_values[dest];
          child_value = child_values[dest];
          table[index] = std::min(tr_value + child_value, table[index]);
          tr_values[dest] = tr_value + child_value;
        }

        DEBUG(printer.node(index, table[index]));

        // This, however, should never be 0
        if(table[index] == 0 || std::isinf(table[index])) {
           std::cerr << table[index] << " at " << index << std::endl;
        }

        if(max_path) {
          double* max = std::min_element(tr_values, tr_values + alphabet_len);
          paths[src].push_back(max - tr_values);
        }
      } else {
        table[index] = std::numeric_limits<double>::infinity();
        if(max_path) {
          paths[src].push_back(-1);
        }
      }
      DEBUG(std::cerr << "p=" << pos << ",s=" << src << ",d=" << *(paths[src].rbegin()) << std::endl);
    }
  }

  DEBUG(printer.node(index, -1));

  int child = child_index(0, alphabet_len);
  double state_val = state_value(crf, mu, x, 0, 0);
  int src = alphabet_len - 1;
  pos = 0;
  bool is_allowed = crf.label_alphabet.allowedState(src, x[pos]);
  state_val = is_allowed ? state_value(crf, mu, x, src, 0) : std::numeric_limits<double>::infinity();
  table[index] = state_val + table[child + src];
  tr_values[src] = state_val + table[child + src];

  for(src = alphabet_len - 2; src >= 0; src--, index--) {
    is_allowed = crf.label_alphabet.allowedState(src, x[pos]);
    state_val = is_allowed ? state_value(crf, mu, x, src, 0) : std::numeric_limits<double>::infinity();

    double increment = state_val + table[child + src];
    tr_values[src] = increment;
    //table[index] = util::sum(table[index], increment);
    table[index] = std::min(table[index], increment);

    DEBUG(printer.edge(index, child + src, ' ', increment));
  }

  if(max_path) {
    double* max = std::min_element(tr_values, tr_values + alphabet_len);
    int state = max - tr_values;
    max_path->push_back(state);
    DEBUG(std::cerr << "Best starts at " << max - tr_values << std::endl);
    for(int i = x.size() - 2; i >= 0; i--) {
      state = paths[state][i];
      max_path->push_back(state);
    }
    delete[] paths;
  }

  double denom = table[0];
  delete[] table;
  DEBUG(printer.end());
  return denom;
}

#endif
