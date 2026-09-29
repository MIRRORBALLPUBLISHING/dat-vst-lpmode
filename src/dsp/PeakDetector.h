#pragma once

#include <array>
#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>

namespace ldat {

// per channel block peak, audio thread writes, ui thread takes and clears.
// a lost update between the two is harmless for a meter.
class PeakDetector {
 public:
  static constexpr int kMaxChannels = 2;

  PeakDetector() {
    for (auto& p : peak_) p.store(0.0f);
  }

  void Push(const juce::AudioBuffer<float>& buf) {
    const int nch = juce::jmin(buf.getNumChannels(), kMaxChannels);
    for (int ch = 0; ch < nch; ++ch) {
      const float m = buf.getMagnitude(ch, 0, buf.getNumSamples());
      auto& slot = peak_[static_cast<size_t>(ch)];
      if (m > slot.load(std::memory_order_relaxed)){
         slot.store(m, std::memory_order_relaxed);
      }
    }
  }

  // @brief Return the peak since the last call and clear it.
  float Take(int ch) {
    return peak_[static_cast<size_t>(ch)].exchange(0.0f, std::memory_order_relaxed);
  }

 private:
  std::array<std::atomic<float>, kMaxChannels> peak_;
};

}  // namespace ldat
