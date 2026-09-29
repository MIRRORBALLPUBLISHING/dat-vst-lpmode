#include "ui/PotLookAndFeel.h"

#include "ui/Fonts.h"

namespace ldat {

namespace {

constexpr int kNumTicks = 11;
constexpr float kBodyRatio = 0.80f;        // body radius over half the bounds
constexpr float kTickFrom = 0.86f;         // tick inner end, same unit
constexpr float kCentreTickFrom = 0.80f;   // middle tick starts closer in
constexpr float kPointerFrom = 0.40f;      // pointer ends, over body radius
constexpr float kPointerTo = 0.88f;
constexpr float kPointerWidth = 0.07f;
constexpr float kTextBoxFontPx = 16.0f;    // value under the knob, default
constexpr int kEditDecimals = 1;   // digits shown when a multi line box is clicked
constexpr float kToggleFontPx = 17.0f;     // Bypass, 32kHz, Emphasis
constexpr int kToggleTextGap = 10;         // between the tick box and the text

// same palette as SegmentMeter
const juce::Colour kRim(0xff080808);
const juce::Colour kBodyHi(0xff2a2a2a);
const juce::Colour kBodyLo(0xff161616);
const juce::Colour kEdge(0xff3a3a3a);
const juce::Colour kTick(0xffb0b0b0);
const juce::Colour kPointer(0xffe8e8f8);
const juce::Colour kText(kUiTextArgb);

}  // namespace

PotLookAndFeel::PotLookAndFeel() : text_box_font_px_(kTextBoxFontPx) {
  setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
  setColour(juce::Slider::textBoxTextColourId, kText);
  setColour(juce::ToggleButton::textColourId, kText);  // toggles take this look and feel, so their text follows too
}

void PotLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float pos, float start_angle, float end_angle, juce::Slider&) {
  const auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
  const auto centre = bounds.getCentre();
  const float outer_r = 0.5f * static_cast<float>(juce::jmin(width, height));
  const float r = outer_r * kBodyRatio;

  // ticks, angles run clockwise from 12 o'clock like the slider's own
  g.setColour(kTick);
  const float tick_w = juce::jmax(1.5f, outer_r * 0.02f);
  for (int i = 0; i < kNumTicks; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(kNumTicks - 1);
    const float a = start_angle + (end_angle - start_angle) * t;
    const float from = (i == kNumTicks / 2 ? kCentreTickFrom : kTickFrom) * outer_r;
    g.drawLine(juce::Line<float>(centre.getPointOnCircumference(from, a),
                                 centre.getPointOnCircumference(outer_r, a)),
               tick_w);
  }

  // body: dark rim, then a slightly lit face, then a thin edge line
  g.setColour(kRim);
  g.fillEllipse(juce::Rectangle<float>(2.0f * r, 2.0f * r).withCentre(centre));
  const float br = r * 0.96f;
  const auto face = juce::Rectangle<float>(2.0f * br, 2.0f * br).withCentre(centre);
  g.setGradientFill(juce::ColourGradient(kBodyHi, centre.x - br * 0.4f, centre.y - br * 0.4f,
                                         kBodyLo, centre.x + br * 0.8f, centre.y + br * 0.8f,
                                         true));
  g.fillEllipse(face);
  g.setColour(kEdge);
  g.drawEllipse(face, 1.0f);

  // pointer
  const float a = start_angle + (end_angle - start_angle) * pos;
  juce::Path p;
  p.startNewSubPath(centre.getPointOnCircumference(r * kPointerFrom, a));
  p.lineTo(centre.getPointOnCircumference(r * kPointerTo, a));
  g.setColour(kPointer);
  g.strokePath(p, juce::PathStrokeType(r * kPointerWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void PotLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool down) {
  // tick box keeps the V4 sizing, which follows the button height. only the
  // label font is ours
  const float h = static_cast<float>(b.getHeight());
  const float tick_w = juce::jmin(15.0f, h * 0.75f) * 1.1f;
  drawTickBox(g, b, 4.0f, (h - tick_w) * 0.5f, tick_w, tick_w, b.getToggleState(), b.isEnabled(), highlighted, down);

  g.setColour(b.findColour(juce::ToggleButton::textColourId));
  g.setFont(fonts::Regular(kToggleFontPx));
  if (!b.isEnabled()) g.setOpacity(0.5f);
  g.drawFittedText(b.getButtonText(), b.getLocalBounds().withTrimmedLeft(juce::roundToInt(tick_w) + kToggleTextGap).withTrimmedRight(2), juce::Justification::centredLeft, 1);
}

juce::Label* PotLookAndFeel::createSliderTextBox(juce::Slider& slider) {
  auto* l = juce::LookAndFeel_V4::createSliderTextBox(slider);
  l->setFont(fonts::Bold(text_box_font_px_));
  // the editor is single line, so a multi line box gets the bare number to type over
  l->onEditorShow = [l, &slider] {
    auto* ed = l->getCurrentTextEditor();
    if (ed == nullptr || !l->getText().containsChar('\n')) return;
    ed->setText(juce::String(slider.getValue(), kEditDecimals), false);
    ed->selectAll();
  };
  return l;
}

}  // namespace ldat
