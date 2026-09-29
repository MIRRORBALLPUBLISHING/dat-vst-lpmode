#pragma once

#include <cstdint>

// DAT option 2 (LP mode) 12bit nonlinear quantizer, RFC3190 table 1 / IEC 61119.
// no JUCE dependency so it can be unit tested standalone.
namespace ldat {

// @brief Encode a 16bit linear sample into a 12bit nonlinear code.
// @param x16 two's complement sample, -32768..32767
// @return 12bit code, -2048..2047
int Encode12(int x16);

// @brief Decode a 12bit nonlinear code back to 16bit linear.
// @detail RFC3190 defines only the forward table. this picks the middle of the source interval so the reconstruction has no DC bias.
// @param y12 12bit code, -2048..2047
// @return 16bit sample, -32768..32767
int Decode16(int y12);

// @brief Bits dropped for a magnitude, given the top of the linear region.
// @detail 0 below threshold, then +1 per octave: threshold..2T-1 -> 1, 2T..4T-1 -> 2, ... threshold 512 reproduces the RFC3190 table (max 6), threshold 4 goes up to 13. shared by the quantizer and the meter colouring so they never disagree.
// @param abs_x 0..32767
// @param threshold 4..512
int RegionShift(int abs_x, int threshold);

// @brief Float in/out quantizer with an adjustable threshold.
// @detail threshold is the upper bound of the linear region. 512 is the standard table, 7 regions, 12bit. lower values start dropping bits earlier and keep going one bit per octave, so the loud end gets coarser than the standard table allows. the 16bit clip point stays at 32767 whatever the threshold, so level and dynamic range are untouched. threshold 4 leaves 2 significant bits near full scale.
class DatNonlinear {
 public:
  static constexpr int kMinThreshold = 4;
  static constexpr int kMaxThreshold = 512;

  // @brief Set threshold, clamped to 4..512.
  void SetThreshold(int threshold);
  int GetThreshold() const { return threshold_; }

  // @brief Quantize one 16bit sample, mid point of its region.
  // @detail negative side is one's complement symmetric like the RFC table.
  // @param x16 -32768..32767
  int Quantize(int x16) const;

  // @brief Quantize one sample, nominal range -1.0..1.0.
  float ProcessSample(float x) const;

 private:
  int QuantizePos(int x) const;
  int threshold_ = kMaxThreshold;
};

}  // namespace ldat
