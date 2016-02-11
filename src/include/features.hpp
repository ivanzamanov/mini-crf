#ifndef __FEATURES_H__
#define __FEATURES_H__

#include<cmath>
#include"crf.hpp"
#include"speech_synthesis.hpp"

using namespace tool;

static constexpr int TOTAL_FEATURES = 6;
struct FeatureStats : public std::array<cost, TOTAL_FEATURES> {
  FeatureStats(): std::array<cost, TOTAL_FEATURES>() {
    for(unsigned i = 0; i < size(); i++)
      (*this)[i] = 0;
  }
};
struct PathFeatureStats : public std::vector<FeatureStats> {
  FeatureStats& get(int pos) {
    resize(std::max(pos + 1, (int) size()));
    return (*this)[pos];
  }
};

struct Features {
  static cost Pitch(const PhonemeInstance& prev,
                    const PhonemeInstance& next,
                    int,
                    const vector<PhonemeInstance>&) {
    return std::abs(prev.pitch_contour[1] - next.pitch_contour[0]);
  }

  static cost LeftContext(const PhonemeInstance& prev,
                          const PhonemeInstance& next,
                          int,
                          const vector<PhonemeInstance>&) {
    return prev.label != next.ctx_left;
  }

  static cost EnergyTrans(const PhonemeInstance& prev,
                          const PhonemeInstance& next,
                          int,
                          const vector<PhonemeInstance>&) {
    return prev.energy - next.energy;
  }

  static cost MFCCDist(const PhonemeInstance& prev,
                       const PhonemeInstance& next,
                       int, const vector<PhonemeInstance>&) {
    const MfccArray& mfcc1 = prev.last().mfcc;
    const MfccArray& mfcc2 = next.first().mfcc;
    cost result = 0;
    for (unsigned i = 0; i < mfcc1.size(); i++) {
      cost diff = mfcc1[i] - mfcc2[i];
      result += diff * diff;
    }
    return std::sqrt(result);
  }

  static cost MFCCDistL1(const PhonemeInstance& prev,
                         const PhonemeInstance& next,
                         int,
                         const vector<PhonemeInstance>&) {
    const MfccArray& mfcc1 = prev.last().mfcc;
    const MfccArray& mfcc2 = next.first().mfcc;
    mfcc_t result = 0;
    for (unsigned i = 0; i < mfcc1.size(); i++) {
      mfcc_t diff = mfcc1[i] - mfcc2[i];
      result += std::abs(diff);
    }
    return result;
  }

  static cost PitchState(const PhonemeInstance& p1,
                         int pos,
                         const vector<PhonemeInstance>& x) {
    const PhonemeInstance& p2 = x[pos];
    return p1.pitch_contour.diff(p2.pitch_contour);
  }

  static cost EnergyState(const PhonemeInstance& p1,
                          int pos,
                          const vector<PhonemeInstance>& x) {
    return std::abs(p1.energy - x[pos].energy);
  }

  static cost Duration(const PhonemeInstance& prev,
                       int pos,
                       const vector<PhonemeInstance>& x) {
    const PhonemeInstance& next = x[pos];
    return std::abs(prev.log_duration - next.log_duration);
  }

  // ---------
  static cost BaselineFunction(const PhonemeInstance& prev,
                               const PhonemeInstance& next,
                               int,
                               const vector<PhonemeInstance>&) {
    return (prev.id + 1 == next.id) ? 0 : 1;
  }
};

struct PhoneticFeatures;
typedef CRandomField<PhonemeAlphabet, PhonemeInstance, PhoneticFeatures> CRF;

struct PhoneticFeatures {
  typedef PathFeatureStats Stats;

  typedef cost (*_EdgeFeature)(const PhonemeInstance&, const PhonemeInstance&, int, const vector<PhonemeInstance>&);
  typedef cost (*_VertexFeature)(const PhonemeInstance&, int, const vector<PhonemeInstance>&);

  static const int ESIZE = 3;
  static const int VSIZE = 3;

  // Keep order in sync with features.cl:transition
  const std::string enames[ESIZE] = {
    "trans-pitch",
    "trans-mfcc",
    "trans-ctx"
  };

  // Keep order in sync with features.cl:transition
  const std::string vnames[VSIZE] = {
    "state-duration",
    "state-pitch",
    "state-energy"
  };

#define INVOKE_TR_ST(idx, func) result += (stats[idx] = (lambda[idx] * func(src, dest, pos, x)))
  cost invoke_transition_stats(const PhonemeInstance& src,
                               const PhonemeInstance& dest,
                               int pos, const vector<PhonemeInstance>& x,
                               const vector<coefficient>& lambda,
                               const vector<coefficient>&,
                               FeatureStats& stats) const {
    cost result = 0;
    INVOKE_TR_ST(0, Features::Pitch);
    INVOKE_TR_ST(1, Features::MFCCDist);
    INVOKE_TR_ST(2, Features::LeftContext);
    return result;
  }

#define INVOKE_STATE_ST(idx, func) result += (stats[idx + ESIZE] = (mu[idx] * func(dest, pos, x)))
  cost invoke_state_stats(const PhonemeInstance&,
                          const PhonemeInstance& dest,
                          int pos, const vector<PhonemeInstance>& x,
                          const vector<coefficient>&,
                          const vector<coefficient>& mu,
                          FeatureStats& stats) const {
    cost result = 0;
    INVOKE_STATE_ST(0, Features::Duration);
    INVOKE_STATE_ST(1, Features::PitchState);
    INVOKE_STATE_ST(2, Features::EnergyState);
    return result;
  }

#define INVOKE_TR(idx, func) result += lambda[idx] * func(src, dest, pos, x)
  cost invoke_transition(const PhonemeInstance& src,
                         const PhonemeInstance& dest,
                         int pos, const vector<PhonemeInstance>& x,
                         const vector<coefficient>& lambda,
                         const vector<coefficient>&) const {
    cost result = 0;
    INVOKE_TR(0, Features::Pitch);
    INVOKE_TR(1, Features::MFCCDist);
    INVOKE_TR(2, Features::LeftContext);
    return result;
  }

#define INVOKE_STATE(idx, func) result += mu[idx] * func(dest, pos, x)
  cost invoke_state(const PhonemeInstance&,
                    const PhonemeInstance& dest,
                    int pos, const vector<PhonemeInstance>& x,
                    const vector<coefficient>&,
                    const vector<coefficient>& mu) const {
    cost result = 0;
    INVOKE_STATE(0, Features::Duration);
    INVOKE_STATE(1, Features::PitchState);
    INVOKE_STATE(2, Features::EnergyState);
    return result;
  }
};

struct BaselineFeatures;
typedef CRandomField<PhonemeAlphabet, PhonemeInstance, BaselineFeatures> BaselineCRF;

struct BaselineFeatures {
  typedef PathFeatureStats Stats;

  typedef cost (*_EdgeFeature)(const PhonemeInstance&, const PhonemeInstance&, int, const vector<PhonemeInstance>&);
  typedef cost (*_VertexFeature)(const PhonemeInstance&, int, const vector<PhonemeInstance>&);

  const int ESIZE = 1;
  const int VSIZE = 0;

  const std::string enames[1] = {
    "trans-baseline"
  };

  const std::string vnames[0] = {};
  const _VertexFeature g[0] = { };

  cost invoke_transition_stats(const PhonemeInstance& src,
                               const PhonemeInstance& dest,
                               int pos, const vector<PhonemeInstance>& x,
                               const vector<coefficient>& lambda,
                               const vector<coefficient>& mu,
                               FeatureStats&) const {
    return invoke_transition(src, dest, pos, x, lambda, mu);
  }

  cost invoke_state_stats(const PhonemeInstance& src,
                          const PhonemeInstance& dest,
                          int pos, const vector<PhonemeInstance>& x,
                          const vector<coefficient>& lambda,
                          const vector<coefficient>& mu,
                          FeatureStats&) const {
    return invoke_state(src, dest, pos, x, lambda, mu);
  }

  cost invoke_transition(const PhonemeInstance& src,
                         const PhonemeInstance& dest,
                         int pos, const vector<PhonemeInstance>& x,
                         const vector<coefficient>& lambda,
                         const vector<coefficient>&) const {
    cost result = 0;
    INVOKE_TR(0, Features::BaselineFunction);
    return result;
  }

  cost invoke_state(const PhonemeInstance&,
                    const PhonemeInstance&,
                    int, const vector<PhonemeInstance>&,
                    const vector<coefficient>&,
                    const vector<coefficient>&) const {
    return 0;
  }
};

#endif
