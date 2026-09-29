#include "dsp/Lowpass32k.h"

namespace ldat {

namespace {

constexpr float kCutoffHz = 15000.0f;   // just under 32k nyquist
constexpr int kOrder = 8;
constexpr double kTargetRate = 32000.0;

}  // namespace

void Lowpass32k::Prepare(double sample_rate, int /*max_block*/, int num_channels) {
  stages_.clear();
  active_ = sample_rate > kTargetRate;
  if (!active_) return;

  auto coefs = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(kCutoffHz, sample_rate, kOrder);
  stages_.resize(static_cast<size_t>(num_channels));
  for (auto& chain : stages_) {
    chain.clear();
    for (int i = 0; i < coefs.size(); ++i){
       chain.push_back(std::make_unique<Filter>(coefs[i]));
    }
  }
  Reset();
}

void Lowpass32k::Reset() {
  for (auto& chain : stages_)
    for (auto& f : chain) f->reset();
}

void Lowpass32k::Process(juce::AudioBuffer<float>& buf) {
  if (!active_) return;
  const int nch = juce::jmin(buf.getNumChannels(), static_cast<int>(stages_.size()));
  const int n = buf.getNumSamples();
  for (int ch = 0; ch < nch; ++ch) {
    float* p = buf.getWritePointer(ch);
    for (auto& f : stages_[static_cast<size_t>(ch)]) {
      for (int i = 0; i < n; ++i) p[i] = f->processSample(p[i]);
      f->snapToZero();
    }
  }
}

}  // namespace ldat
