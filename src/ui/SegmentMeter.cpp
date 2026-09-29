#include "ui/SegmentMeter.h"

#include <cmath>

#include "dsp/DatNonlinear.h"
#include "ui/Fonts.h"

namespace ldat {

namespace {

constexpr int kInset = 2;
constexpr int kLeftW = 56;       // title / "-dB" column, also holds the L/R box
constexpr int kLrW = 20;         // L / R box inside the left column
constexpr int kOverW = 36;       // OVER box
constexpr int kScaleH = 10;      // dB numbers row
constexpr int kGap = 2;

constexpr float kScaleFontPx = 11.0f;   // title and the dB numbers
constexpr float kLrFontPx = 12.0f;
constexpr float kOverFontPx = 9.0f;

// which segments get a number above them, same picks as the SONY TCD-D8 print
constexpr bool kLabelled[SegmentMeter::kNumSegments] = { true, true, false, true, false, false, true, false, true, true, true, true};

const juce::Colour kBg(0xff101010);
const juce::Colour kOff(0xff2a2a2a);
const juce::Colour kOn(0xffe8e8f8);
const juce::Colour kOverOn(0xffff3030);
const juce::Colour kText(0xffc0c0c0);

// lit colour per quantizer region: 0 = pass through, s = bits dropped.
// 1..6 is the standard table range, pale yellow to orange. threshold below
// 512 goes past 6, up to 13 at threshold 4, orange to red from there so the
// standard look stays the same.
constexpr int kTableShift = 6;
constexpr int kMaxShift = 13;
const juce::Colour kShiftLow(0xfff8f0b0);
const juce::Colour kShiftHigh(0xffff8000);
const juce::Colour kShiftHot(0xffff2000);

juce::Colour ShiftColour(int s) {
  if (s <= 0) return kOn;
  if (s <= kTableShift) {
    const float t = static_cast<float>(s - 1) / static_cast<float>(kTableShift - 1);
    return kShiftLow.interpolatedWith(kShiftHigh, t);
  }
  const float t = static_cast<float>(s - kTableShift) / static_cast<float>(kMaxShift - kTableShift);
  return kShiftHigh.interpolatedWith(kShiftHot, t);
}

// which region a level falls in. goes through the sample magnitude and the
// same RegionShift as the dsp, so float dB rounding can not put a segment on
// the wrong side of a boundary
int ShiftForDb(float db, int threshold) {
  float amp = std::round(32768.0f * std::pow(10.0f, db / 20.0f));
  if (amp > 32767.0f) amp = 32767.0f;
  return RegionShift(static_cast<int>(amp), threshold);
}

}  // namespace

SegmentMeter::SegmentMeter(const juce::String& title) : title_(title) {
  setOpaque(true);
}

void SegmentMeter::SetLevel(int ch, float db, bool over) {
  if (ch < 0 || ch >= kNumChannels) return;
  const auto i = static_cast<size_t>(ch);
  // exact compare on purpose, a tolerance would eat real repaints
  if (juce::exactlyEqual(level_db_[i], db) && over_[i] == over) return;
  level_db_[i] = db;
  over_[i] = over;
  repaint();
}

void SegmentMeter::SetThreshold(int threshold) {
  if (threshold_ == threshold) return;
  threshold_ = threshold;
  repaint();
}

juce::Rectangle<int> SegmentMeter::ChannelRow(int ch) const {
  auto area = getLocalBounds().reduced(kInset);
  area.removeFromTop(kScaleH);
  const int row_h = (area.getHeight() - kGap) / kNumChannels;
  return area.withY(area.getY() + ch * (row_h + kGap)).withHeight(row_h);
}

juce::Rectangle<int> SegmentMeter::OverRect(int ch) const {
  return ChannelRow(ch).removeFromRight(kOverW);
}

void SegmentMeter::mouseDown(const juce::MouseEvent& e) {
  for (int ch = 0; ch < kNumChannels; ++ch) {
    if (OverRect(ch).contains(e.getPosition()) && on_over_reset) {
      on_over_reset();
      return;
    }
  }
}

void SegmentMeter::paint(juce::Graphics& g) {
  g.fillAll(kBg);
  auto area = getLocalBounds().reduced(kInset);

  // scale row: title on the left, numbers over the segments
  auto scale_row = area.removeFromTop(kScaleH);
  auto title_box = scale_row.removeFromLeft(kLeftW);
  scale_row.removeFromRight(kOverW + kGap);
  g.setColour(kText);
  g.setFont(fonts::Bold(kScaleFontPx));
  g.drawText(title_ + " -dB", title_box, juce::Justification::centredLeft);
  g.setFont(fonts::Regular(kScaleFontPx));
  const float seg_w = static_cast<float>(scale_row.getWidth()) / kNumSegments;
  for (int i = 0; i < kNumSegments; ++i) {
    if (!kLabelled[i]) continue;
    const auto r = juce::Rectangle<float>(scale_row.getX() + seg_w * i, (float)scale_row.getY(), seg_w, (float)scale_row.getHeight());
    g.drawText(juce::String(static_cast<int>(-kSegmentDb[static_cast<size_t>(i)])), r.toNearestInt(), juce::Justification::centred);
  }

  // one row per channel
  for (int ch = 0; ch < kNumChannels; ++ch) {
    auto row = ChannelRow(ch);
    const auto ci = static_cast<size_t>(ch);

    auto label = row.removeFromLeft(kLeftW).removeFromLeft(kLrW);
    g.setColour(kOn);
    g.fillRect(label.reduced(1));
    g.setColour(kBg);
    g.setFont(fonts::Bold(kLrFontPx));
    g.drawText(ch == 0 ? "L" : "R", label, juce::Justification::centred);

    auto over = row.removeFromRight(kOverW);
    row.removeFromRight(kGap);
    g.setColour(over_[ci] ? kOverOn : kOff);
    g.fillRect(over.reduced(1));
    g.setColour(over_[ci] ? kBg : kText);
    g.setFont(fonts::Bold(kOverFontPx));
    g.drawText("OVER", over, juce::Justification::centred);

    const float w = static_cast<float>(row.getWidth()) / kNumSegments;
    for (int i = 0; i < kNumSegments; ++i) {
      const float seg_db = kSegmentDb[static_cast<size_t>(i)];
      const bool lit = level_db_[ci] >= seg_db;
      auto r = juce::Rectangle<float>(row.getX() + w * i, (float)row.getY(), w, (float)row.getHeight()).reduced(1.0f, 1.5f);
      if (!lit) g.setColour(kOff);
      else if (threshold_ > 0) g.setColour(ShiftColour(ShiftForDb(seg_db, threshold_)));
      else g.setColour(kOn);
      g.fillRect(r);
    }
  }
}

}  // namespace ldat
