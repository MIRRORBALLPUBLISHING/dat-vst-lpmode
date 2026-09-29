#include "dsp/Emphasis.h"

namespace ldat {

namespace {

constexpr float kT1 = 50.0e-6f;
constexpr float kT2 = 15.0e-6f;

// bilinear transform of (1 + s*tz) / (1 + s*tp), no prewarp, same as CD players
juce::dsp::IIR::Coefficients<float>::Ptr MakeShelf(double sample_rate, float tz, float tp) {
  const float c = 2.0f * static_cast<float>(sample_rate);
  const float b0 = 1.0f + tz * c;
  const float b1 = 1.0f - tz * c;
  const float a0 = 1.0f + tp * c;
  const float a1 = 1.0f - tp * c;
  return new juce::dsp::IIR::Coefficients<float>(b0, b1, a0, a1);
}

}  // namespace

void Emphasis::Prepare(double sample_rate, int num_channels) {
  pre_.clear();
  de_.clear();
  auto pre = MakeShelf(sample_rate, kT1, kT2);
  auto de = MakeShelf(sample_rate, kT2, kT1);
  for (int ch = 0; ch < num_channels; ++ch) {
    pre_.push_back(std::make_unique<Filter>(pre));
    de_.push_back(std::make_unique<Filter>(de));
  }
  Reset();
}

void Emphasis::Reset() {
  for (auto& f : pre_) f->reset();
  for (auto& f : de_) f->reset();
}

void Emphasis::ProcessPre(juce::AudioBuffer<float>& buf) { Run(pre_, buf); }
void Emphasis::ProcessDe(juce::AudioBuffer<float>& buf) { Run(de_, buf); }

void Emphasis::Run(FilterList& filters, juce::AudioBuffer<float>& buf) {
  const int nch = juce::jmin(buf.getNumChannels(), static_cast<int>(filters.size()));
  const int n = buf.getNumSamples();
  for (int ch = 0; ch < nch; ++ch) {
    auto& f = *filters[static_cast<size_t>(ch)];
    float* p = buf.getWritePointer(ch);
    for (int i = 0; i < n; ++i) p[i] = f.processSample(p[i]);
    f.snapToZero();
  }
}

}  // namespace ldat
