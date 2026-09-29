#pragma once

#include <memory>
#include <vector>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace ldat {

/**
 * 50/15us CD/DAT type emphasis. H(s) = (1 + s*T1) / (1 + s*T2), T1 = 50us,
 * T2 = 15us, bilinear transformed to a 1st order IIR at the current rate.
 * pre goes before the 12bit stage, de goes after it.
 **/
class Emphasis {
 public:
  void Prepare(double sample_rate, int num_channels);
  void Reset();

  // @brief Apply pre-emphasis (+10.5dB shelf at HF) in place.
  void ProcessPre(juce::AudioBuffer<float>& buf);
  // @brief Apply de-emphasis (inverse of pre) in place.
  void ProcessDe(juce::AudioBuffer<float>& buf);

 private:
  // IIR::Filter is not assignable, so hold them by pointer
  using Filter = juce::dsp::IIR::Filter<float>;
  using FilterList = std::vector<std::unique_ptr<Filter>>;
  static void Run(FilterList& filters, juce::AudioBuffer<float>& buf);

  FilterList pre_;
  FilterList de_;
};

}  // namespace ldat
