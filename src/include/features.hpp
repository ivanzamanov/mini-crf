#ifndef __FEATURES_H__
#define __FEATURES_H__

#include<cmath>
#include<tuple>

#include"crf.hpp"
#include"speech_synthesis.hpp"

using namespace tool;

struct Pitch {
  static const bool is_state = false;
  cost operator()(const PhonemeInstance& prev,
                  const PhonemeInstance& next) const {
    return std::abs(prev.pitch_contour[1] - next.pitch_contour[0]);
  }
};

struct LeftContext {
  static const bool is_state = false;
  cost operator()(const PhonemeInstance& prev,
                  const PhonemeInstance& next) const {
    return prev.label != next.ctx_left;
  }
};

struct EnergyTrans {
  static const bool is_state = false;
  cost operator()(const PhonemeInstance& prev,
                  const PhonemeInstance& next) const {
    return prev.energy - next.energy;
  }
};

struct MFCCDist {
  static const bool is_state = false;
  cost operator()(const PhonemeInstance& prev,
                  const PhonemeInstance& next) const {
    const MfccArray& mfcc1 = prev.last().mfcc;
    const MfccArray& mfcc2 = next.first().mfcc;
    cost result = 0;
    for (auto i = 0u; i < mfcc1.size() / 2; i++) {
      auto diff = mfcc1[i] - mfcc2[i];
      result += diff * diff;
    }
    return std::sqrt(result);
  }
};

struct MFCCDistL1 {
  static const bool is_state = false;
  cost operator()(const PhonemeInstance& prev,
                  const PhonemeInstance& next) const {
    const MfccArray& mfcc1 = prev.last().mfcc;
    const MfccArray& mfcc2 = next.first().mfcc;
    mfcc_t result = 0;
    for (unsigned i = 0; i < mfcc1.size(); i++) {
      mfcc_t diff = mfcc1[i] - mfcc2[i];
      result += std::abs(diff);
    }
    return result;
  }
};

struct PitchState {
  static const bool is_state = true;
  cost operator()(const PhonemeInstance& x,
                  const PhonemeInstance& y) const {
    return x.pitch_contour.diff(y.pitch_contour);
  }
};

struct EnergyState {
  static const bool is_state = true;
  cost operator()(const PhonemeInstance& x,
                  const PhonemeInstance& y) const {
    return std::abs(y.energy - x.energy);
  }
};

struct Duration {
  static const bool is_state = false;
  cost operator()(const PhonemeInstance& x,
                  const PhonemeInstance& y) const {
    return std::abs(x.log_duration - y.log_duration);
  }
};

struct Baseline {
  static const bool is_state = false;
  cost operator()(const PhonemeInstance& prev,
                  const PhonemeInstance& next) const {
    return (prev.id + 1 == next.id) ? 0 : 1;
  }
};

struct PhoneticFeatures {
  typedef decltype(std::make_tuple(Pitch{},
                                   LeftContext{},
                                   MFCCDist{},
                                   PitchState{},
                                   EnergyState{},
                                   Duration{})) FunctionsType;
  static const FunctionsType Functions;

  static constexpr auto size = std::tuple_size<decltype(Functions)>::value;
  static const std::array<std::string, size> Names;

  static_assert( std::tuple_size<decltype(Functions)>::value ==
                 std::tuple_size<decltype(Functions)>::value,
                 "Features and names have same sizes");
};

struct BaselineFeatures {
  typedef decltype(std::make_tuple(Baseline{})) FunctionsType;
  static const FunctionsType Functions;

  static constexpr auto size = std::tuple_size<decltype(Functions)>::value;
  static const std::array<std::string, size> Names;
};

typedef CRandomField<PhonemeAlphabet, PhonemeInstance, PhoneticFeatures> CRF;
typedef CRandomField<PhonemeAlphabet, PhonemeInstance, BaselineFeatures> BaselineCRF;

#endif
