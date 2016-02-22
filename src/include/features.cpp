#include"features.hpp"

static std::array<std::string, PhoneticFeatures::size> initializeNames() {
  return std::array<std::string, PhoneticFeatures::size>
    {{
        "trans-pitch",
        "trans-ctx",
        "trans-mfcc",
        "state-pitch",
        "state-energy",
        "state-duration"
          }};
}

const std::array<std::string, PhoneticFeatures::size> PhoneticFeatures::Names = initializeNames();

