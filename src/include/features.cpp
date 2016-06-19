#include"features.hpp"

static std::array<std::string, PhoneticFeatures::size> initializeNames() {
  return std::array<std::string, PhoneticFeatures::size>
    {{
        "trans-pitch",
          "trans-ctx",
          "trans-mfcc",
          "state-pitch",
          "state-energy",
          "state-duration",
          "trans-baseline"
          }};
}

static std::array<std::string, BaselineFeatures::size> initializeBaselineNames() {
  return std::array<std::string, BaselineFeatures::size>
    {{ "baseline" }};
}

const auto PhoneticFeatures::Names = initializeNames();
const auto PhoneticFeatures::Functions = PhoneticFeatures::FunctionsType();

const auto BaselineFeatures::Names = initializeBaselineNames();
const auto BaselineFeatures::Functions = BaselineFeatures::FunctionsType();
