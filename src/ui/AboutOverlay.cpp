#include "AboutOverlay.h"

#include <BinaryData.h>

namespace ldat {

namespace {

// how dark the ui behind the panel goes. 1.0 hides it completely
constexpr float kAboutDimAlpha = 0.72f;

// panel size, clamped to the editor if it ever gets smaller than this
constexpr int kAboutPanelW = 470;
constexpr int kAboutPanelH = 260;
constexpr int kAboutPanelPad = 12;
constexpr float kAboutCornerPx = 4.0f;

constexpr juce::uint32 kAboutPanelBgArgb = 0xff1c1c1c;
constexpr juce::uint32 kAboutBorderArgb = 0xff909090;  // same grey as the logo
constexpr juce::uint32 kAboutTextArgb = 0xffd0d0d0;

// monospaced, the about text is laid out by hand
constexpr float kAboutFontPx = 13.0f;

}  // namespace

AboutOverlay::AboutOverlay() {
  // eat every click, including the ones that land on the dimmed area
  setInterceptsMouseClicks(true, true);
  setWantsKeyboardFocus(true);

  text_.setMultiLine(true, true);
  text_.setReadOnly(true);
  text_.setScrollbarsShown(true);
  text_.setCaretVisible(false);
  text_.setPopupMenuEnabled(false);
  // focus stays on the overlay so escape reaches keyPressed below. a read only
  // editor still scrolls with the wheel and the scrollbar without focus
  text_.setWantsKeyboardFocus(false);

  text_.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
  text_.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
  text_.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
  text_.setColour(juce::TextEditor::textColourId, juce::Colour(kAboutTextArgb));

  text_.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
                                             kAboutFontPx, juce::Font::plain)));
  // name and version come from CMake project(VERSION) via juce_add_plugin, so
  // about.txt carries no version of its own
  const auto heading = juce::String(JucePlugin_Name) + " Version " + JucePlugin_VersionString;
  const auto body = juce::String::fromUTF8(BinaryData::about_txt, BinaryData::about_txtSize);
  text_.setText(heading + "\n" + body, false);
  addAndMakeVisible(text_);
}

juce::Rectangle<int> AboutOverlay::PanelRect() const {
  const auto w = juce::jmin(kAboutPanelW, getWidth());
  const auto h = juce::jmin(kAboutPanelH, getHeight());
  return getLocalBounds().withSizeKeepingCentre(w, h);
}

void AboutOverlay::paint(juce::Graphics& g) {
  g.fillAll(juce::Colours::black.withAlpha(kAboutDimAlpha));

  const auto panel = PanelRect().toFloat();
  g.setColour(juce::Colour(kAboutPanelBgArgb));
  g.fillRoundedRectangle(panel, kAboutCornerPx);
  g.setColour(juce::Colour(kAboutBorderArgb));
  g.drawRoundedRectangle(panel.reduced(0.5f), kAboutCornerPx, 1.0f);
}

void AboutOverlay::resized() {
  text_.setBounds(PanelRect().reduced(kAboutPanelPad));
}

void AboutOverlay::mouseUp(const juce::MouseEvent& e) {
  // clicks inside the panel belong to the text, only the dimmed area closes
  if (!PanelRect().contains(e.getPosition()) && on_close) on_close();
}

bool AboutOverlay::keyPressed(const juce::KeyPress& k) {
  if (k != juce::KeyPress::escapeKey) return false;
  if (on_close) on_close();
  return true;
}

}  // namespace ldat
