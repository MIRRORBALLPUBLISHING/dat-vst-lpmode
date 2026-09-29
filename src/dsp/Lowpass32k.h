#pragma once

#include <memory>
#include <vector>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace ldat {

// DAT option 2 32kHz anti-alias stage. v1 is a steep LPF at the current
// rate only. v2 will resample to 32kHz and back behind the same interface.
class Lowpass32k {
 public:
  void Prepare(double sample_rate, int max_block, int num_channels);
  void Reset();
  void Process(juce::AudioBuffer<float>& buf);

  // @brief Samples of delay this stage adds, 0 for the IIR version.
  int GetLatency() const { return 0; }

  // @brief False when the host rate is already <= 32kHz.
  bool IsActive() const { return active_; }

 private:
  // IIR::Filter is not assignable, so hold them by pointer
  using Filter = juce::dsp::IIR::Filter<float>;
  using FilterChain = std::vector<std::unique_ptr<Filter>>;
  std::vector<FilterChain> stages_;   // [channel][biquad]
  bool active_ = false;
};

}  // namespace ldat
