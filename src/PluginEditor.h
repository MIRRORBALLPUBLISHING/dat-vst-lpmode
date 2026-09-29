#pragma once

#include <array>
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "ui/AboutOverlay.h"
#include "ui/PotLookAndFeel.h"
#include "ui/SegmentMeter.h"

namespace ldat {

// how close to the top a drag has to land to count as the top. the authentic setting is the very end of the range, too fine to hit by eye otherwise
inline constexpr float kThresholdSnapDb = 1.f;

// @brief Slider that pulls a drag near the top of its range onto the maximum.
class TopSnapSlider : public juce::Slider {
 protected:
  double snapValue(double value, DragMode mode) override {
    // typed values are left alone, only dragging snaps
    if (mode != notDragging && getMaximum() - value <= kThresholdSnapDb) return getMaximum();
    return value;
  }
};

// @brief Component that draws the logo svg and reports clicks on it.
class LogoButton : public juce::Component {
 public:
  LogoButton() {
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
  }

  void SetDrawable(std::unique_ptr<juce::Drawable> d) {
    drawable_ = std::move(d);
    repaint();
  }

  std::function<void()> on_click;

  void paint(juce::Graphics& g) override {
    if (drawable_ != nullptr){
      drawable_->drawWithin(g, getLocalBounds().toFloat(), kPlacement, 1.0f);
    }
  }

  void mouseUp(const juce::MouseEvent& e) override {
    // the artwork is narrower than the column, ignore the empty sides
    if (on_click && DrawnRect().contains(e.position)) {
      on_click();
    }
  }

 private:
  // yBottom drops it to the bottom right of the column, xMid holds the aspect ratio. the svg is wider than it is tall, so x has no slack to give anyway
  static constexpr int kPlacement = juce::RectanglePlacement::xMid | juce::RectanglePlacement::yBottom;

  // where the svg really lands inside the bounds
  juce::Rectangle<float> DrawnRect() const {
    if (drawable_ == nullptr) return {};
    return juce::RectanglePlacement(kPlacement).appliedTo(drawable_->getDrawableBounds(), getLocalBounds().toFloat());
  }

  std::unique_ptr<juce::Drawable> drawable_;
};

class LDatEditor : public juce::AudioProcessorEditor, private juce::Timer {
 public:
  explicit LDatEditor(LDatProcessor& p);
  ~LDatEditor() override;

  void paint(juce::Graphics& g) override;
  void resized() override;

 private:
  using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
  using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

  // per meter, per channel ballistics
  struct MeterState {
    std::array<float, 2> db{-100.0f, -100.0f};
    std::array<int, 2> over_ticks{0, 0};
  };

  void timerCallback() override;
  void ShowAbout();
  void UpdateMeter(PeakDetector& det, MeterState& st, SegmentMeter& meter);
  void SetupSlider(juce::Slider& s, juce::Label& l, const juce::String& text, int box_w, int box_h, PotLookAndFeel& lnf);
  void SetupToggle(juce::ToggleButton& b, const juce::String& text);

  LDatProcessor& processor_;
  std::atomic<float>* threshold_raw_ = nullptr;

  // declared before the sliders so they outlive them. two of them, the threshold box carries a bigger font than the gain boxes
  PotLookAndFeel pot_lnf_;
  PotLookAndFeel threshold_lnf_;

  LogoButton logo_button_;

  SegmentMeter in_meter_{"INPUT"};
  SegmentMeter out_meter_{"OUTPUT"};
  MeterState in_state_;
  MeterState out_state_;

  juce::ToggleButton bypass_;
  juce::Slider in_gain_;
  juce::Label in_gain_label_;
  juce::ToggleButton lp32k_;
  juce::ToggleButton emphasis_;
  TopSnapSlider threshold_;
  juce::Label threshold_label_;
  juce::Slider out_gain_;
  juce::Label out_gain_label_;

  std::unique_ptr<ButtonAttachment> bypass_att_;
  std::unique_ptr<SliderAttachment> in_gain_att_;
  std::unique_ptr<ButtonAttachment> lp32k_att_;
  std::unique_ptr<ButtonAttachment> emphasis_att_;
  std::unique_ptr<SliderAttachment> threshold_att_;
  std::unique_ptr<SliderAttachment> out_gain_att_;

  // declared last so it sits on top of everything else
  AboutOverlay about_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LDatEditor)
};

}  // namespace ldat
