#pragma once

#include <array>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace ldat {

// 12 segment L/R peak meter with OVER, modeled on the TCD-D8 LCD.
// caller feeds dB values, ballistics live in the editor timer.
class SegmentMeter : public juce::Component {
 public:
  static constexpr int kNumSegments = 12;
  static constexpr int kNumChannels = 2;
  static constexpr std::array<float, kNumSegments> kSegmentDb = {-50.0f, -40.0f, -30.0f, -24.0f, -20.0f, -16.0f, -12.0f, -8.0f, -6.0f, -4.0f, -2.0f, 0.0f,};

  explicit SegmentMeter(const juce::String& title);

  // @brief Set displayed level for one channel, dBFS, and the OVER flag.
  void SetLevel(int ch, float db, bool over);

  // @brief Enable per segment colouring by quantizer region for this threshold.
  // @detail 0 disables colouring, lit segments are plain white then.
  void SetThreshold(int threshold);

  // called when either OVER box is clicked, owner clears its hold state
  std::function<void()> on_over_reset;

  void paint(juce::Graphics& g) override;
  void mouseDown(const juce::MouseEvent& e) override;

 private:
  // OVER box for one channel, shared by paint and hit test
  juce::Rectangle<int> OverRect(int ch) const;
  // rows for L and R below the scale, before the OVER box is cut off
  juce::Rectangle<int> ChannelRow(int ch) const;

  juce::String title_;
  int threshold_ = 0;
  std::array<float, kNumChannels> level_db_{-100.0f, -100.0f};
  std::array<bool, kNumChannels> over_{false, false};
};

}  // namespace ldat
