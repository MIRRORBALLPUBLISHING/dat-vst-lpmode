// unit test for the RFC3190 table, no framework, exit code != 0 on failure
#include <cstdio>
#include <cstdlib>
#include "Parameters.h"
#include "dsp/DatNonlinear.h"

namespace {

int g_failures = 0;

void ExpectEq(int got, int want, const char* what) {
  if (got != want) {
    std::printf("FAIL %s: got %d want %d\n", what, got, want);
    ++g_failures;
  }
}

void ExpectTrue(bool cond, const char* what) {
  if (!cond) {
    std::printf("FAIL %s\n", what);
    ++g_failures;
  }
}

struct Pair { int x16; int y12; };

// every interval edge from RFC3190 table 1
constexpr Pair kEdges[] = {
    {32767, 2047}, {16384, 1792},
    {16383, 1791}, {8192, 1536},
    {8191, 1535},  {4096, 1280},
    {4095, 1279},  {2048, 1024},
    {2047, 1023},  {1024, 768},
    {1023, 767},   {512, 512},
    {511, 511},    {0, 0},
    {-1, -1},      {-512, -512},
    {-513, -513},  {-1024, -768},
    {-1025, -769}, {-2048, -1024},
    {-2049, -1025}, {-4096, -1280},
    {-4097, -1281}, {-8192, -1536},
    {-8193, -1537}, {-16384, -1792},
    {-16385, -1793}, {-32768, -2048},
};

void TestTableEdges() {
  char buf[64];
  for (const auto& p : kEdges) {
    std::snprintf(buf, sizeof(buf), "encode(%d)", p.x16);
    ExpectEq(ldat::Encode12(p.x16), p.y12, buf);
  }
}

void TestMonotonicAndRange() {
  int prev = ldat::Encode12(-32768);
  for (int x = -32767; x <= 32767; ++x) {
    const int y = ldat::Encode12(x);
    ExpectTrue(y >= -2048 && y <= 2047, "encode range");
    ExpectTrue(y >= prev, "encode monotonic");
    prev = y;
  }
}

void TestDecodeRoundTrip() {
  // decode(encode(x)) must land back in the same 12bit bucket, and the
  // reconstruction error must not exceed half the bucket width
  for (int x = -32768; x <= 32767; ++x) {
    const int y = ldat::Encode12(x);
    const int r = ldat::Decode16(y);
    ExpectTrue(r >= -32768 && r <= 32767, "decode range");
    ExpectEq(ldat::Encode12(r), y, "re-encode of decoded");
    // error is at most half the bucket width (max bucket = 64)
    const int err = r > x ? r - x : x - r;
    ExpectTrue(err <= 32, "decode error within half bucket");
  }
  // spot checks of the mid point rule
  ExpectEq(ldat::Decode16(0x300), 1026, "decode 0x300");   // 1024..1027
  ExpectEq(ldat::Decode16(0x200), 513, "decode 0x200");    // 512..513, mid rounds up
  ExpectEq(ldat::Decode16(0x7FF), 32736, "decode 0x7FF");  // 32704..32767
  ExpectEq(ldat::Decode16(0x1FF), 511, "decode 0x1FF");
  ExpectEq(ldat::Decode16(-513), -514, "decode -513");     // mirror of 0x200
  ExpectEq(ldat::Decode16(-2048), -32737, "decode -2048"); // mirror of 0x7FF
}

struct ShiftCase { int x; int threshold; int s; };

constexpr ShiftCase kShiftCases[] = {
    {0, 512, 0},     {511, 512, 0},   {512, 512, 1},    {1023, 512, 1},
    {1024, 512, 2},  {16383, 512, 5}, {16384, 512, 6},  {32767, 512, 6},
    {15, 16, 0},     {16, 16, 1},     {32767, 16, 11},
    {3, 4, 0},       {4, 4, 1},       {16383, 4, 12},   {16384, 4, 13}, {32767, 4, 13},
    {63, 64, 0},     {64, 64, 1},     {32767, 64, 9},
    {99, 100, 0},    {100, 100, 1},   {25599, 100, 8},  {25600, 100, 9}, {32767, 100, 9},
};

void TestRegionShift() {
  char buf[64];
  for (const auto& c : kShiftCases) {
    std::snprintf(buf, sizeof(buf), "RegionShift(%d, %d)", c.x, c.threshold);
    ExpectEq(ldat::RegionShift(c.x, c.threshold), c.s, buf);
  }
}

void TestThresholdStandard() {
  // threshold 512 must be the RFC table exactly, for every 16bit value
  ldat::DatNonlinear q;
  q.SetThreshold(512);
  for (int x = -32768; x <= 32767; ++x) {
    ExpectEq(q.Quantize(x), ldat::Decode16(ldat::Encode12(x)), "threshold 512 == table");
  }
}

void TestThresholdProperties() {
  constexpr int kThresholds[] = {4, 16, 64, 100, 512};
  char buf[64];
  for (const int t : kThresholds) {
    ldat::DatNonlinear q;
    q.SetThreshold(t);
    ExpectEq(q.GetThreshold(), t, "GetThreshold");
    int prev = q.Quantize(-32768);
    for (int x = -32768; x <= 32767; ++x) {
      const int r = q.Quantize(x);
      std::snprintf(buf, sizeof(buf), "threshold %d x %d", t, x);
      ExpectTrue(r >= -32768 && r <= 32767, buf);
      ExpectTrue(r >= prev, buf);
      prev = r;
      // one's complement symmetry, same as the RFC table
      if (x >= 0) ExpectEq(q.Quantize(~x), ~r, buf);
      // error at most half the region width
      const int ax = x < 0 ? ~x : x;
      const int half = (1 << ldat::RegionShift(ax, t)) / 2;
      const int err = r > x ? r - x : x - r;
      ExpectTrue(err <= half, buf);
    }
  }
}

void TestThresholdLevel() {
  ldat::DatNonlinear q;
  q.SetThreshold(512);
  // small signal passes through the linear region unchanged
  const float small = 100.0f / 32768.0f;
  ExpectTrue(q.ProcessSample(small) == small, "threshold 512 linear region");
  // a large sample loses its low bits
  const float big = 20000.0f / 32768.0f;
  const float out = q.ProcessSample(big) * 32768.0f;
  ExpectTrue(out >= 19968.0f && out <= 20031.0f, "threshold 512 coarse region");

  q.SetThreshold(64);
  // 100 LSB is now in region s=1: 100 -> 101 (mid of 100..101)
  ExpectEq(static_cast<int>(q.ProcessSample(small) * 32768.0f), 101, "threshold 64 small");
  // clip point does not move: full scale stays within the top region
  ExpectTrue(q.ProcessSample(1.0f) >= 0.99f, "threshold 64 full scale");
  ExpectTrue(q.ProcessSample(-1.0f) <= -0.99f, "threshold 64 negative full scale");
  // top region is 512 wide at threshold 64: 32767 -> floor 32256 + 256
  ExpectEq(q.Quantize(32767), 32512, "threshold 64 top");

  // clamping
  q.SetThreshold(1);
  ExpectEq(q.GetThreshold(), 4, "threshold clamp low");
  // threshold 4: top region 16384..32767 in steps of 8192, 2 significant bits
  q.SetThreshold(4);
  ExpectEq(q.Quantize(32767), 24576 + 4096, "threshold 4 top");
  ExpectEq(q.Quantize(16384), 16384 + 4096, "threshold 4 top region low end");
  q.SetThreshold(4096);
  ExpectEq(q.GetThreshold(), 512, "threshold clamp high");
}

void TestThresholdFromDb() {
  // range ends must hit the exact LSB counts, default must be the standard table
  ExpectEq(ldat::ThresholdFromDb(ldat::kThresholdMaxDb), 512, "max dB -> 512");
  ExpectEq(ldat::ThresholdFromDb(ldat::kThresholdDefaultDb), 512, "default dB -> 512");
  ExpectEq(ldat::ThresholdFromDb(ldat::kThresholdMinDb), 4, "min dB -> 4");
  // every octave step lands on a power of two
  for (int k = 2; k <= 9; ++k) {
    const float db = static_cast<float>(k - 15) * ldat::kOctaveDb;
    ExpectEq(ldat::ThresholdFromDb(db), 1 << k, "octave step -> power of two");
  }
  // out of range clamps, never leaves the quantizer's limits
  ExpectEq(ldat::ThresholdFromDb(0.0f), 512, "0 dB clamps to 512");
  ExpectEq(ldat::ThresholdFromDb(-120.0f), 4, "-120 dB clamps to 4");
  // in between is monotonic
  int prev = 4;
  for (float db = ldat::kThresholdMinDb; db <= ldat::kThresholdMaxDb; db += 0.05f) {
    const int t = ldat::ThresholdFromDb(db);
    ExpectTrue(t >= prev && t <= 512, "ThresholdFromDb monotonic");
    prev = t;
  }
}

}  // namespace

int main() {
  TestTableEdges();
  TestMonotonicAndRange();
  TestDecodeRoundTrip();
  TestRegionShift();
  TestThresholdStandard();
  TestThresholdProperties();
  TestThresholdLevel();
  TestThresholdFromDb();
  if (g_failures != 0) {
    std::printf("%d failure(s)\n", g_failures);
    return EXIT_FAILURE;
  }
  std::printf("all tests passed\n");
  return EXIT_SUCCESS;
}
