#include "dsp/DatNonlinear.h"
#include <cmath>

namespace ldat {

namespace {

constexpr int kLinearMax = 511;   // 0..511 pass through
constexpr int kMaxShift = 6;      // 16384..32767 -> /64
constexpr float kFullScale = 32768.0f;

// number of significant bits above the 9bit linear region
int ShiftFor(int x) {
  int s = 0;
  while ((x >> (9 + s)) != 0 && s < kMaxShift) ++s;
  return s;
}

// positive half of RFC3190 table 1: Y = INT(X / 2^s) + s * 0x100
int EncodePos(int x) {
  if (x <= kLinearMax) return x;
  const int s = ShiftFor(x);
  return (x >> s) + (s << 8);
}

// inverse of EncodePos, middle of the interval
int DecodePos(int y) {
  if (y <= kLinearMax) return y;
  const int s = (y >> 8) - 1;   // 0x200.. -> 1, 0x300.. -> 2, ...
  const int base = (y - (s << 8)) << s;
  return base + (1 << (s - 1));
}

}  // namespace

int Encode12(int x16) {
  // negative side is one's complement symmetric:
  // RFC gives Y = INT((X + 1) / 2^s) - (s * 0x100 + 1), same as ~Encode(~X)
  if (x16 < 0) return ~EncodePos(~x16);
  return EncodePos(x16);
}

int Decode16(int y12) {
  if (y12 < 0) return ~DecodePos(~y12);
  return DecodePos(y12);
}

int RegionShift(int abs_x, int threshold) {
  // smallest s with abs_x < threshold << s. threshold >= 4 and abs_x <= 32767
  // so s <= 13 and the shift never overflows
  int s = 0;
  while (abs_x >= (threshold << s)) ++s;
  return s;
}

void DatNonlinear::SetThreshold(int threshold) {
  if (threshold < kMinThreshold) threshold = kMinThreshold;
  if (threshold > kMaxThreshold) threshold = kMaxThreshold;
  threshold_ = threshold;
}

int DatNonlinear::QuantizePos(int x) const {
  const int s = RegionShift(x, threshold_);
  if (s == 0) return x;
  // floor to the region grid, then mid point. (x >> s) << s is at most
  // 32768 - 2^s so adding 2^(s-1) stays below 32768
  return ((x >> s) << s) + (1 << (s - 1));
}

int DatNonlinear::Quantize(int x16) const {
  if (x16 < 0) return ~QuantizePos(~x16);
  return QuantizePos(x16);
}

float DatNonlinear::ProcessSample(float x) const {
  float v = std::round(x * kFullScale);
  if (v > 32767.0f) v = 32767.0f;
  if (v < -32768.0f) v = -32768.0f;
  return static_cast<float>(Quantize(static_cast<int>(v))) / kFullScale;
}

}  // namespace ldat
