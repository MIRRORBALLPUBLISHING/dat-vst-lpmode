#include "PluginEditor.h"
#include <cmath>
#include <BinaryData.h>
#include "Parameters.h"
#include "ui/Fonts.h"

namespace ldat {

namespace {

constexpr int kTimerHz = 30;
constexpr float kDecayDbPerSec = 20.0f;
constexpr float kDecayDbPerTick = kDecayDbPerSec / kTimerHz;
constexpr int kOverHoldTicks = 2 * kTimerHz;   // 2 seconds
constexpr float kFloorDb = -100.0f;

constexpr int kWidth = 620;
constexpr int kHeight = 324;
constexpr int kMeterH = 38;
constexpr int kMargin = 10;

// control row: small gain knobs stacked on the left, threshold in the middle, toggles on the right
constexpr int kSmallW = 130;
constexpr int kToggleW = 120;
constexpr int kToggleH = 16;    // size checkbox and label
constexpr int kLabelH = 14;
constexpr float kLabelFontPx = 19.0f;  // Input Gain, Output Gain, Threshold,..

// threshold box holds 2 lines at the right end. taller box = smaller knob, the knob fits in what is left of the column height
constexpr int kThresholdBoxW = 200;   // wide enough to keep the text 2 lines
constexpr int kThresholdBoxH = 40;
constexpr float kThresholdValueFontPx = 18.0f;   // the gain boxes keep the smaller default
constexpr const char* kAuthenticText = "Now Authentic DAT LP mode\n12-bit non-linear";

// gain knob diameter, over what the column would give it
constexpr float kSmallKnobScale = 0.85f;

// the svg strokes are pure black, recoloured on load. tweak this to taste
constexpr juce::uint32 kLogoArgb = 0xff909090;

}  // namespace

LDatEditor::LDatEditor(LDatProcessor& p) : AudioProcessorEditor(p), processor_(p) {
  auto& state = processor_.GetState();

  threshold_raw_ = state.getRawParameterValue(kParamThreshold);

  addAndMakeVisible(in_meter_);
  addAndMakeVisible(out_meter_);

  // OVER click clears the hold for both channels of that meter, shown at once
  auto reset_over = [](MeterState& st, SegmentMeter& m) {
    st.over_ticks = {0, 0};
    for (int ch = 0; ch < 2; ++ch) m.SetLevel(ch, st.db[static_cast<size_t>(ch)], false);
  };
  in_meter_.on_over_reset = [this, reset_over] { reset_over(in_state_, in_meter_); };
  out_meter_.on_over_reset = [this, reset_over] { reset_over(out_state_, out_meter_); };

  auto logo = juce::Drawable::createFromImageData(BinaryData::ldat_svg, BinaryData::ldat_svgSize);
  if (logo != nullptr) logo->replaceColour(juce::Colours::black, juce::Colour(kLogoArgb));
  logo_button_.SetDrawable(std::move(logo));
  logo_button_.on_click = [this] { ShowAbout(); };
  addAndMakeVisible(logo_button_);

  about_.on_close = [this] { about_.setVisible(false); };
  addChildComponent(about_);

  threshold_lnf_.SetTextBoxFontPx(kThresholdValueFontPx);

  SetupToggle(bypass_, "Bypass");
  SetupSlider(in_gain_, in_gain_label_, "Input Gain", 60, 16, pot_lnf_);
  SetupToggle(lp32k_, "32kHz");
  SetupToggle(emphasis_, "Emphasis");
  SetupSlider(threshold_, threshold_label_, "Threshold", kThresholdBoxW, kThresholdBoxH, threshold_lnf_);
  SetupSlider(out_gain_, out_gain_label_, "Output Gain", 60, 16, pot_lnf_);

  bypass_att_ = std::make_unique<ButtonAttachment>(state, kParamBypass, bypass_);
  in_gain_att_ = std::make_unique<SliderAttachment>(state, kParamInGain, in_gain_);
  lp32k_att_ = std::make_unique<ButtonAttachment>(state, kParamLp32k, lp32k_);
  emphasis_att_ = std::make_unique<ButtonAttachment>(state, kParamEmphasis, emphasis_);
  threshold_att_ = std::make_unique<SliderAttachment>(state, kParamThreshold, threshold_);
  out_gain_att_ = std::make_unique<SliderAttachment>(state, kParamOutGain, out_gain_);

  // right end = 512 LSB, the real DAT LP table. say so there instead of dB.
  // UI only, host still sees dB. set after the attachment, it installs its own
  auto* thr = state.getParameter(kParamThreshold);
  threshold_.textFromValueFunction = [thr](double v) {
    const auto db = static_cast<float>(v);
    if (ThresholdFromDb(db) == DatNonlinear::kMaxThreshold) return juce::String(kAuthenticText);
    return thr->getText(thr->convertTo0to1(db), 0) + " dB";
  };
  threshold_.updateText();

  // unit lives in the box, not in the parameter, so the host string stays plain
  auto append_db = [](juce::Slider& s) {
    auto base = std::move(s.textFromValueFunction);
    s.textFromValueFunction = [base](double v) { return base(v) + " dB"; };
    s.updateText();
  };
  append_db(in_gain_);
  append_db(out_gain_);

  setSize(kWidth, kHeight);
  startTimerHz(kTimerHz);
}

LDatEditor::~LDatEditor() {
  in_gain_.setLookAndFeel(nullptr);
  threshold_.setLookAndFeel(nullptr);
  out_gain_.setLookAndFeel(nullptr);
  bypass_.setLookAndFeel(nullptr);
  lp32k_.setLookAndFeel(nullptr);
  emphasis_.setLookAndFeel(nullptr);
}

void LDatEditor::SetupSlider(juce::Slider& s, juce::Label& l, const juce::String& text,
                             int box_w, int box_h, PotLookAndFeel& lnf) {
  // double click returns to the parameter default, set by SliderAttachment
  s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  s.setLookAndFeel(&lnf);
  s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, box_w, box_h);
  addAndMakeVisible(s);
  l.setText(text, juce::dontSendNotification);
  l.setFont(fonts::Regular(kLabelFontPx));
  // the label is not under the pot look and feel, so it needs the colour set here
  l.setColour(juce::Label::textColourId, juce::Colour(kUiTextArgb));
  l.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(l);
}

void LDatEditor::SetupToggle(juce::ToggleButton& b, const juce::String& text) {
  b.setButtonText(text);
  // V4 picks the font off the height, the pot look and feel sets it by hand
  b.setLookAndFeel(&pot_lnf_);
  addAndMakeVisible(b);
}

void LDatEditor::paint(juce::Graphics& g) {
  g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void LDatEditor::ShowAbout() {
  about_.setVisible(true);
  about_.toFront(true);
  about_.grabKeyboardFocus();
}

void LDatEditor::resized() {
  auto area = getLocalBounds().reduced(kMargin);
  in_meter_.setBounds(area.removeFromTop(kMeterH));
  out_meter_.setBounds(area.removeFromBottom(kMeterH));
  area.reduce(0, kMargin);

  auto toggles = area.removeFromRight(kToggleW);
  bypass_.setBounds(toggles.removeFromTop(kToggleH));
  toggles.removeFromTop(kMargin);
  lp32k_.setBounds(toggles.removeFromTop(kToggleH));
  toggles.removeFromTop(kMargin);
  emphasis_.setBounds(toggles.removeFromTop(kToggleH));
  // logo takes the rest of the column, so it scales to the toggle width.
  // it draws at the bottom of that, right above the output meter
  toggles.removeFromTop(kMargin);
  logo_button_.setBounds(toggles);

  // label, small gap so the top ticks stay off it, then the knob with its box.
  // scale shrinks the knob only, the box below it keeps its height
  auto place = [](juce::Slider& s, juce::Label& l, juce::Rectangle<int> col, float scale) {
    l.setBounds(col.removeFromTop(kLabelH));
    col.removeFromTop(kMargin / 2);
    if (scale < 1.0f) {
      const int box_h = s.getTextBoxHeight();
      const float knob = static_cast<float>(juce::jmin(col.getWidth(), col.getHeight() - box_h));
      col = col.withSizeKeepingCentre(col.getWidth(), juce::roundToInt(scale * knob) + box_h);
    }
    s.setBounds(col);
  };
  auto small = area.removeFromLeft(kSmallW);
  place(in_gain_, in_gain_label_, small.removeFromTop(small.getHeight() / 2), kSmallKnobScale);
  place(out_gain_, out_gain_label_, small, kSmallKnobScale);
  place(threshold_, threshold_label_, area, 1.0f);

  about_.setBounds(getLocalBounds());
}

void LDatEditor::timerCallback() {
  // input meter sits right before the quantizer, so colour it by region
  in_meter_.SetThreshold(ThresholdFromDb(threshold_raw_->load()));
  UpdateMeter(processor_.GetInputMeter(), in_state_, in_meter_);
  UpdateMeter(processor_.GetOutputMeter(), out_state_, out_meter_);
}

void LDatEditor::UpdateMeter(PeakDetector& det, MeterState& st, SegmentMeter& meter) {
  for (int ch = 0; ch < 2; ++ch) {
    const auto i = static_cast<size_t>(ch);
    const float peak = det.Take(ch);
    const float db = peak > 0.0f ? 20.0f * std::log10(peak) : kFloorDb;
    // instant attack, linear dB decay
    st.db[i] = juce::jmax(db, st.db[i] - kDecayDbPerTick, kFloorDb);
    if (peak >= 1.0f) st.over_ticks[i] = kOverHoldTicks;
    else if (st.over_ticks[i] > 0) --st.over_ticks[i];
    meter.SetLevel(ch, st.db[i], st.over_ticks[i] > 0);
  }
}

}  // namespace ldat
