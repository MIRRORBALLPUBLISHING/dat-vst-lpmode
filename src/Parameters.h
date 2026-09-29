#pragma once

#include <cmath>
#include "dsp/DatNonlinear.h"

// parameter ids shared by processor and editor
namespace ldat {

inline constexpr const char* kParamBypass = "bypass";
inline constexpr const char* kParamInGain = "inGain";
inline constexpr const char* kParamLp32k = "lp32k";
inline constexpr const char* kParamEmphasis = "emphasis";
inline constexpr const char* kParamThreshold = "threshold";
inline constexpr const char* kParamOutGain = "outGain";

inline constexpr float kGainMinDb = -24.0f;
inline constexpr float kGainMaxDb = 24.0f;

/**
 * threshold = level of the top of the linear region, dBFS. one more bit is
 * dropped per octave above it. -36.1 dBFS is 512 LSB, the standard IEC 61119
 * table. the floor -78.3 is 4 LSB, 2 significant bits left at full scale.
 * held in dB so sliders and automation move in octaves, not in LSB
 **/
inline constexpr float kOctaveDb = 6.0205999f;
inline constexpr float kThresholdMinDb = -13.0f * kOctaveDb;   // 4 LSB
inline constexpr float kThresholdMaxDb = -6.0f * kOctaveDb;    // 512 LSB
inline constexpr float kThresholdDefaultDb = kThresholdMaxDb;

// @brief Convert threshold dBFS to the LSB count the quantizer takes.
inline int ThresholdFromDb(float db) {
  const float t = std::round(32768.0f * std::pow(10.0f, db / 20.0f));
  if (t < static_cast<float>(DatNonlinear::kMinThreshold)) return DatNonlinear::kMinThreshold;
  if (t > static_cast<float>(DatNonlinear::kMaxThreshold)) return DatNonlinear::kMaxThreshold;
  return static_cast<int>(t);
}

}  // namespace ldat
