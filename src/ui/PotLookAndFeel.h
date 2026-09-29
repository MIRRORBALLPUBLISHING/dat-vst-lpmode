#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ldat {

// text colour shared by the knob labels, the value boxes and the toggles.
// JUCE's own default here is plain white, which reads too hot next to the
// meter. tweak this to taste
inline constexpr juce::uint32 kUiTextArgb = 0xffc0c0c0;

// rotary slider drawn as a black pot: dark bevelled body, white pointer,
// short tick marks around it. all sizes scale with the slider bounds so
// one instance serves both the small gain knobs and the big threshold knob.
// also draws the toggles, so their text uses the same font as everything else.
class PotLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  PotLookAndFeel();

  void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float pos, float start_angle, float end_angle, juce::Slider& slider) override;

  // @brief Draw a toggle the V4 way, but with our font instead of the one V4
  // derives from the button height.
  void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool down) override;

  // @brief Build the value box with a smaller font than the V4 default.
  juce::Label* createSliderTextBox(juce::Slider& slider) override;

  // @brief Set the value box font size, px. Takes effect on the next box built,
  // so call it before the slider gets this look and feel.
  void SetTextBoxFontPx(float px) { text_box_font_px_ = px; }

 private:
  float text_box_font_px_;
};

}  // namespace ldat
