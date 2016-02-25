#ifndef __FEATURES_H__
#define __FEATURES_H__

#include<cmath>
#include<tuple>

#include"crf.hpp"
#include"speech_synthesis.hpp"

using namespace tool;

struct Pitch {
  cost operator()(const PhonemeInstance&,
                  const PhonemeInstance& prev,
                  const PhonemeInstance& next) {
    return (prev.id != next.id) * std::abs(prev.pitch_contour[1] - next.pitch_contour[0]);
  }
};

struct LeftContext {
  cost operator()(const PhonemeInstance&,
                  const PhonemeInstance& prev,
                  const PhonemeInstance& next) {
    return (prev.id != next.id) * (prev.label != next.ctx_left);
  }
};

struct EnergyTrans {
  cost operator()(const PhonemeInstance&,
                  const PhonemeInstance& prev,
                  const PhonemeInstance& next) {
    return prev.energy - next.energy;
  }
};

struct MFCCDist {
  cost operator()(const PhonemeInstance&,
                  const PhonemeInstance& prev,
                  const PhonemeInstance& next) {
    if(prev.id == next.id)
      return 0;
    const MfccArray& mfcc1 = prev.last().mfcc;
    const MfccArray& mfcc2 = next.first().mfcc;
    cost result = 0;
    for (unsigned i = 0; i < mfcc1.size(); i++) {
      cost diff = mfcc1[i] - mfcc2[i];
      result += diff * diff;
    }
    return std::sqrt(result);
  }
};

struct MFCCDistL1 {
  cost operator()(const PhonemeInstance&,
                  const PhonemeInstance& prev,
                  const PhonemeInstance& next) {
    if(prev.id == next.id)
      return 0;
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
  cost operator()(const PhonemeInstance& x,
                  const PhonemeInstance&,
                  const PhonemeInstance& next) {
    return x.pitch_contour.diff(next.pitch_contour);
  }
};

struct EnergyState {
  cost operator()(const PhonemeInstance& x,
                  const PhonemeInstance&,
                  const PhonemeInstance& next) {
    return std::abs(next.energy - x.energy);
  }
};

struct Duration {
  cost operator()(const PhonemeInstance& x,
                  const PhonemeInstance&,
                  const PhonemeInstance& next) {
    return std::abs(x.log_duration - next.log_duration);
  }
};

struct Baseline {
  cost operator()(const PhonemeInstance&,
                  const PhonemeInstance& prev,
                  const PhonemeInstance& next) {
    return (prev.id + 1 == next.id) ? 0 : 1;
  }
};

struct PhoneticFeatures {
  static constexpr auto Functions =
    std::make_tuple(Pitch{},
                    LeftContext{},
                    MFCCDist{},
                    PitchState{},
                    EnergyState{},
                    Duration{});

  static constexpr auto size = std::tuple_size<decltype(Functions)>::value;
  static const std::array<std::string, size> Names;

  static_assert( std::tuple_size<decltype(Functions)>::value ==
                 std::tuple_size<decltype(Functions)>::value,
                 "Features and names have same sizes");
};

struct BaselineFeatures {
  static constexpr auto Functions =
    std::make_tuple(Baseline{});

  static constexpr auto Names =
    std::make_tuple("baseline");

  static constexpr auto size = std::tuple_size<decltype(Functions)>::value;
};

typedef CRandomField<PhonemeAlphabet, PhonemeInstance, PhoneticFeatures> CRF;
typedef CRandomField<PhonemeAlphabet, PhonemeInstance, BaselineFeatures> BaselineCRF;

#endif
